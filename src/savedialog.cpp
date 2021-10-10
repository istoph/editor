#include "savedialog.h"

void SaveDialog::refreshFolder() {
    QStringList items;
    QFileInfoList list;
    if(_hiddenCheckBox->checkState() == Qt::CheckState::Unchecked) {
        _dir.setFilter(QDir::AllEntries);
    } else {
        _dir.setFilter(QDir::AllEntries | QDir::Hidden);
    }
    _dir.setSorting(QDir::DirsFirst | QDir::Name);
    list = _dir.entryInfoList();

    _curentPath->setText(_dir.absolutePath().right(44));
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

SaveDialog::SaveDialog(Tui::ZWidget *parent, File *file) : Dialog(parent) {
    setDefaultPlacement(Qt::AlignCenter);
    setGeometry({0, 0, 50, 15});
    setWindowTitle("Save as...");
    setContentsMargins({ 1, 1, 2, 1});

    _curentPath = new Tui::ZLabel(this);
    _curentPath->setGeometry({2,2,45,1});

    _folder = new ListView(this);
    _folder->setGeometry({3,3,44,6});
    _folder->setFocus();

    _filenameText = new InputBox(this);
    _filenameText->setGeometry({3,10,44,1});

    _hiddenCheckBox = new Tui::ZCheckBox(Tui::withMarkup, "<m>h</m>idden", this);
    _hiddenCheckBox->setGeometry({3, 11, 13, 1});

    _dos = new Tui::ZCheckBox(Tui::withMarkup, "<m>D</m>OS Mode", this);
    _dos->setGeometry({3, 12, 16, 1});
    if(file->getMsDosMode()) {
        _dos->setCheckState(Qt::Checked);
    } else {
        _dos->setCheckState(Qt::Unchecked);
    }

    _cancelButton = new Tui::ZButton(this);
    _cancelButton->setGeometry({25, 12, 12, 1});
    _cancelButton->setText("Cancel");

    _okButton = new Tui::ZButton(this);
    _okButton->setGeometry({37, 12, 10, 1});
    _okButton->setText("Save");
    _okButton->setDefault(true);
    _okButton->setEnabled(false);


    QObject::connect(_folder, &ListView::enterPressed, [this](int selected){
        (void)selected;
        userInput(_folder->currentItem());
    });
    QObject::connect(_filenameText, &InputBox::textChanged, this, &SaveDialog::filenameChanged);
    QObject::connect(_hiddenCheckBox, &Tui::ZCheckBox::stateChanged, this, [&]{
        refreshFolder();
    });
    QObject::connect(_dos, &Tui::ZCheckBox::stateChanged, [=] {
       if(_dos->checkState() == Qt::Checked) {
            file->setMsDosMode(true);
       } else {
            file->setMsDosMode(false);
       }
    });
    QObject::connect(_cancelButton, &Tui::ZButton::clicked, this, &SaveDialog::rejected);
    QObject::connect(_okButton, &Tui::ZButton::clicked, this, &SaveDialog::saveFile);

    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    setGeometry(r);

    refreshFolder();
}

void SaveDialog::rejected() {
    deleteLater();
}

void SaveDialog::saveFile() {
    QString fileName = _dir.absoluteFilePath(_filenameText->text());
    if(_dir.exists(fileName)) {

        OverwriteDialog *overwriteDialog = new OverwriteDialog(parentWidget(), fileName);
        overwriteDialog->setFocus();
        QObject::connect(overwriteDialog, &OverwriteDialog::confirm, this,
                         [this, overwriteDialog, fileName](bool value) {
                            if (value) {
                                fileSelected(fileName);
                                deleteLater();
                            }
                            overwriteDialog->deleteLater();
                         }
        );
    } else {
        fileSelected(fileName);
        deleteLater();
    }
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
    if(filename == "../") {
        _dir.cdUp();
        refreshFolder();
    } else if(QFileInfo(_dir.filePath(filename)).isDir()) {
        _dir.setPath(_dir.filePath(filename));
        refreshFolder();
    } else {
        _filenameText->setText(filename);
        _filenameText->setFocus();
    }
}
