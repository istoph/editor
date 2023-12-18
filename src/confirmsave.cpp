// SPDX-License-Identifier: BSL-1.0

#include "confirmsave.h"

#include <Tui/ZButton.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>


ConfirmSave::ConfirmSave(Tui::ZWidget *parent, QString filename, Type type, bool saveable) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    QString title, nosave, save, mainLabel;
    if (type == Close) {
        title = "Close";
        nosave = "Discard";
        if (saveable) {
            save = "Save";
        } else {
            save = "Save as";
        }
        mainLabel = "Save changes to \"" + filename + "\"?";
    } else if (type == CloseUnnamed) {
        title = "Close";
        nosave = "Discard";
        save = "Save as";
        mainLabel = "Save changes before closing?";
    } else if (type == Reload) {
        title = "Reload";
        nosave = "Discard and Reload";
        // `save` is not used in this mode
        save = "";
        mainLabel = "Discard changes to \"" + filename + "\"?";
    } else if (type == QuitUnnamed) {
        title = "Exit";
        nosave = "Quit";
        save = "Save as and Quit";
        mainLabel = "Save changes before closing?";
    } else {
        title = "Exit";
        nosave = "Quit";
        if (saveable) {
            save = "Save and Quit";
        } else {
            save = "Save as and Quit";
        }
        mainLabel = "Save changes to \"" + filename + "\"?";
    }

    //Dialog Box
    setWindowTitle(title);
    setContentsMargins({1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    //Label
    Tui::ZLabel *l1 = new Tui::ZLabel(this);
    l1->setText(mainLabel);
    vbox->addWidget(l1);
    vbox->addStretch();

    Tui::ZHBoxLayout* hbox = new Tui::ZHBoxLayout();

    hbox->addStretch();

    //Cancel
    Tui::ZButton *bCancel = new Tui::ZButton(this);
    if (type == Reload) {
        bCancel->setDefault(true);
        bCancel->setFocus();
    }
    bCancel->setText("Cancel");
    hbox->addWidget(bCancel);

    //Discard
    Tui::ZButton *bDiscard = new Tui::ZButton(this);
    bDiscard->setText(nosave);
    hbox->addWidget(bDiscard);
    hbox->setSpacing(1);

    //Save
    if (type != Reload) {
        Tui::ZButton *bSave = new Tui::ZButton(this);
        bSave->setText(save);
        bSave->setDefault(true);
        bSave->setFocus();
        hbox->addWidget(bSave);
        QObject::connect(bSave, &Tui::ZButton::clicked, this, &ConfirmSave::saveSelected);
    }

    vbox->add(hbox);

    QObject::connect(bCancel, &Tui::ZButton::clicked, this, &ConfirmSave::rejected);
    QObject::connect(bDiscard, &Tui::ZButton::clicked, this, &ConfirmSave::discardSelected);
}
