#include "tabdialog.h"


TabDialog::TabDialog(Tui::ZWidget *parent) : Dialog(parent) {

    setFocus();
    setWindowTitle("Formatting Tab");
    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    HBoxLayout* hbox1 = new HBoxLayout();
    _tabRadioButton = new RadioButton(this);
    _tabRadioButton->setMarkup("<m>T</m>ab");
    hbox1->addWidget(_tabRadioButton);

    _blankRadioButton = new RadioButton(this);
    _blankRadioButton->setMarkup("<m>B</m>lank");
    hbox1->addWidget(_blankRadioButton);

    vbox->add(hbox1);
    vbox->addStretch();

    HBoxLayout* hbox2 = new HBoxLayout();
    Label *tabstopLable = new Label(this);
    tabstopLable->setText("Tab Stops: ");
    hbox2->addWidget(tabstopLable);

    _tabsizeInputBox = new InputBox(this);
    _tabsizeInputBox->setFocus();
    _tabsizeInputBox->setEnabled(true);
    hbox2->addWidget(_tabsizeInputBox);
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

    QObject::connect(convertButton, &Button::clicked, [this] {
        convert(!_blankRadioButton->checked(), _tabsizeInputBox->text().toInt());
        deleteLater();
    });

    QObject::connect(saveButton, &Button::clicked, [this] {
        Q_EMIT settingsChanged(!_blankRadioButton->checked(), _tabsizeInputBox->text().toInt());
        deleteLater();
    });

    QObject::connect(_tabsizeInputBox, &InputBox::textChanged, [this, convertButton, saveButton] {
        if(_tabsizeInputBox->text().toInt() > 0 && _tabsizeInputBox->text().toInt() < 100) {
            convertButton->setEnabled(true);
            saveButton->setEnabled(true);
        } else {
            convertButton->setEnabled(false);
            saveButton->setEnabled(false);
        }
    });

    QObject::connect(cancelButton, &Button::clicked, [this] {
        deleteLater();
    });
}

void TabDialog::updateSettings(bool useTabs, int indentSize) {
    _tabRadioButton->setChecked(useTabs);
    _blankRadioButton->setChecked(!useTabs);
    _tabsizeInputBox->setText(QString::number(indentSize));
}
