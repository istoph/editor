#include "quitdialog.h"

QuitDialog::QuitDialog(Tui::ZWidget *parent, QString filename) : Dialog(parent) {
    setGeometry({20, 2, 40, 8});
    setFocus();
    setWindowTitle("Save and Exit");

    Label *l1 = new Label(this);
    l1->setText("Save: " + filename);
    l1->setGeometry({2,2,35,1});

    Button *b1 = new Button(this);
    b1->setGeometry({2, 5, 10, 7});
    b1->setText("Exit");
    //b1->setDefault(true);

    Button *b2 = new Button(this);
    b2->setGeometry({18, 5, 19, 7});
    b2->setText("Save and Exit");
    b2->setDefault(true);

    QObject::connect(b1, &Button::clicked, this, &QuitDialog::exitSelected);
    QObject::connect(b2, &Button::clicked, this, &QuitDialog::saveSelected);

}
