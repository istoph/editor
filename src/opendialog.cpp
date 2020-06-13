#include "opendialog.h"

void OpenDialog::refreshFolder()
{
    QStringList items;
    QFileInfoList list = _dir.entryInfoList();
    _dir.setFilter(QDir::AllEntries);

    //TODO: das abschneiden muss das Lable machen ;)
    curentPath->setText(_dir.absolutePath().right(33));
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if(fileInfo.fileName() != ".") {
            if(fileInfo.fileName() == "..") {
                if(_dir.path() != "/") {
                    items.append("../");
                }
            } else if(fileInfo.isDir()) {
                items.append(fileInfo.fileName()+"/");
            } else {
                items.append(fileInfo.fileName());
            }
        }
    }
    folder->setItems(items);
}

OpenDialog::OpenDialog(Tui::ZWidget *parent) : Dialog(parent) {

    _dir.makeAbsolute();

    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    setGeometry({20, 2, 40, 14});
    setWindowTitle("File Open");

    curentPath = new Label(this);
    curentPath->setGeometry({3,1,34,1});

    folder = new ListView(this);
    folder->setGeometry({3,2,34,8});
    folder->setFocus();

    connect(folder, &ListView::enterPressed, [this](int selected){
        userInput(folder->items()[selected]);
    });
    refreshFolder();

    /*
    filenameText = new InputBox(this);
    filenameText->setGeometry({3,2,30,1});
    filenameText->setText("dokument.txt");
    filenameText->setFocus();
    */

    okButton = new Button(this);
    okButton->setGeometry({28, 11, 8, 7});
    okButton->setText("OK");
    okButton->setDefault(true);

    //connect(filenameText, &InputBox::textChanged, this, &OpenDialog::filenameChanged);

    QObject::connect(okButton, &Button::clicked, this, &OpenDialog::open);

    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    setGeometry(r);
    setVisible(true);
}

void OpenDialog::open() {
    // TODO: error message
    //fileSelected(filenameText->text());
    fileSelected(folder->currentItem());
    deleteLater();
}

void OpenDialog::filenameChanged(QString filename) {
    QFileInfo datei(filename);
    okButton->setEnabled(datei.isReadable());
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
