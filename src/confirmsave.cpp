#include "confirmsave.h"

ConfirmSave::ConfirmSave(Tui::ZWidget *parent, QString filename, Type type, bool saveable) : Dialog(parent) {
    QString mainLable = "Save: " + filename;

    QString title, nosave, save;
    if(type == Close) {
        title = "Close";
        nosave = "Discard";
        save = "Save";
    } else if (type == Reload) {
        title = "Reload";
        nosave = "Discard and Reload";
        save = "Reload";
        saveable = false;
    } else {
        title = "Exit";
        nosave = "Quit";
        save = "Save and Quit";
    }
    //Dialog Box
    setWindowTitle(title);
    //setMinimumSize(50,8);

    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    //Lable
    Label *l1 = new Label(this);
    l1->setText("Save: " + filename);
    vbox->addWidget(l1);
    vbox->addStretch();

    HBoxLayout* hbox = new HBoxLayout();

    //Cancel
    Button *bCancel = new Button(this);
    if(!saveable) {
        bCancel->setDefault(!saveable);
        bCancel->setFocus();
    }
    bCancel->setText("Cancel");
    hbox->addWidget(bCancel);

    //Discard
    Button *bDiscard = new Button(this);
    bDiscard->setText(nosave);
    hbox->addWidget(bDiscard);
    hbox->setSpacing(1);

    //Save
    Button *bSave = new Button(this);
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

    QObject::connect(bCancel, &Button::clicked, this, &ConfirmSave::rejected);
    QObject::connect(bDiscard, &Button::clicked, this, &ConfirmSave::exitSelected);
    QObject::connect(bSave, &Button::clicked, this, &ConfirmSave::saveSelected);

}
