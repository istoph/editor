#include "wrapdialog.h"

WrapDialog::WrapDialog(Tui::ZWidget *parent, File *file) : Dialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::MoveOption | Tui::ZWindow::ResizeOption | Tui::ZWindow::AutomaticOption);
    setFocus();
    setWindowTitle("Wrap long lines");
    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    _noWrapRadioButton = new Tui::ZRadioButton(this);
    _noWrapRadioButton->setMarkup("<m>N</m>o Wrap");
    if(file->getWrapOption() == ZTextOption::NoWrap) {
        _noWrapRadioButton->setChecked(true);
    }
    vbox->addWidget(_noWrapRadioButton);

    _wordWrapRadioButton = new Tui::ZRadioButton(this);
    _wordWrapRadioButton->setMarkup("<m>W</m>ord Wrap");
    if(file->getWrapOption() == ZTextOption::WordWrap) {
        _wordWrapRadioButton->setChecked(true);
    }
    vbox->addWidget(_wordWrapRadioButton);

    _hardWrapRadioButton = new Tui::ZRadioButton(this);
    _hardWrapRadioButton->setMarkup("<m>H</m>ard Wrap");
    if(file->getWrapOption() == ZTextOption::WrapAnywhere) {
        _hardWrapRadioButton->setChecked(true);
    }
   vbox->addWidget(_hardWrapRadioButton);

   HBoxLayout *hbox5 = new HBoxLayout();
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
           file->setWrapOption(ZTextOption::NoWrap);
       } else if (_wordWrapRadioButton->checked()) {
           file->setWrapOption(ZTextOption::WordWrap);
       } else if (_hardWrapRadioButton->checked()) {
           file->setWrapOption(ZTextOption::WrapAnywhere);
       }

       deleteLater();
   });

}
