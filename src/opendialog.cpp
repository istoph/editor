#include "opendialog.h"

void OpenDialog::refreshFolder() {
    QStringList items;
    QFileInfoList list; // = _dir.entryInfoList();
    if(_hiddenCheckBox->checkState() == Qt::CheckState::Unchecked) {
        _dir.setFilter(QDir::AllEntries);
    } else {
        _dir.setFilter(QDir::AllEntries | QDir::Hidden);
    }
    _dir.setSorting(QDir::DirsFirst |QDir::Name);
    list = _dir.entryInfoList();

    _curentPath->setText(_dir.absolutePath());
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if(fileInfo.fileName() != ".") {
            if(fileInfo.fileName() == "..") {
                if(_dir.path() != "/") {
                    items.insert(0,"../");
                }
            } else if(fileInfo.isDir()) {
                items.append(fileInfo.fileName()+"/");
            } else {
                items.append(fileInfo.fileName());
            }
        }
    }
    _folder->setItems(items);
}

OpenDialog::OpenDialog(Tui::ZWidget *parent, QString path) : Dialog(parent) {
    setDefaultPlacement(Qt::AlignCenter);
    if(path.size()) {
        _dir.setPath(path);
    }
    _dir.makeAbsolute();

    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    setGeometry({20, 2, 42, 14});
    setWindowTitle("File Open");

    _curentPath = new Tui::ZLabel(this);
    _curentPath->setGeometry({3,1,36,1});

    _folder = new ListView(this);
    _folder->setGeometry({3,2,36,8});
    _folder->setFocus();

    _hiddenCheckBox = new Tui::ZCheckBox(Tui::withMarkup, "<m>h</m>idden", this);
    _hiddenCheckBox->setGeometry({3, 11, 13, 1});

    _cancelButton = new Tui::ZButton(this);
    _cancelButton->setGeometry({16, 11, 12, 1});
    _cancelButton->setText("Cancel");

    _okButton = new Tui::ZButton(this);
    _okButton->setGeometry({29, 11, 10, 1});
    _okButton->setText("Open");
    _okButton->setDefault(true);

    QObject::connect(_folder, &ListView::enterPressed, [this](int selected){
        (void)selected;
        userInput(_folder->currentItem());
    });
    QObject::connect(_hiddenCheckBox, &Tui::ZCheckBox::stateChanged, this, [&]{
        refreshFolder();
    });
    QObject::connect(_okButton, &Tui::ZButton::clicked, [this] {
        if(_folder->currentItem().right(1) == "/") {
            userInput(_folder->currentItem());
        } else {
            open();
        }
    });
    QObject::connect(_cancelButton, &Tui::ZButton::clicked, this, &OpenDialog::rejected);

    refreshFolder();
    setVisible(true);
}

void OpenDialog::open() {
    // TODO: error message
    //fileSelected(filenameText->text());
    fileSelected(_folder->currentItem());
    deleteLater();
}

void OpenDialog::filenameChanged(QString filename) {
    QFileInfo datei(filename);
    _okButton->setEnabled(datei.isReadable());
}

void OpenDialog::pathSelected(QString path) {
    _dir.setPath(path);
}

void OpenDialog::userInput(QString filename) {
    QString tmp = filename.left(filename.size()-1);
    if(QFileInfo(_dir.filePath(filename)).isDir() && QFileInfo(_dir.filePath(tmp)).isSymLink()) {
       _dir.setPath(_dir.filePath(QFileInfo(_dir.filePath(tmp)).readLink()));
       _dir.makeAbsolute();
       refreshFolder();
    } else if(QFileInfo(_dir.filePath(filename)).isDir()) {
        _dir.setPath(_dir.filePath(filename));
        _dir.makeAbsolute();
        refreshFolder();
    } else {
        fileSelected(_dir.filePath(filename));
        deleteLater();
    }
}

void OpenDialog::rejected() {
    deleteLater();
}
