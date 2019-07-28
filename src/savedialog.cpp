#include "savedialog.h"

void SaveDialog::refreshFolder() {
    QStringList items;
    QFileInfoList list = dir.entryInfoList();
    dir.setFilter(QDir::AllEntries);
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if(fileInfo.fileName() != ".") {
            if(fileInfo.isDir()) {
                items.append(fileInfo.fileName()+"/");
            } else {
                items.append(fileInfo.fileName());
            }
        }
    }
    folder->setItems(items);
}

SaveDialog::SaveDialog(Tui::ZWidget *parent) : Dialog(parent) {

    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    setGeometry({20, 2, 40, 14});
    setWindowTitle("Save as...");

    folder = new ListView(this);
    folder->setGeometry({3,2,34,6});
    folder->setFocus();

    connect(folder, &ListView::enterPressed, [this](int selected){
        userInput(folder->items()[selected]);
    });
    refreshFolder();

    filenameText = new InputBox(this);
    filenameText->setGeometry({3,9,34,1});
    //filenameText->setText(file.getFilename());
    //filenameText->setFocus();

    okButton = new Button(this);
    okButton->setGeometry({29, 11, 8, 7});
    okButton->setText("OK");
    okButton->setDefault(true);
    okButton->setEnabled(false);

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
    if (filename.isEmpty()) {
        ok = false;
    } else if (datei.isDir()) {
        dir.setPath(dir.filePath(filename));
        refreshFolder();
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

void SaveDialog::userInput(QString filename) {
    if(QFileInfo(dir.filePath(filename)).isDir()) {
        dir.setPath(dir.filePath(filename));
        refreshFolder();
    } else {
        filenameText->setText(filename);
        filenameText->setFocus();
    }
}
