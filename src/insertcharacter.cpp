#include "insertcharacter.h"

#include <Tui/ZButton.h>
#include <Tui/ZLabel.h>

InsertCharacter::InsertCharacter(Tui::ZWidget *parent) : Dialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose | Tui::ZWindow::MoveOption);
    setWindowTitle("Insert Character");
    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    HBoxLayout* hbox1 = new HBoxLayout();
    Tui::ZLabel *hexLabel = new Tui::ZLabel(this);
    hexLabel->setText("Hex:   0x");
    hbox1->addWidget(hexLabel);

    InputBox *hexInputBox = new InputBox(this);
    hexInputBox->setFocus();
    hbox1->addWidget(hexInputBox);

    vbox->add(hbox1);
    vbox->addStretch();

    HBoxLayout* hbox2 = new HBoxLayout();
    Tui::ZLabel *intLabel = new Tui::ZLabel(this);
    intLabel->setText("Int:     ");
    hbox2->addWidget(intLabel);

    InputBox *intInputBox = new InputBox(this);
    hbox2->addWidget(intInputBox);

    vbox->add(hbox2);
    vbox->addStretch();

    HBoxLayout* hbox3 = new HBoxLayout();
    //replace it with something that can also display all characters
    Tui::ZLabel *previewLabel = new Tui::ZLabel(this);
    previewLabel->setText("Preview: ");
    hbox3->addWidget(previewLabel);

    vbox->add(hbox3);

    HBoxLayout* hbox4 = new HBoxLayout();
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

    QObject::connect(hexInputBox, &InputBox::textChanged, [=]{
        if(hexInputBox->focus()) {
            _codepoint = hexInputBox->text().toInt(&_check, 16);
            if(_check && _codepoint >= 0) {
                previewLabel->setText("Preview: "+ intToChar(_codepoint));
                intInputBox->setText(QString::number(_codepoint, 10).toUpper());
                cancelBtn->setDefault(false);
                insertButton->setDefault(true);
                insertButton->setEnabled(true);
            } else {
                previewLabel->setText("Preview: ERROR");
                insertButton->setEnabled(false);
                insertButton->setDefault(false);
                cancelBtn->setDefault(true);
            }
        }
    });

    QObject::connect(intInputBox, &InputBox::textChanged, [=]{
        if(intInputBox->focus()) {
            _codepoint = intInputBox->text().toInt(&_check, 10);
            if(_check && _codepoint >= 0) {
                previewLabel->setText("Preview: "+ intToChar(_codepoint));
                hexInputBox->setText(QString::number(_codepoint, 16).toUpper());
                cancelBtn->setDefault(false);
                insertButton->setDefault(true);
                insertButton->setEnabled(true);
            } else {
                previewLabel->setText("Preview: ERROR");
                insertButton->setEnabled(false);
                insertButton->setDefault(false);
                cancelBtn->setDefault(true);
            }
        }
    });


    QObject::connect(cancelBtn, &Tui::ZButton::clicked, this, &InsertCharacter::rejected);
    QObject::connect(insertButton, &Tui::ZButton::clicked, [this]{
        if(_check) {
            Q_EMIT characterSelected(intToChar(_codepoint));
        }
        deleteLater();
    });
}

void InsertCharacter::rejected() {
    deleteLater();
}

QString InsertCharacter::intToChar(int i) {
    if(i < 0 || i > 0x10FFFF) {
        return 0;
    } else if(i <= 0xFFFF) {
        return QString(1,QChar(i)).toUtf8();
    } else if (i <= 0x10FFFF){
        return (QString(1,QChar::highSurrogate(i))+QString(1,QChar::lowSurrogate(i))).toUtf8();
    }
    return 0;
}
