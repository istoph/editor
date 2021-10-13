#include "formattingdialog.h"

FormattingDialog::FormattingDialog(Tui::ZWidget *parent) : Dialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::MoveOption);
    setFocus();
    setWindowTitle("Formatting");
    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    HBoxLayout *hbox1 = new HBoxLayout();
    _formattingCharacters = new Tui::ZCheckBox(this);
    _formattingCharacters->setMarkup("Formatting <m>C</m>haracters");
    _formattingCharacters->focus();
    hbox1->addWidget(_formattingCharacters);
    vbox->add(hbox1);
    //vbox->addStretch();

    HBoxLayout *hbox2 = new HBoxLayout();
    _colorTabs = new Tui::ZCheckBox(this);
    _colorTabs->setMarkup("Color <m>T</m>abs");
    hbox2->addWidget(_colorTabs);
    vbox->add(hbox2);
    //vbox->addStretch();

    HBoxLayout *hbox3 = new HBoxLayout();
    _colorSpaceEnd = new Tui::ZCheckBox(this);
    _colorSpaceEnd->setMarkup("Color <m>S</m>pacs at end of line");
    hbox3->addWidget(_colorSpaceEnd);
    vbox->add(hbox3);
    vbox->addStretch();

    HBoxLayout *hbox5 = new HBoxLayout();
    Tui::ZButton *cancelButton = new Tui::ZButton(this);
    cancelButton->setText("Cancel");
    hbox5->addWidget(cancelButton);

    Tui::ZButton *saveButton = new Tui::ZButton(this);
    saveButton->setText("Save");
    saveButton->setDefault(true);
    hbox5->addWidget(saveButton);
    vbox->add(hbox5);

    QObject::connect(saveButton, &Tui::ZButton::clicked, [this] {
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
