#include "opendialog.h"

OpenDialog::OpenDialog(Tui::ZWidget *parent) : Dialog(parent) {

    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    setGeometry({20, 2, 40, 8});
    setWindowTitle("File Open");

    filenameText = new InputBox(this);
    filenameText->setGeometry({3,2,30,1});
    filenameText->setText("dokument.txt");
    filenameText->setFocus();

    okButton = new Button(this);
    okButton->setGeometry({25, 5, 8, 7});
    okButton->setText("OK");
    okButton->setDefault(true);

    connect(filenameText, &InputBox::textChanged, this, &OpenDialog::filenameChanged);

    QObject::connect(okButton, &Button::clicked, this, &OpenDialog::open);

    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    setGeometry(r);
    setVisible(true);
}

void OpenDialog::open() {
    // TODO: error message
    fileSelected(filenameText->text());
    deleteLater();
}

void OpenDialog::filenameChanged(QString filename) {
    QFileInfo datei(filename);
    okButton->setEnabled(datei.isReadable());
}
