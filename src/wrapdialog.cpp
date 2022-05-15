#include "wrapdialog.h"

#include <Tui/ZHBoxLayout.h>
#include <Tui/ZVBoxLayout.h>

WrapDialog::WrapDialog(Tui::ZWidget *parent, File *file) : Dialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::MoveOption | Tui::ZWindow::ResizeOption | Tui::ZWindow::AutomaticOption);
    setWindowTitle("Wrap long lines");
    setContentsMargins({ 1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    _noWrapRadioButton = new Tui::ZRadioButton(this);
    _noWrapRadioButton->setMarkup("<m>N</m>o Wrap");
    if(file->getWrapOption() == Tui::ZTextOption::NoWrap) {
        _noWrapRadioButton->setChecked(true);
    }
    vbox->addWidget(_noWrapRadioButton);

    _wordWrapRadioButton = new Tui::ZRadioButton(this);
    _wordWrapRadioButton->setMarkup("<m>W</m>ord Wrap");
    if(file->getWrapOption() == Tui::ZTextOption::WordWrap) {
        _wordWrapRadioButton->setChecked(true);
    }
    vbox->addWidget(_wordWrapRadioButton);

    _wrapAnywhereRadioButton = new Tui::ZRadioButton(this);
    _wrapAnywhereRadioButton->setMarkup("Wrap <m>A</m>nywhere");
    if(file->getWrapOption() == Tui::ZTextOption::WrapAnywhere) {
        _wrapAnywhereRadioButton->setChecked(true);
    }
   vbox->addWidget(_wrapAnywhereRadioButton);

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

   QObject::connect(cancelButton, &Tui::ZButton::clicked, [this] {
       deleteLater();
   });

   QObject::connect(saveButton, &Tui::ZButton::clicked, [this, file] {
       if(_noWrapRadioButton->checked()) {
           file->setWrapOption(Tui::ZTextOption::NoWrap);
       } else if (_wordWrapRadioButton->checked()) {
           file->setWrapOption(Tui::ZTextOption::WordWrap);
       } else if (_wrapAnywhereRadioButton->checked()) {
           file->setWrapOption(Tui::ZTextOption::WrapAnywhere);
       }

       deleteLater();
   });

}
