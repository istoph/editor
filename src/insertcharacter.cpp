#include "insertcharacter.h"

InsertCharacter::InsertCharacter(Tui::ZWidget *parent) : Dialog(parent) {
    setWindowTitle("Insert Character");
    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    HBoxLayout* hbox1 = new HBoxLayout();
    Label *hexLabel = new Label(this);
    hexLabel->setText("Hex:   0x");
    hbox1->addWidget(hexLabel);

    InputBox *hexInputBox = new InputBox(this);
    hexInputBox->setFocus();
    hbox1->addWidget(hexInputBox);

    vbox->add(hbox1);
    vbox->addStretch();

    HBoxLayout* hbox2 = new HBoxLayout();
    Label *intLabel = new Label(this);
    intLabel->setText("Int:     ");
    hbox2->addWidget(intLabel);

    InputBox *intInputBox = new InputBox(this);
    hbox2->addWidget(intInputBox);

    vbox->add(hbox2);
    vbox->addStretch();

    HBoxLayout* hbox3 = new HBoxLayout();
    //replace it with something that can also display all characters
    Label *previewLabel = new Label(this);
    previewLabel->setText("Preview: ");
    hbox3->addWidget(previewLabel);

    vbox->add(hbox3);

    HBoxLayout* hbox4 = new HBoxLayout();
    Button *cancelBtn = new Button(this);
    cancelBtn->setGeometry({3, 5, 7, 7});
    cancelBtn->setText("Cancel");
    hbox4->addWidget(cancelBtn);

    Button *insertButton = new Button(this);
    insertButton->setGeometry({15, 5, 7, 7});
    insertButton->setText("Insert");
    insertButton->setDefault(true);
    insertButton->setEnabled(false);
    hbox4->addWidget(insertButton);

    vbox->add(hbox4);

    QObject::connect(hexInputBox, &InputBox::textChanged, [=]{
        if(hexInputBox->focus()) {
            _codepoint = hexInputBox->text().toInt(&_check, 16);
            if(_check && _codepoint >= 0) {
                previewLabel->setText("Preview: "+ intToChar(_codepoint));
                intInputBox->setText(QString::number(_codepoint, 10).toUpper());
                cancelBtn->setDefault(false);
                insertButton->setDefault(true);
                insertButton->setEnabled(true);
            } else {
                previewLabel->setText("Preview: ERROR");
                insertButton->setEnabled(false);
                insertButton->setDefault(false);
                cancelBtn->setDefault(true);
            }
        }
    });

    QObject::connect(intInputBox, &InputBox::textChanged, [=]{
        if(intInputBox->focus()) {
            _codepoint = intInputBox->text().toInt(&_check, 10);
            if(_check && _codepoint >= 0) {
                previewLabel->setText("Preview: "+ intToChar(_codepoint));
                hexInputBox->setText(QString::number(_codepoint, 16).toUpper());
                cancelBtn->setDefault(false);
                insertButton->setDefault(true);
                insertButton->setEnabled(true);
            } else {
                previewLabel->setText("Preview: ERROR");
                insertButton->setEnabled(false);
                insertButton->setDefault(false);
                cancelBtn->setDefault(true);
            }
        }
    });


    QObject::connect(cancelBtn, &Button::clicked, this, &InsertCharacter::rejected);
    QObject::connect(insertButton, &Button::clicked, [this]{
        if(_check) {
            Q_EMIT characterSelected(intToChar(_codepoint));
        }
        deleteLater();
    });
}

void InsertCharacter::rejected() {
    deleteLater();
}

QString InsertCharacter::intToChar(int i) {
    if(i < 0 || i > 0x10FFFF) {
        return 0;
    } else if(i <= 0xFFFF) {
        return QString(1,QChar(i)).toUtf8();
    } else if (i <= 0x10FFFF){
        return (QString(1,QChar::highSurrogate(i))+QString(1,QChar::lowSurrogate(i))).toUtf8();
    }
    return 0;
}
