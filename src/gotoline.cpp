#include "gotoline.h"

#include <Tui/ZButton.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>

GotoLine::GotoLine(Tui::ZWidget *parent) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose
               | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setWindowTitle("Goto Line");
    setContentsMargins({ 1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    Tui::ZHBoxLayout *hbox1 = new Tui::ZHBoxLayout();
    Tui::ZLabel *labelGoto = new Tui::ZLabel(this);
    labelGoto->setText("Goto line: ");
    hbox1->addWidget(labelGoto);

    Tui::ZInputBox *inputboxGoto = new Tui::ZInputBox(this);
    inputboxGoto->setFocus();
    hbox1->addWidget(inputboxGoto);

    vbox->add(hbox1);
    vbox->addStretch();

    Tui::ZHBoxLayout *hbox2 = new Tui::ZHBoxLayout();
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
