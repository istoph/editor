#include "overwritedialog.h"

OverwriteDialog::OverwriteDialog(Tui::ZWidget *parent, QString fileName) : Dialog(parent) {
    setOptions(Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setContentsMargins({ 1, 1, 2, 1});
    setWindowTitle("Overwrite?");

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);
    {
        Tui::ZTextLine *tl = new Tui::ZTextLine("Overwrite the existing file?", this);
        vbox->addWidget(tl);

        Tui::ZTextLine *tl2 = new Tui::ZTextLine(fileName, this);
        vbox->addWidget(tl2);
    }

    {
        HBoxLayout *hbox = new HBoxLayout();
        hbox->setSpacing(2);
        _cancelButton = new Tui::ZButton(this);
        _cancelButton->setText("Cancel");
        hbox->addWidget(_cancelButton);

        _okButton = new Tui::ZButton(this);
        _okButton->setText("Overwrite");
        _okButton->setDefault(true);
        hbox->addWidget(_okButton);
        vbox->add(hbox);
    }

    QObject::connect(_cancelButton, &Tui::ZButton::clicked, this, [this] { confirm(false); });
    QObject::connect(_okButton, &Tui::ZButton::clicked, this, [this] { confirm(true); });
}
