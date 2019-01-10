#include "confirmsave.h"

ConfirmSave::ConfirmSave(Tui::ZWidget *parent, QString filename, Type type) : Dialog(parent) {
    QString mainLable = "Save: " + filename;

    QString title, nosave, save;
    if(type == New) {
        title = "Save";
        nosave = "Discard";
        save = "Save";
    } else if (type == Open) {
        title = "Save";
        nosave = "Discard";
        save = "Save";
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

    //Cancle
    Button *bCancle = new Button(this);
    bCancle->setText("Cancle");
    hbox->addWidget(bCancle);

    //Discard
    Button *bDiscard = new Button(this);
    bDiscard->setText(nosave);
    hbox->addWidget(bDiscard);
    hbox->setSpacing(1);

    //Save
    Button *bSave = new Button(this);
    bSave->setText(save);
    bSave->setDefault(true);
    bSave->setFocus();
    hbox->addWidget(bSave);
    vbox->add(hbox);

    QObject::connect(bCancle, &Button::clicked, this, &ConfirmSave::rejected);
    QObject::connect(bDiscard, &Button::clicked, this, &ConfirmSave::exitSelected);
    QObject::connect(bSave, &Button::clicked, this, &ConfirmSave::saveSelected);

}
