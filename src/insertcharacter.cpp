// SPDX-License-Identifier: BSL-1.0

#include "insertcharacter.h"

#include <Tui/ZButton.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZPalette.h>
#include <Tui/ZVBoxLayout.h>


InsertCharacter::InsertCharacter(Tui::ZWidget *parent) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose
               | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setWindowTitle("Insert Character");
    setContentsMargins({1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    Tui::ZHBoxLayout *hbox1 = new Tui::ZHBoxLayout();
    Tui::ZLabel *hexLabel = new Tui::ZLabel(this);
    hexLabel->setText("Hex:   0x");
    hbox1->addWidget(hexLabel);

    Tui::ZInputBox *hexInputBox = new Tui::ZInputBox(this);
    hexInputBox->setFocus();
    hbox1->addWidget(hexInputBox);

    vbox->add(hbox1);
    vbox->addStretch();

    Tui::ZHBoxLayout *hbox2 = new Tui::ZHBoxLayout();
    Tui::ZLabel *intLabel = new Tui::ZLabel(this);
    intLabel->setText("Decimal: ");
    hbox2->addWidget(intLabel);

    Tui::ZInputBox *intInputBox = new Tui::ZInputBox(this);
    hbox2->addWidget(intInputBox);

    vbox->add(hbox2);
    vbox->addStretch();

    Tui::ZHBoxLayout *hbox3 = new Tui::ZHBoxLayout();
    Tui::ZInputBox *preview = new Tui::ZInputBox(this);

    Tui::ZPalette tmpPalette = Tui::ZPalette::classic();
    tmpPalette.setColors({{"lineedit.bg", Tui::Colors::lightGray}, {"lineedit.fg", Tui::Colors::black}});
    preview->setPalette(tmpPalette);
    preview->setText(" Preview: ");
    preview->setEnabled(false);
    hbox3->addWidget(preview);
    vbox->add(hbox3);

    Tui::ZHBoxLayout *hbox4 = new Tui::ZHBoxLayout();
    Tui::ZButton *cancelBtn = new Tui::ZButton(this);
    cancelBtn->setGeometry({3, 5, 7, 7});
    cancelBtn->setText("Cancel");
    hbox4->addWidget(cancelBtn);

    Tui::ZButton *insertButton = new Tui::ZButton(this);
    insertButton->setGeometry({15, 5, 7, 7});
    insertButton->setText("Insert");
    insertButton->setDefault(true);
    insertButton->setEnabled(false);
    hbox4->addWidget(insertButton);

    vbox->add(hbox4);

    QObject::connect(hexInputBox, &Tui::ZInputBox::textChanged, this, [this,hexInputBox,preview,intInputBox,cancelBtn,insertButton] {
        if (hexInputBox->focus()) {
            _codepoint = hexInputBox->text().toInt(&_check, 16);
            if (_check && _codepoint >= 0 && _codepoint <= 0x10FFFF) {
                preview->setText(" Preview: " + intToChar(_codepoint));
                intInputBox->setText(QString::number(_codepoint, 10).toUpper());
                cancelBtn->setDefault(false);
                insertButton->setDefault(true);
                insertButton->setEnabled(true);
            } else {
                preview->setText(" Preview: ERROR");
                insertButton->setEnabled(false);
                insertButton->setDefault(false);
                cancelBtn->setDefault(true);
            }
        }
    });

    QObject::connect(intInputBox, &Tui::ZInputBox::textChanged, this, [this,hexInputBox,preview,intInputBox,cancelBtn,insertButton] {
        if (intInputBox->focus()) {
            _codepoint = intInputBox->text().toInt(&_check, 10);
            if (_check && _codepoint >= 0 && _codepoint <= 0x10FFFF) {
                preview->setText(" Preview: " + intToChar(_codepoint));
                hexInputBox->setText(QString::number(_codepoint, 16).toUpper());
                cancelBtn->setDefault(false);
                insertButton->setDefault(true);
                insertButton->setEnabled(true);
            } else {
                preview->setText(" Preview: ERROR");
                insertButton->setEnabled(false);
                insertButton->setDefault(false);
                cancelBtn->setDefault(true);
            }
        }
    });


    QObject::connect(cancelBtn, &Tui::ZButton::clicked, this, &InsertCharacter::rejected);
    QObject::connect(insertButton, &Tui::ZButton::clicked, [this] {
        if (_check) {
            Q_EMIT characterSelected(intToChar(_codepoint));
        }
        deleteLater();
    });
}

void InsertCharacter::rejected() {
    deleteLater();
}

QString InsertCharacter::intToChar(int i) {
    if (i < 0 || i > 0x10FFFF) {
        return "";
    } else if (i <= 0xFFFF) {
        return QString(1, QChar(i));
    } else if (i <= 0x10FFFF){
        return (QString(1, QChar::highSurrogate(i)) + QString(1, QChar::lowSurrogate(i)));
    }
    return "";
}
