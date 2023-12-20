// SPDX-License-Identifier: BSL-1.0

#include "formattingdialog.h"

#include <Tui/ZButton.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZVBoxLayout.h>


FormattingDialog::FormattingDialog(Tui::ZWidget *parent) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption
               | Tui::ZWindow::DeleteOnClose);
    setFocus();
    setWindowTitle("Formatting");
    setContentsMargins({ 1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    Tui::ZHBoxLayout *hbox1 = new Tui::ZHBoxLayout();
    _formattingCharacters = new Tui::ZCheckBox(this);
    _formattingCharacters->setMarkup("Formatting <m>C</m>haracters");
    _formattingCharacters->setFocus();
    hbox1->addWidget(_formattingCharacters);
    vbox->add(hbox1);
    //vbox->addStretch();

    Tui::ZHBoxLayout *hbox2 = new Tui::ZHBoxLayout();
    _colorTabs = new Tui::ZCheckBox(this);
    _colorTabs->setMarkup("Color <m>T</m>abs");
    hbox2->addWidget(_colorTabs);
    vbox->add(hbox2);
    //vbox->addStretch();

    Tui::ZHBoxLayout *hbox3 = new Tui::ZHBoxLayout();
    _colorSpaceEnd = new Tui::ZCheckBox(this);
    _colorSpaceEnd->setMarkup("Color <m>S</m>paces at end of line");
    hbox3->addWidget(_colorSpaceEnd);
    vbox->add(hbox3);
    vbox->addStretch();

    Tui::ZHBoxLayout *hbox5 = new Tui::ZHBoxLayout();

    hbox5->addStretch();

    Tui::ZButton *cancelButton = new Tui::ZButton(this);
    cancelButton->setText("Cancel");
    hbox5->addWidget(cancelButton);

    Tui::ZButton *saveButton = new Tui::ZButton(this);
    saveButton->setText("Ok");
    saveButton->setDefault(true);
    hbox5->addWidget(saveButton);
    vbox->add(hbox5);

    QObject::connect(saveButton, &Tui::ZButton::clicked, this, [this] {
        Q_EMIT settingsChanged(_formattingCharacters->checkState() == Qt::CheckState::Checked,
                               _colorTabs->checkState() == Qt::CheckState::Checked,
                               _colorSpaceEnd->checkState() == Qt::CheckState::Checked);
        deleteLater();
    });

    QObject::connect(cancelButton, &Tui::ZButton::clicked, [this] {
        deleteLater();
    });
}

void FormattingDialog::updateSettings(bool formattingCharacters, bool colorTabs, bool colorSpaceEnd) {
    if (formattingCharacters) {
        _formattingCharacters->setCheckState(Qt::CheckState::Checked);
    } else {
        _formattingCharacters->setCheckState(Qt::CheckState::Unchecked);
    }

    if (colorTabs) {
        _colorTabs->setCheckState(Qt::CheckState::Checked);
    } else {
        _colorTabs->setCheckState(Qt::CheckState::Unchecked);
    }

    if (colorSpaceEnd) {
        _colorSpaceEnd->setCheckState(Qt::CheckState::Checked);
    } else {
        _colorSpaceEnd->setCheckState(Qt::CheckState::Unchecked);
    }

}
