#include "tabdialog.h"


TabDialog::TabDialog(File *file, Tui::ZWidget *parent) : Dialog(parent) {

    setFocus();
    setWindowTitle("Formatting Tab");
    //setPaletteClass({"window","cyan"});
    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    HBoxLayout* hbox1 = new HBoxLayout();

    RadioButton *r1 = new RadioButton(this);
    r1->setMarkup("<m>T</m>ab");
    // TODO: onSelect i1 und b1->setEnabled(false);
    r1->setChecked(!file->getTabOption());
    hbox1->addWidget(r1);

    RadioButton *r2 = new RadioButton(this);
    r2->setMarkup("<m>B</m>lank");
    r2->setChecked(file->getTabOption());
    hbox1->addWidget(r2);

    vbox->add(hbox1);
    vbox->addStretch();

    HBoxLayout* hbox2 = new HBoxLayout();

    Label *l1 = new Label(this);
    l1->setText("Tab Stops: ");
    hbox2->addWidget(l1);

    InputBox *i1 = new InputBox(this);
    i1->setText(QString::number(file->getTabsize()));
    i1->setFocus();
    i1->setEnabled(true);
    hbox2->addWidget(i1);

    vbox->add(hbox2);

    HBoxLayout* hbox3 = new HBoxLayout();

    Button *cancelButton = new Button(this);
    cancelButton->setText("Cancel");
    hbox3->addWidget(cancelButton);

    Button *okButton = new Button(this);
    okButton->setText("OK");
    okButton->setDefault(true);
    hbox3->addWidget(okButton);

    vbox->add(hbox3);

    QObject::connect(okButton, &Button::clicked, [=]{
        if(i1->text().toInt() > 0) {
            file->setTabOption(r2->checked());
            file->setTabsize(i1->text().toInt());
            deleteLater();
        }
        //TODO: error message
    });

    QObject::connect(cancelButton, &Button::clicked, [=]{
        deleteLater();
    });
}
