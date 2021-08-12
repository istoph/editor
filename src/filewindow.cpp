#include "filewindow.h"

#include <unistd.h>

#include "confirmsave.h"


FileWindow::FileWindow(Tui::ZWidget *parent) : WindowWidget(parent) {
    setBorderEdges({ Qt::TopEdge });

    _file = new File(this);


    ScrollBar *sc = new ScrollBar(this);
    sc->setTransparent(true);
    connect(_file, &File::scrollPositionChanged, sc, &ScrollBar::scrollPosition);
    connect(_file, &File::textMax, sc, &ScrollBar::positonMax);

    _scrollbarHorizontal = new ScrollBar(this);
    connect(_file, &File::scrollPositionChanged, _scrollbarHorizontal, &ScrollBar::scrollPosition);
    connect(_file, &File::textMax, _scrollbarHorizontal, &ScrollBar::positonMax);
    _scrollbarHorizontal->setTransparent(true);

    _winLayout = new WindowLayout();
    setLayout(_winLayout);
    _winLayout->addRightBorderWidget(sc);
    _winLayout->setRightBorderTopAdjust(-1);
    _winLayout->setRightBorderBottomAdjust(-1);
    _winLayout->addBottomBorderWidget(_scrollbarHorizontal);
    _winLayout->setBottomBorderLeftAdjust(-1);
    _winLayout->addCentralWidget(_file);


    connect(_file, &File::modifiedChanged, this,
            [this] {
                QString filename = _file->getFilename();
                terminal()->setTitle("chr - "+ filename);
                terminal()->setIconTitle("chr - "+ filename);
                if(_file->isModified()) {
                    filename = "*" + filename;
                }
                setWindowTitle(filename);
            }
    );

    QObject::connect(new Tui::ZCommandNotifier("Wrap", this), &Tui::ZCommandNotifier::activated,
         [this] {
            setWrap(!_file->getWrapOption());
        }
    );

    //Save
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("s"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &FileWindow::saveOrSaveas);
    QObject::connect(new Tui::ZCommandNotifier("Save", this), &Tui::ZCommandNotifier::activated,
                    this, &FileWindow::saveOrSaveas);

    //Save As
    //shortcut dose not work in vte, konsole, ...
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("S", Qt::ControlModifier | Qt::ShiftModifier), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &FileWindow::saveFileDialog);
    QObject::connect(new Tui::ZCommandNotifier("SaveAs", this), &Tui::ZCommandNotifier::activated,
                     this, &FileWindow::saveFileDialog);

    //Reload
    QObject::connect(new Tui::ZCommandNotifier("Reload", this), &Tui::ZCommandNotifier::activated,
         this, [this] {
            if(_file->isModified()) {
                ConfirmSave *confirmDialog = new ConfirmSave(this, _file->getFilename(), ConfirmSave::Reload);
                QObject::connect(confirmDialog, &ConfirmSave::exitSelected, [=]{
                    delete confirmDialog;
                    FileWindow::reload();
                });

                QObject::connect(confirmDialog, &ConfirmSave::rejected, [=]{
                    delete confirmDialog;
                });
            } else {
                FileWindow::reload();
            }

    });

    //Edit
    QObject::connect(new Tui::ZCommandNotifier("Selectall", this), &Tui::ZCommandNotifier::activated,
         [this] {
            _file->selectAll();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Cutline", this), &Tui::ZCommandNotifier::activated,
         [this] {
            _file->cutline();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("SelectMode", this), &Tui::ZCommandNotifier::activated,
         [this] {
            _file->toggleSelectMode();
        }
    );


    // File follow and pipe
    _cmdInputPipe = new Tui::ZCommandNotifier("InputPipe", this);
    _cmdInputPipe->setEnabled(false);
    QObject::connect(_cmdInputPipe, &Tui::ZCommandNotifier::activated,
         [&] {
            closePipe();
        }
    );

    _cmdFollow = new Tui::ZCommandNotifier("Following", this);
    _cmdFollow->setEnabled(false);
    QObject::connect(_cmdFollow, &Tui::ZCommandNotifier::activated,
         [&] {
            setFollow(!getFollow());
        }
    );

    QObject::connect(this, &FileWindow::readFromStandadInput, this, [=](bool enable){
        _cmdInputPipe->setEnabled(enable);
        _cmdFollow->setEnabled(enable);
    });


    _watcher = new QFileSystemWatcher();
    QObject::connect(_watcher, &QFileSystemWatcher::fileChanged, this, [=]{
        fileChangedExternally(true);
    });
}

File *FileWindow::getFileWidget() {
    return _file;
}

void FileWindow::setWrap(bool wrap) {
    if (wrap) {
        _file->setWrapOption(true);
        _scrollbarHorizontal->setVisible(false);
        _winLayout->setRightBorderBottomAdjust(-2);
    } else {
        _file->setWrapOption(false);
        _scrollbarHorizontal->setVisible(true);
        _winLayout->setRightBorderBottomAdjust(-1);
    }
}

void FileWindow::saveFile(QString filename) {
    _file->setFilename(filename);
    watcherRemove();
    if(_file->saveText()) {
        //windowTitle(filename);
        update();
        fileChangedExternally(false);
    } else {
        Alert *e = new Alert(this);
        e->addPaletteClass("red");
        e->setWindowTitle("Error");
        e->setMarkup("file could not be saved"); //Die Datei konnte nicht gespeicher werden!
        e->setGeometry({15,5,50,5});
        e->setDefaultPlacement(Qt::AlignCenter);
        e->setVisible(true);
        e->setFocus();
    }
    watcherAdd();

}

SaveDialog *FileWindow::saveFileDialog() {
    SaveDialog * saveDialog = new SaveDialog(this->parentWidget(), _file);
    connect(saveDialog, &SaveDialog::fileSelected, this, &FileWindow::saveFile);
    return saveDialog;
}

SaveDialog *FileWindow::saveOrSaveas() {
    if (_file->isSaveAs()) {
        SaveDialog *q = saveFileDialog();
        return q;
    } else {
        saveFile(_file->getFilename());
        return nullptr;
    }
}

void FileWindow::newFile(QString filename) {
    closePipe();
    watcherRemove();
    _file->newText(filename);
    fileChangedExternally(false);
    watcherAdd();
}

void FileWindow::openFile(QString filename) {
    closePipe();
    watcherRemove();
    _file->openText(filename);
    fileChangedExternally(false);
    watcherAdd();
}

void FileWindow::reload() {
    QPoint xy = _file->getCursorPosition();

    _file->openText(_file->getFilename());
    fileChangedExternally(false);

    _file->setCursorPosition(xy);
}

void FileWindow::watcherAdd() {
    QFileInfo filenameInfo(_file->getFilename());
    if(filenameInfo.exists()) {
        _watcher->addPath(_file->getFilename());
    }
}
void FileWindow::watcherRemove() {
    if(_watcher->files().count() > 0) {
        _watcher->removePaths(_watcher->files());
    }
}


void FileWindow::closePipe() {
    if(_pipeSocketNotifier != nullptr && _pipeSocketNotifier->isEnabled()) {
        _pipeSocketNotifier->setEnabled(false);
        _pipeSocketNotifier->deleteLater();
        _pipeSocketNotifier = nullptr;
        close(0);
        readFromStandadInput(false);
    }
}

void FileWindow::watchPipe() {
    _pipeSocketNotifier = new QSocketNotifier(0, QSocketNotifier::Type::Read, this);
    QObject::connect(_pipeSocketNotifier, &QSocketNotifier::activated, this, &FileWindow::inputPipeReadable);
    _file->stdinText();
    readFromStandadInput(true);
}


void FileWindow::inputPipeReadable(int socket) {
    char buff[1024];
    int bytes = read(socket, buff, 1024);
    if (bytes == 0) {
        // EOF
        if (!_pipeLineBuffer.isEmpty()) {
            _file->appendLine(surrogate_escape::decode(_pipeLineBuffer));
        }
        _pipeSocketNotifier->deleteLater();
        _pipeSocketNotifier = nullptr;
    } else if (bytes < 0) {
        // TODO error handling
        _pipeSocketNotifier->deleteLater();
        _pipeSocketNotifier = nullptr;
    } else {
        _pipeLineBuffer.append(buff, bytes);
        int index;
        while ((index = _pipeLineBuffer.indexOf('\n')) != -1) {
            _file->appendLine(surrogate_escape::decode(_pipeLineBuffer.left(index)));
            _pipeLineBuffer = _pipeLineBuffer.mid(index + 1);
        }
        _file->modifiedChanged(true);
    }

    if(_pipeSocketNotifier == nullptr) {
        readFromStandadInput(false);
    } else {
        readFromStandadInput(true);
    }
}


void FileWindow::setFollow(bool follow) {
    _follow = follow;
    _file->followStandardInput(getFollow());
    followStandadInput(getFollow());
}

bool FileWindow::getFollow() {
    return _follow;
}

