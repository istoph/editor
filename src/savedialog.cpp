// SPDX-License-Identifier: BSL-1.0

#include "savedialog.h"

#include <Tui/ZTerminal.h>

#include "overwritedialog.h"


SaveDialog::SaveDialog(Tui::ZWidget *parent, File *file) : Tui::ZDialog(parent) {
    setDefaultPlacement(Qt::AlignCenter);
    setGeometry({0, 0, 50, 15});
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose
               | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setWindowTitle("Save as...");
    setContentsMargins({ 1, 1, 2, 1});

    _curentPath = new Tui::ZLabel(this);
    _curentPath->setGeometry({2,2,45,1});

    _model = std::make_unique<DlgFileModel>(_dir);

    _folder = new Tui::ZListView(this);
    _folder->setGeometry({3,3,44,6});
    _folder->setFocus();
    _folder->setModel(_model.get());

    _filenameText = new Tui::ZInputBox(this);
    _filenameText->setGeometry({3,10,44,1});

    _hiddenCheckBox = new Tui::ZCheckBox(Tui::withMarkup, "<m>h</m>idden", this);
    _hiddenCheckBox->setGeometry({3, 11, 13, 1});
    _model->setDisplayHidden(_hiddenCheckBox->checkState() == Qt::CheckState::Checked);

    _dos = new Tui::ZCheckBox(Tui::withMarkup, "<m>D</m>OS Mode", this);
    _dos->setGeometry({3, 12, 16, 1});
    if(file->document()->crLfMode()) {
        _dos->setCheckState(Qt::Checked);
    } else {
        _dos->setCheckState(Qt::Unchecked);
    }

    _cancelButton = new Tui::ZButton(this);
    _cancelButton->setGeometry({25, 12, 12, 1});
    _cancelButton->setText("Cancel");

    _saveButton = new Tui::ZButton(this);
    _saveButton->setGeometry({37, 12, 10, 1});
    _saveButton->setText("Save");
    _saveButton->setDefault(true);
    _saveButton->setEnabled(false);

    QObject::connect(_folder, &Tui::ZListView::enterPressed, [this](int selected){
        (void)selected;
        userInput(_folder->currentItem());
    });
    QObject::connect(_filenameText, &Tui::ZInputBox::textChanged, this, &SaveDialog::filenameChanged);
    QObject::connect(_hiddenCheckBox, &Tui::ZCheckBox::stateChanged, this, [&]{
        _model->setDisplayHidden(_hiddenCheckBox->checkState() == Qt::CheckState::Checked);
    });
    QObject::connect(_dos, &Tui::ZCheckBox::stateChanged, [=] {
       if(_dos->checkState() == Qt::Checked) {
            file->document()->setCrLfMode(true);
       } else {
            file->document()->setCrLfMode(false);
       }
    });
    QObject::connect(_cancelButton, &Tui::ZButton::clicked, this, &SaveDialog::rejected);
    QObject::connect(this, &Tui::ZDialog::rejected, this, &SaveDialog::rejected);
    QObject::connect(_saveButton, &Tui::ZButton::clicked, this, &SaveDialog::saveFile);

    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    setGeometry(r);

    refreshFolder();
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
    bool isSaveable = false;
    isSaveable = _filenameChanged(filename, false);
    _saveButton->setEnabled(isSaveable);
}

bool SaveDialog::_filenameChanged(QString filename, bool absolutePath) {
    if (!absolutePath) {
        QRegExp regex("[\/]");
        if (regex.indexIn(filename) < 0) {
            filename = _dir.absolutePath() + QString('/') + filename;
        } else {
            return false;
        }
    }
    QFileInfo datei(filename);
    if (filename.isEmpty()) {
        return false;
    } else if (datei.isDir()) {
        return false;
    } else if (datei.isFile()) {
        if (datei.isSymLink()) {
            return _filenameChanged(QString(datei.symLinkTarget()), true);
        } else if (datei.isWritable()) {
            return true;
        }
    } else {
        if (datei.isSymLink()) {
            return _filenameChanged(QString(datei.symLinkTarget()), true);
        } else {
            QFileInfo parent(datei.path());
            if(parent.isWritable()) {
                return true;
            }
        }
    }
    return false;
}

void SaveDialog::refreshFolder() {
    _model->setDirectory(_dir);
    _curentPath->setText(_dir.absolutePath().right(44));
    _folder->setCurrentIndex(_model->index(0, 0));
}

void SaveDialog::userInput(QString filename) {
    QString tmp = filename.left(filename.size()-1);
    if(QFileInfo(_dir.filePath(filename)).isDir() && QFileInfo(_dir.filePath(tmp)).isSymLink()) {
       _dir.setPath(_dir.filePath(QFileInfo(_dir.filePath(tmp)).symLinkTarget()));
       _dir.makeAbsolute();
       refreshFolder();
    } else if(QFileInfo(_dir.filePath(filename)).isDir()) {
        _dir.setPath(_dir.filePath(filename));
        _dir.makeAbsolute();
        refreshFolder();
    } else {
        _filenameText->setText(filename);
        _filenameText->setFocus();
    }
}

void SaveDialog::rejected() {
    deleteLater();
}
