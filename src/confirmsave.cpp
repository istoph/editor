#include "confirmsave.h"

#include <Tui/ZButton.h>
#include <Tui/ZLabel.h>

ConfirmSave::ConfirmSave(Tui::ZWidget *parent, QString filename, Type type, bool saveable) : Dialog(parent) {
    QString title, nosave, save, mainLable;
    if(type == Close) {
        title = "Close";
        nosave = "Discard";
        save = "Save";
        mainLable = "Save: " + filename;
    } else if (type == Reload) {
        title = "Reload";
        nosave = "Discard and Reload";
        save = "Reload";
        saveable = false;
        mainLable = title + ": " + filename;
    } else {
        title = "Exit";
        nosave = "Quit";
        save = "Save and Quit";
        mainLable = "Unsaved: " + filename;
    }

    //Dialog Box
    setWindowTitle(title);
    //setMinimumSize(50,8);

    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    //Lable
    Tui::ZLabel *l1 = new Tui::ZLabel(this);
    l1->setText(mainLable);
    vbox->addWidget(l1);
    vbox->addStretch();

    HBoxLayout* hbox = new HBoxLayout();

    //Cancel
    Tui::ZButton *bCancel = new Tui::ZButton(this);
    if(!saveable) {
        bCancel->setDefault(!saveable);
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
    Tui::ZButton *bSave = new Tui::ZButton(this);
    bSave->setText(save);
    bSave->setEnabled(saveable);
    if (saveable) {
        bSave->setDefault(saveable);
        bSave->setFocus();
    }
    if (type != Reload) {
        hbox->addWidget(bSave);
    }
    vbox->add(hbox);

    QObject::connect(bCancel, &Tui::ZButton::clicked, this, &ConfirmSave::rejected);
    QObject::connect(bDiscard, &Tui::ZButton::clicked, this, &ConfirmSave::exitSelected);
    QObject::connect(bSave, &Tui::ZButton::clicked, this, &ConfirmSave::saveSelected);

}
