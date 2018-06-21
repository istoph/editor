#include "savedialog.h"

SaveDialog::SaveDialog(Tui::ZWidget *parent) : Dialog(parent) {

    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    setGeometry({20, 2, 40, 8});
    setWindowTitle("Save as...");

    filenameText = new InputBox(this);
    filenameText->setGeometry({3,2,30,1});
    //filenameText->setText(file.getFilename());
    filenameText->setFocus();

    okButton = new Button(this);
    okButton->setGeometry({25, 5, 8, 7});
    okButton->setText("OK");
    okButton->setDefault(true);

    connect(filenameText, &InputBox::textChanged, this, &SaveDialog::filenameChanged);
    QObject::connect(okButton, &Button::clicked, this, &SaveDialog::saveFile);

    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    setGeometry(r);
    setVisible(true);
}


void SaveDialog::saveFile() {
    fileSelected(filenameText->text());
    deleteLater();
//file->unsave
    //File::saveText()
}


void SaveDialog::filenameChanged(QString filename) {
    QFileInfo datei(filename);
    bool ok = true;
    if (datei.isDir()) {
        ok = false;
    } else if (!datei.dir().exists()) {
        ok = false;
    } else {
        if (datei.isFile()) {
            if (!datei.isWritable()) {
                ok = false;
            }
        } else {
            QFileInfo parent(datei.path());
            if(!parent.isWritable()) {
                ok = false;
            }
        }
    }
    okButton->setEnabled(ok);
}
