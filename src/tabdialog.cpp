#include "tabdialog.h"


TabDialog::TabDialog(File *file, Tui::ZWidget *parent) : Dialog(parent) {

    setFocus();
    setWindowTitle("Formatting Tab");
    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    HBoxLayout* hbox1 = new HBoxLayout();
    RadioButton *tabRadioButton = new RadioButton(this);
    tabRadioButton->setMarkup("<m>T</m>ab");
    tabRadioButton->setChecked(!file->getTabOption());
    hbox1->addWidget(tabRadioButton);

    RadioButton *blankRadioButton = new RadioButton(this);
    blankRadioButton->setMarkup("<m>B</m>lank");
    blankRadioButton->setChecked(file->getTabOption());
    hbox1->addWidget(blankRadioButton);

    vbox->add(hbox1);
    vbox->addStretch();

    HBoxLayout* hbox2 = new HBoxLayout();
    Label *tabstopLable = new Label(this);
    tabstopLable->setText("Tab Stops: ");
    hbox2->addWidget(tabstopLable);

    InputBox *tabsizeInputBox = new InputBox(this);
    tabsizeInputBox->setText(QString::number(file->getTabsize()));
    tabsizeInputBox->setFocus();
    tabsizeInputBox->setEnabled(true);
    hbox2->addWidget(tabsizeInputBox);
    vbox->add(hbox2);

    HBoxLayout* hbox3 = new HBoxLayout();
    Label *tabtospaceLabel = new Label(this);
    tabtospaceLabel->setText("Tab to Space: ");
    hbox3->addWidget(tabtospaceLabel);

    Button *convertButton = new Button(this);
    convertButton->setText("Convert");
    convertButton->setDefault(true);
    hbox3->addWidget(convertButton);
    vbox->add(hbox3);

    HBoxLayout* hbox4 = new HBoxLayout();
    Button *cancelButton = new Button(this);
    cancelButton->setText("Cancel");
    hbox4->addWidget(cancelButton);

    Button *saveButton = new Button(this);
    saveButton->setText("Save");
    saveButton->setDefault(true);
    hbox4->addWidget(saveButton);
    vbox->add(hbox4);

    QObject::connect(convertButton, &Button::clicked, [=]{
        file->setTabOption(blankRadioButton->checked());
        file->setTabsize(tabsizeInputBox->text().toInt());
        file->tabToSpace();
        deleteLater();
    });

    QObject::connect(saveButton, &Button::clicked, [=]{
        file->setTabOption(blankRadioButton->checked());
        file->setTabsize(tabsizeInputBox->text().toInt());
        deleteLater();
    });

    QObject::connect(tabsizeInputBox, &InputBox::textChanged, [=]{
        if(tabsizeInputBox->text().toInt() > 0 && tabsizeInputBox->text().toInt() < 100) {
            convertButton->setEnabled(true);
            saveButton->setEnabled(true);
        } else {
            convertButton->setEnabled(false);
            saveButton->setEnabled(false);
        }
    });

    QObject::connect(cancelButton, &Button::clicked, [=]{
        deleteLater();
    });
}
