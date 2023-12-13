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

    _currentPath = new Tui::ZLabel(this);
    _currentPath->setGeometry({2,2,45,1});

    if (!file->isNewFile()) {
        QFileInfo fileInfo(file->getFilename());
        _dir.setPath(fileInfo.path());
    }
    _model = std::make_unique<DlgFileModel>(_dir);

    _folder = new Tui::ZListView(this);
    _folder->setGeometry({3,3,44,6});
    _folder->setFocus();
    _folder->setModel(_model.get());

    _filenameText = new Tui::ZInputBox(this);
    _filenameText->setGeometry({3,10,44,1});

    if (!file->isNewFile()) {
        QFileInfo fileInfo(file->getFilename());
        _filenameText->setText(fileInfo.fileName());
    }

    _hiddenCheckBox = new Tui::ZCheckBox(Tui::withMarkup, "<m>h</m>idden", this);
    _hiddenCheckBox->setGeometry({3, 11, 13, 1});
    _model->setDisplayHidden(_hiddenCheckBox->checkState() == Qt::CheckState::Checked);

    _crlf = new Tui::ZCheckBox(Tui::withMarkup, "<m>C</m>RLF Mode", this);
    _crlf->setGeometry({3, 12, 16, 1});
    if (file->document()->crLfMode()) {
        _crlf->setCheckState(Qt::Checked);
    } else {
        _crlf->setCheckState(Qt::Unchecked);
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
    QObject::connect(_hiddenCheckBox, &Tui::ZCheckBox::stateChanged, this, [&] {
        _model->setDisplayHidden(_hiddenCheckBox->checkState() == Qt::CheckState::Checked);
    });
    QObject::connect(_cancelButton, &Tui::ZButton::clicked, this, &SaveDialog::rejected);
    QObject::connect(this, &Tui::ZDialog::rejected, this, &SaveDialog::rejected);
    QObject::connect(_saveButton, &Tui::ZButton::clicked, this, &SaveDialog::saveFile);

    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    setGeometry(r);

    refreshFolder();

    if (_filenameText->text().size()) {
        filenameChanged(_filenameText->text());
    }
}

void SaveDialog::saveFile() {
    QString fileName = _dir.absoluteFilePath(_filenameText->text());
    QFileInfo fileInfo(fileName);
    if (fileInfo.isDir()) {
        _previewDir.reset();
        _dir.setPath(fileName);
        refreshFolder();
        _filenameText->setText("");
        return;
    }
    if (_dir.exists(fileName)) {

        OverwriteDialog *overwriteDialog = new OverwriteDialog(parentWidget(), fileName);
        overwriteDialog->setFocus();
        QObject::connect(overwriteDialog, &OverwriteDialog::confirm, this,
                         [this, overwriteDialog, fileName](bool value) {
                            if (value) {
                                fileSelected(fileName, _crlf->checkState() == Qt::Checked);
                                deleteLater();
                            }
                            overwriteDialog->deleteLater();
                         }
        );
    } else {
        fileSelected(fileName, _crlf->checkState() == Qt::Checked);
        deleteLater();
    }
}

void SaveDialog::filenameChanged(QString filename) {
    if (filename.isEmpty()) {
        if (_previewDir) {
            _previewDir.reset();
            refreshFolder();
        }
        _saveButton->setText("Save");
        _saveButton->setEnabled(false);
        return;
    }

    if (!filename.contains('/')) {
        filename = _dir.absolutePath() + QString('/') + filename;
        if (_previewDir) {
            _previewDir.reset();
            refreshFolder();
        }
    } else {
        QFileInfo fileInfo(_dir.filePath(QFileInfo(filename).path()));
        if (fileInfo.isDir()) {
            _previewDir.emplace(fileInfo.absoluteFilePath());
            refreshFolder();
            filename = QFileInfo(_dir.filePath(filename)).absoluteFilePath();
        } else {
            _saveButton->setText("Save");
            _saveButton->setEnabled(false);
            return;
        }
    }

    // Loop to easyly catch symlink loops
    for (int cycle = 0; cycle < 100; cycle++) {
        QFileInfo fileInfo(filename);
        if (filename.isEmpty()) {
            _saveButton->setText("Save");
            _saveButton->setEnabled(false);
            return;
        } else if (fileInfo.isDir()) {
            _saveButton->setText("Open");
            _saveButton->setEnabled(true);
            return;
        } else if (fileInfo.isFile()) {
            // existing file
            if (fileInfo.isSymLink()) {
                filename = fileInfo.symLinkTarget();
                continue;
            } else if (fileInfo.isWritable()) {
                _saveButton->setText("Save");
                _saveButton->setEnabled(true);
                return;
            }
        } else {
            // to create a new file
            if (fileInfo.isSymLink()) {
                filename = fileInfo.symLinkTarget();
                continue;
            } else {
                QFileInfo parent(fileInfo.path());
                if (parent.isWritable()) {
                    _saveButton->setText("Save");
                    _saveButton->setEnabled(true);
                    return;
                }
            }
        }
        _saveButton->setText("Save");
        _saveButton->setEnabled(false);
        return;
    }

    // too many symlinks, bail out
    _saveButton->setText("Save");
    _saveButton->setEnabled(false);
}

void SaveDialog::refreshFolder() {
    if (_previewDir) {
        _model->setDirectory(*_previewDir);
        _currentPath->setText((*_previewDir).absolutePath().right(34) + " (preview)");
        _folder->setCurrentIndex(_model->index(0, 0));
    } else {
        _model->setDirectory(_dir);
        _currentPath->setText(_dir.absolutePath().right(44));
        _folder->setCurrentIndex(_model->index(0, 0));
    }
}

void SaveDialog::userInput(QString filename) {
    if (_previewDir) {
        _dir = *_previewDir;
        _previewDir.reset();
        _currentPath->setText(_dir.absolutePath().right(44));
    }

    const bool isDirectory = QFileInfo(_dir.filePath(filename)).isDir();
    if (isDirectory) {
        QString tmp = filename.left(filename.size() - 1);
        const bool isSymlink = QFileInfo(_dir.filePath(tmp)).isSymLink();
        if (isSymlink) {
            _dir.setPath(_dir.filePath(QFileInfo(_dir.filePath(tmp)).symLinkTarget()));
        } else {
            _dir.setPath(_dir.filePath(filename));
        }
        _dir.makeAbsolute();

        if (_filenameText->text().contains('/')) {
            _filenameText->setText(_filenameText->text().section('/', -1));
        } else {
            filenameChanged(_filenameText->text());
        }
        refreshFolder();
    } else {
        _filenameText->setText(filename);
        _filenameText->setFocus();
    }
}

void SaveDialog::rejected() {
    deleteLater();
}
