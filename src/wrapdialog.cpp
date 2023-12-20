// SPDX-License-Identifier: BSL-1.0

#include "wrapdialog.h"

#include <Tui/ZButton.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>


WrapDialog::WrapDialog(Tui::ZWidget *parent, File *file) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::MoveOption | Tui::ZWindow::ResizeOption
               | Tui::ZWindow::AutomaticOption | Tui::ZWindow::DeleteOnClose);
    setWindowTitle("Wrap long lines");
    setContentsMargins({1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    _noWrapRadioButton = new Tui::ZRadioButton(this);
    _noWrapRadioButton->setMarkup("<m>N</m>o Wrap");
    if (file->wordWrapMode() == Tui::ZTextOption::NoWrap) {
        _noWrapRadioButton->setChecked(true);
    }
    vbox->addWidget(_noWrapRadioButton);

    _wordWrapRadioButton = new Tui::ZRadioButton(this);
    _wordWrapRadioButton->setMarkup("<m>W</m>ord Wrap");
    if(file->wordWrapMode() == Tui::ZTextOption::WordWrap) {
        _wordWrapRadioButton->setChecked(true);
    }
    vbox->addWidget(_wordWrapRadioButton);

    _wrapAnywhereRadioButton = new Tui::ZRadioButton(this);
    _wrapAnywhereRadioButton->setMarkup("Wrap <m>A</m>nywhere");
    if(file->wordWrapMode() == Tui::ZTextOption::WrapAnywhere) {
        _wrapAnywhereRadioButton->setChecked(true);
    }
    vbox->addWidget(_wrapAnywhereRadioButton);

    Tui::ZHBoxLayout *hintLayout = new Tui::ZHBoxLayout();
    Tui::ZLabel *hintLabel = new Tui::ZLabel(Tui::withMarkup,  "Display Right <m>M</m>argin at Column: ", this);
    hintLayout->addWidget(hintLabel);
    _rightMarginHintInput = new Tui::ZInputBox(QString::number(file->rightMarginHint()), this);
    _rightMarginHintInput->setMaximumSize(6, 1);
    hintLabel->setBuddy(_rightMarginHintInput);
    hintLayout->addWidget(_rightMarginHintInput);

    vbox->add(hintLayout);

    _noWrapRadioButton->setFocus();

    Tui::ZHBoxLayout *hbox5 = new Tui::ZHBoxLayout();
    Tui::ZButton *cancelButton = new Tui::ZButton(this);
    cancelButton->setText("Cancel");
    hbox5->addWidget(cancelButton);

    Tui::ZButton *saveButton = new Tui::ZButton(this);
    saveButton->setText("Save");
    saveButton->setDefault(true);
    hbox5->addWidget(saveButton);
    vbox->add(hbox5);

    QObject::connect(cancelButton, &Tui::ZButton::clicked, this, [this] {
       deleteLater();
    });

    QObject::connect(saveButton, &Tui::ZButton::clicked, this, [this, file] {
       if (_noWrapRadioButton->checked()) {
           file->setWordWrapMode(Tui::ZTextOption::NoWrap);
       } else if (_wordWrapRadioButton->checked()) {
           file->setWordWrapMode(Tui::ZTextOption::WordWrap);
       } else if (_wrapAnywhereRadioButton->checked()) {
           file->setWordWrapMode(Tui::ZTextOption::WrapAnywhere);
       }

       bool ok = false;
       int marginHint = _rightMarginHintInput->text().toInt(&ok);
       if (ok) {
           file->setRightMarginHint(marginHint);
       }

       deleteLater();
    });

}
