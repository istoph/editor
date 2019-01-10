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

    setContentsMargins({ 1, 1, 0, 1});

    WindowLayout *wl = new WindowLayout();
    setLayout(wl);

    VBoxLayout *vbox = new VBoxLayout();

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
    wl->addCentral(vbox);

    int b = std::max(48,mainLable.length()+6);
    setGeometry({std::max(1, (80/2 - b / 2)), 3, b , 8});
    QObject::connect(bCancle, &Button::clicked, this, &ConfirmSave::rejected);
    QObject::connect(bDiscard, &Button::clicked, this, &ConfirmSave::exitSelected);
    QObject::connect(bSave, &Button::clicked, this, &ConfirmSave::saveSelected);

}
