#include "tabdialog.h"


TabDialog::TabDialog(File *file, Tui::ZWidget *parent) : Dialog(parent) {

    setGeometry({20, 3, 40, 9});
    setFocus();
    setWindowTitle("Formatting Tab");
    //setPaletteClass({"window","cyan"});

    RadioButton *r1 = new RadioButton(this);
    r1->setGeometry({1, 2, 12, 1});
    r1->setMarkup("<m>T</m>ab");
    // TODO: onSelect i1 und b1->setEnabled(false);
    r1->setChecked(!file->getTabOption());

    RadioButton *r2 = new RadioButton(this);
    r2->setGeometry({12, 2, 12, 1});
    r2->setMarkup("<m>B</m>lank");
    r2->setChecked(file->getTabOption());

    Label *l1 = new Label(this);
    l1->setText("Tab Stops: ");
    l1->setGeometry({1,4,12,1});

    InputBox *i1 = new InputBox(this);
    i1->setText(QString::number(file->getTabsize()));
    i1->setGeometry({15,4,6,1});
    i1->setFocus();

    i1->setEnabled(true);

    Button *b1 = new Button(this);
    b1->setGeometry({15, 6, 6, 7});
    b1->setText("OK");
    b1->setDefault(true);

    QObject::connect(b1, &Button::clicked, [=]{
        if(i1->text().toInt() > 0) {
            file->setTabOption(r2->checked());
            file->setTabsize(i1->text().toInt());
            deleteLater();
        }
        //TODO: error message
    });

    Button *cancel = new Button(this);
    cancel->setText("Cancel");
    cancel->setGeometry({28,6,10,7});

    QObject::connect(cancel, &Button::clicked, [=]{
        deleteLater();
    });
}
