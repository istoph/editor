// SPDX-License-Identifier: BSL-1.0

#include "tabdialog.h"

#include <Tui/ZButton.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>


TabDialog::TabDialog(Tui::ZWidget *parent) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setFocus();
    setWindowTitle("Tab settings");
    setContentsMargins({1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    Tui::ZHBoxLayout *hbox1 = new Tui::ZHBoxLayout();
    _tabRadioButton = new Tui::ZRadioButton(this);
    _tabRadioButton->setMarkup("<m>T</m>ab");
    hbox1->addWidget(_tabRadioButton);

    _blankRadioButton = new Tui::ZRadioButton(this);
    _blankRadioButton->setMarkup("<m>S</m>paces");
    hbox1->addWidget(_blankRadioButton);

    vbox->add(hbox1);
    vbox->addStretch();

    Tui::ZHBoxLayout *hbox2 = new Tui::ZHBoxLayout();
    Tui::ZLabel *tabstopLable = new Tui::ZLabel(this);
    tabstopLable->setText("Tab Width: ");
    hbox2->addWidget(tabstopLable);

    _tabsizeInputBox = new Tui::ZInputBox(this);
    _tabsizeInputBox->setFocus();
    _tabsizeInputBox->setEnabled(true);
    hbox2->addWidget(_tabsizeInputBox);
    vbox->add(hbox2);
    vbox->addStretch();

    Tui::ZHBoxLayout *hbox3 = new Tui::ZHBoxLayout();
    Tui::ZLabel *tabtospaceLabel = new Tui::ZLabel(this);
    tabtospaceLabel->setText("Tab to Space: ");
    hbox3->addWidget(tabtospaceLabel);

    Tui::ZButton *convertButton = new Tui::ZButton(this);
    convertButton->setText("Convert");
    hbox3->addWidget(convertButton);
    vbox->add(hbox3);
    vbox->addStretch();

    Tui::ZHBoxLayout *hbox4 = new Tui::ZHBoxLayout();
    _eatSpaceBeforeTabBox = new Tui::ZCheckBox(this);
    _eatSpaceBeforeTabBox->setMarkup("eat space <m>b</m>efore tab");
    hbox4->addWidget(_eatSpaceBeforeTabBox);
    vbox->add(hbox4);
    vbox->addStretch();

    Tui::ZHBoxLayout *hbox5 = new Tui::ZHBoxLayout();

    hbox5->addStretch();

    Tui::ZButton *cancelButton = new Tui::ZButton(this);
    cancelButton->setText("Cancel");
    hbox5->addWidget(cancelButton);

    Tui::ZButton *saveButton = new Tui::ZButton(this);
    saveButton->setText("Save");
    saveButton->setDefault(true);
    hbox5->addWidget(saveButton);
    vbox->add(hbox5);

    QObject::connect(convertButton, &Tui::ZButton::clicked, [this] {
        convert(!_blankRadioButton->checked(), _tabsizeInputBox->text().toInt());
        deleteLater();
    });

    QObject::connect(saveButton, &Tui::ZButton::clicked, [this] {
        bool eat;
        if (_eatSpaceBeforeTabBox->checkState() == Qt::CheckState::Checked) {
            eat = true;
        } else {
            eat = false;
        }
        Q_EMIT settingsChanged(!_blankRadioButton->checked(), _tabsizeInputBox->text().toInt(), eat);
        deleteLater();
    });

    QObject::connect(_tabsizeInputBox, &Tui::ZInputBox::textChanged, [this, convertButton, saveButton] {
        if(_tabsizeInputBox->text().toInt() > 0 && _tabsizeInputBox->text().toInt() < 100) {
            convertButton->setEnabled(true);
            saveButton->setEnabled(true);
        } else {
            convertButton->setEnabled(false);
            saveButton->setEnabled(false);
        }
    });

    QObject::connect(cancelButton, &Tui::ZButton::clicked, [this] {
        deleteLater();
    });
}

void TabDialog::updateSettings(bool useTabs, int indentSize, bool eatSpaceBeforeTabs) {
    _tabRadioButton->setChecked(useTabs);
    _blankRadioButton->setChecked(!useTabs);
    _tabsizeInputBox->setText(QString::number(indentSize));
    if(eatSpaceBeforeTabs) {
        _eatSpaceBeforeTabBox->setCheckState(Qt::CheckState::Checked);
    } else {
        _eatSpaceBeforeTabBox->setCheckState(Qt::CheckState::Unchecked);
    }
}
