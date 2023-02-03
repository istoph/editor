// SPDX-License-Identifier: BSL-1.0

#include "overwritedialog.h"

#include <Tui/ZHBoxLayout.h>
#include <Tui/ZVBoxLayout.h>

OverwriteDialog::OverwriteDialog(Tui::ZWidget *parent, QString fileName) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setContentsMargins({ 1, 1, 2, 1});
    setWindowTitle("Overwrite?");

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);
    {
        Tui::ZTextLine *tl = new Tui::ZTextLine("Overwrite the existing file?", this);
        vbox->addWidget(tl);

        Tui::ZTextLine *tl2 = new Tui::ZTextLine(fileName, this);
        vbox->addWidget(tl2);
    }

    {
        Tui::ZHBoxLayout *hbox = new Tui::ZHBoxLayout();
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
