// SPDX-License-Identifier: BSL-1.0

#include "opendialog.h"

#include <QRect>


OpenDialog::OpenDialog(Tui::ZWidget *parent, QString path) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose
               | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setDefaultPlacement(Qt::AlignCenter);
    setGeometry({0, 0, 50, 15});
    setWindowTitle("Open File");
    setContentsMargins({ 1, 1, 2, 1});

    if(path.size()) {
        QFileInfo fi(path);
        if (fi.isDir() && path.right(1) != "/") {
            fi.setFile(path + "/");
        }
        _dir.setPath(fi.absolutePath());
    }
    _dir.makeAbsolute();

    _curentPath = new Tui::ZLabel(this);
    _curentPath->setGeometry({2,2,45,1});

    _hiddenCheckBox = new Tui::ZCheckBox(Tui::withMarkup, "<m>h</m>idden", this);
    _hiddenCheckBox->setGeometry({3, 12, 13, 1});

    _model = std::make_unique<DlgFileModel>(_dir);
    _model->setDisplayHidden(_hiddenCheckBox->checkState() == Qt::CheckState::Checked);

    _folder = new Tui::ZListView(this);
    _folder->setGeometry({3,3,44,8});
    _folder->setFocus();
    _folder->setModel(_model.get());

    _cancelButton = new Tui::ZButton(this);
    _cancelButton->setGeometry({25, 12, 12, 1});
    _cancelButton->setText("Cancel");

    _okButton = new Tui::ZButton(this);
    _okButton->setGeometry({37, 12, 10, 1});
    _okButton->setText("Open");
    _okButton->setDefault(true);


    QObject::connect(_folder->selectionModel(), &QItemSelectionModel::selectionChanged, [this] {
        filenameChanged(_folder->currentItem());
    });
    QObject::connect(_folder, &Tui::ZListView::enterPressed, [this](int selected){
        (void)selected;
        if (_okButton->isEnabled()) {
            userInput(_folder->currentItem());
        }
    });
    QObject::connect(_hiddenCheckBox, &Tui::ZCheckBox::stateChanged, this, [&]{
        _model->setDisplayHidden(_hiddenCheckBox->checkState() == Qt::CheckState::Checked);
    });
    QObject::connect(_okButton, &Tui::ZButton::clicked, [this] {
        userInput(_folder->currentItem());
    });
    QObject::connect(_cancelButton, &Tui::ZButton::clicked, this, &OpenDialog::rejected);

    refreshFolder();
}

void OpenDialog::filenameChanged(QString filename) {
    QFileInfo datei(_dir.path() + "/" + filename);
    _okButton->setEnabled( datei.isReadable() );
}

void OpenDialog::refreshFolder() {
    _model->setDirectory(_dir);
    _curentPath->setText(_dir.absolutePath().right(44));
    _folder->setCurrentIndex(_model->index(0, 0));
}

void OpenDialog::userInput(QString filename) {
    if(QFileInfo(_dir.filePath(filename)).isDir()) {
        if (QFileInfo(_dir.filePath(filename)).isSymLink()) {
            _dir.setPath(_dir.filePath(QFileInfo(_dir.filePath(filename)).readLink()));
        } else {
            _dir.setPath(_dir.filePath(filename));
        }
        _dir.makeAbsolute();
        refreshFolder();
    } else {
        fileSelected(_dir.filePath(filename));
        rejected();
    }
}

void OpenDialog::rejected() {
    deleteLater();
}
