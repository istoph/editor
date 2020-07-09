#include "savedialog.h"

void SaveDialog::refreshFolder() {
    QStringList items;
    QFileInfoList list = _dir.entryInfoList();
    _dir.setFilter(QDir::AllEntries);
    _curentPath->setText(_dir.absolutePath().right(44));
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
    _folder->setItems(items);
}

SaveDialog::SaveDialog(Tui::ZWidget *parent) : Dialog(parent) {

    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    setGeometry({10, 2, 50, 15});
    setWindowTitle("Save as...");

    _curentPath = new Label(this);
    _curentPath->setGeometry({2,2,45,1});
    _folder = new ListView(this);
    _folder->setGeometry({3,3,44,6});
    _folder->setFocus();

    connect(_folder, &ListView::enterPressed, [this](int selected){
        userInput(_folder->items()[selected]);
    });
    refreshFolder();

    _filenameText = new InputBox(this);
    _filenameText->setGeometry({3,10,44,1});
    _cancleButton = new Button(this);
    _cancleButton->setGeometry({25, 12, 12, 7});
    _cancleButton->setText("Cancle");

    _okButton = new Button(this);
    _okButton->setGeometry({37, 12, 10, 7});
    _okButton->setText("Save");
    _okButton->setDefault(true);
    _okButton->setEnabled(false);

    QObject::connect(_filenameText, &InputBox::textChanged, this, &SaveDialog::filenameChanged);
    QObject::connect(_cancleButton, &Button::clicked, this, &SaveDialog::rejected);
    QObject::connect(_okButton, &Button::clicked, this, &SaveDialog::saveFile);

    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    setGeometry(r);
    setVisible(true);
}

void SaveDialog::saveFile() {
    fileSelected(_dir.absolutePath() + "/" + _filenameText->text());
    deleteLater();
}

void SaveDialog::filenameChanged(QString filename) {
    QFileInfo datei(filename);
    bool ok = true;
    if (filename.isEmpty()) {
        ok = false;
    } else if (datei.isDir()) {
        _dir.setPath(_dir.filePath(filename));
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
    _okButton->setEnabled(ok);
}

void SaveDialog::userInput(QString filename) {
    if(QFileInfo(_dir.filePath(filename)).isDir()) {
        _dir.setPath(_dir.filePath(filename));
        refreshFolder();
    } else {
        _filenameText->setText(filename);
        _filenameText->setFocus();
    }
}
