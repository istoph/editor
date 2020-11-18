#include "opendialog.h"

void OpenDialog::refreshFolder()
{
    QStringList items;
    _dir.setSorting(QDir::DirsFirst | QDir::Name);
    QFileInfoList list = _dir.entryInfoList();
    _dir.setFilter(QDir::AllEntries);

    //TODO: das abschneiden muss das Lable machen ;)
    _curentPath->setText(_dir.absolutePath().right(33));
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
    if(path.size()) {
        _dir.setPath(path);
    }
    _dir.makeAbsolute();

    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    setGeometry({20, 2, 40, 14});
    setWindowTitle("File Open");

    _curentPath = new Label(this);
    _curentPath->setGeometry({3,1,34,1});

    _folder = new ListView(this);
    _folder->setGeometry({3,2,34,8});
    _folder->setFocus();

    connect(_folder, &ListView::enterPressed, [this](int selected){
        userInput(_folder->items()[selected]);
    });
    refreshFolder();

    _okButton = new Button(this);
    _okButton->setGeometry({27, 11, 10, 7});
    _okButton->setText("Open");
    _okButton->setDefault(true);

    _cancelButton = new Button(this);
    _cancelButton->setGeometry({14, 11, 12, 7});
    _cancelButton->setText("Cancel");


    //connect(filenameText, &InputBox::textChanged, this, &OpenDialog::filenameChanged);

    QObject::connect(_okButton, &Button::clicked, this, &OpenDialog::open);
    QObject::connect(_cancelButton, &Button::clicked, this, &OpenDialog::rejected);

    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    setGeometry(r);
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
