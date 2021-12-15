#include "gotoline.h"

#include <Tui/ZButton.h>
#include <Tui/ZLabel.h>

GotoLine::GotoLine(Tui::ZWidget *parent) : Dialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose
               | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setWindowTitle("Goto Line");
    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    HBoxLayout *hbox1 = new HBoxLayout();
    Tui::ZLabel *labelGoto = new Tui::ZLabel(this);
    labelGoto->setText("Goto line: ");
    hbox1->addWidget(labelGoto);

    InputBox *inputboxGoto = new InputBox(this);
    inputboxGoto->setFocus();
    hbox1->addWidget(inputboxGoto);

    vbox->add(hbox1);
    vbox->addStretch();

    HBoxLayout *hbox2 = new HBoxLayout();
    Tui::ZButton *buttonCancel = new Tui::ZButton(this);
    buttonCancel->setGeometry({3, 5, 7, 7});
    buttonCancel->setText("Cancel");
    hbox2->addWidget(buttonCancel);

    Tui::ZButton *buttonOK = new Tui::ZButton(this);
    buttonOK->setGeometry({15, 5, 7, 7});
    buttonOK->setText("OK");
    buttonOK->setDefault(true);
    hbox2->addWidget(buttonOK);

    vbox->add(hbox2);

    QObject::connect(buttonCancel, &Tui::ZButton::clicked, this, &GotoLine::rejected);
    QObject::connect(buttonOK, &Tui::ZButton::clicked, this, [this, inputboxGoto]{
        lineSelected(inputboxGoto->text());
        this->deleteLater();
    });
}
