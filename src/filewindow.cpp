#include "filewindow.h"

#include <unistd.h>

#include "confirmsave.h"


FileWindow::FileWindow(Tui::ZWidget *parent) : Tui::ZWindow(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose
               | Tui::ZWindow::MoveOption | Tui::ZWindow::ResizeOption
               | Tui::ZWindow::AutomaticOption);
    setBorderEdges({ Qt::TopEdge });

    _file = new File(this);

    _scrollbarVertical = new ScrollBar(this);
    _scrollbarVertical->setTransparent(true);
    QObject::connect(_file, &File::scrollPositionChanged, _scrollbarVertical, &ScrollBar::scrollPosition);
    QObject::connect(_file, &File::textMax, _scrollbarVertical, &ScrollBar::positonMax);

    _scrollbarHorizontal = new ScrollBar(this);
    QObject::connect(_file, &File::scrollPositionChanged, _scrollbarHorizontal, &ScrollBar::scrollPosition);
    QObject::connect(_file, &File::textMax, _scrollbarHorizontal, &ScrollBar::positonMax);
    _scrollbarHorizontal->setTransparent(true);

    _winLayout = new WindowLayout();
    setLayout(_winLayout);
    _winLayout->addRightBorderWidget(_scrollbarVertical);
    _winLayout->setRightBorderTopAdjust(-1);
    _winLayout->setRightBorderBottomAdjust(-1);
    _winLayout->addBottomBorderWidget(_scrollbarHorizontal);
    _winLayout->setBottomBorderLeftAdjust(-1);
    _winLayout->addCentralWidget(_file);


    QObject::connect(_file, &File::modifiedChanged, this,
            [this] {
                QString filename = _file->getFilename();
                if (terminal()) {
                    terminal()->setTitle("chr - "+ filename);
                    terminal()->setIconTitle("chr - "+ filename);
                }
                if(_file->isModified()) {
                    filename = "*" + filename;
                    if (!_file->isNewFile()) {
                        _cmdReload->setEnabled(true);
                    }
                } else {
                    _cmdReload->setEnabled(false);
                }
                setWindowTitle(filename);
            }
    );

    //Wrap
    QObject::connect(new Tui::ZCommandNotifier("Wrap", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated,
                    this, &FileWindow::wrapDialog);

    //Save
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("s"), this, Qt::WindowShortcut), &Tui::ZShortcut::activated,
            this, &FileWindow::saveOrSaveas);
    QObject::connect(new Tui::ZCommandNotifier("Save", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated,
                    this, &FileWindow::saveOrSaveas);

    //Save As
    //shortcut dose not work in vte, konsole, ...
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("S", Qt::ControlModifier | Qt::ShiftModifier), this, Qt::WindowShortcut), &Tui::ZShortcut::activated,
            this, &FileWindow::saveFileDialog);
    QObject::connect(new Tui::ZCommandNotifier("SaveAs", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated,
                     this, &FileWindow::saveFileDialog);

    //Reload
    _cmdReload = new Tui::ZCommandNotifier("Reload", this, Qt::WindowShortcut);
    _cmdReload->setEnabled(false);
    QObject::connect(_cmdReload, &Tui::ZCommandNotifier::activated,
         this, [this] {
                if(_file->isModified()) {
                    ConfirmSave *confirmDialog = new ConfirmSave(this->parentWidget(), _file->getFilename(), ConfirmSave::Reload, false);
                    QObject::connect(confirmDialog, &ConfirmSave::exitSelected, [=]{
                        confirmDialog->deleteLater();
                        FileWindow::reload();
                    });

                    QObject::connect(confirmDialog, &ConfirmSave::rejected, [=]{
                        confirmDialog->deleteLater();
                    });
                } else {
                    FileWindow::reload();
                }
    });

    QObject::connect(new Tui::ZCommandNotifier("Close", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated, this, [this]{
        closeRequested();
    });

    //Edit
    QObject::connect(new Tui::ZCommandNotifier("Selectall", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated,
         [this] {
            _file->selectAll();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Cutline", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated,
         [this] {
            _file->cutline();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("SelectMode", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated,
         [this] {
            _file->toggleSelectMode();
        }
    );


    // File follow and pipe
    _cmdInputPipe = new Tui::ZCommandNotifier("InputPipe", this, Qt::WindowShortcut);
    _cmdInputPipe->setEnabled(false);
    QObject::connect(_cmdInputPipe, &Tui::ZCommandNotifier::activated,
         [&] {
            closePipe();
        }
    );

    _cmdFollow = new Tui::ZCommandNotifier("Following", this, Qt::WindowShortcut);
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
        _cmdReload->setEnabled(true);
    });

    _file->newText("");
    backingFileChanged("");
}

File *FileWindow::getFileWidget() {
    return _file;
}

void FileWindow::setWrap(Tui::ZTextOption::WrapMode wrap) {
    _file->setWrapOption(wrap);
    if(wrap == Tui::ZTextOption::NoWrap) {
        _scrollbarHorizontal->setVisible(true);
        _winLayout->setRightBorderBottomAdjust(-1);
    } else if (wrap == Tui::ZTextOption::WordWrap) {
        _scrollbarHorizontal->setVisible(false);
        _winLayout->setRightBorderBottomAdjust(-2);
    } else if (wrap == Tui::ZTextOption::WrapAnywhere) {
        _scrollbarHorizontal->setVisible(false);
        _winLayout->setRightBorderBottomAdjust(-2);
    }
}

void FileWindow::saveFile(QString filename) {
    _file->setFilename(filename);
    backingFileChanged(_file->getFilename());
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

WrapDialog *FileWindow::wrapDialog() {
    WrapDialog *wrapDialog = new WrapDialog(parentWidget(), _file);
    return wrapDialog;
}

SaveDialog *FileWindow::saveFileDialog() {
    SaveDialog *saveDialog = new SaveDialog(parentWidget(), _file);
    QObject::connect(saveDialog, &SaveDialog::fileSelected, this, &FileWindow::saveFile);
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
    if (filename.size()) {
        backingFileChanged(_file->getFilename());
    } else {
        backingFileChanged("");
    }
    fileChangedExternally(false);
    watcherAdd();
}

void FileWindow::openFile(QString filename) {
    closePipe();
    watcherRemove();
    _file->openText(filename);
    backingFileChanged(_file->getFilename());
    fileChangedExternally(false);
    watcherAdd();
}

void FileWindow::reload() {
    closePipe();
    QPoint xy = _file->getCursorPosition();
    watcherRemove();
    _file->openText(_file->getFilename());
    fileChangedExternally(false);
    _file->setCursorPosition(xy);
    watcherAdd();
}

void FileWindow::closeEvent(Tui::ZCloseEvent *event) {
    if (!event->skipChecks().contains("unsaved")) {
        closeRequested();
        event->ignore();
    }
}

void FileWindow::resizeEvent(Tui::ZResizeEvent *event) {
    updateBorders();
    Tui::ZWindow::resizeEvent(event);
}

void FileWindow::moveEvent(Tui::ZMoveEvent *event) {
    updateBorders();
    Tui::ZWindow::moveEvent(event);
}

void FileWindow::updateBorders() {
    auto * const term = terminal();
    if (!term) return;
    Qt::Edges borders = Qt::TopEdge;
    QRect g = geometry();
    bool touchesLeftEdge = (g.x() == 0);
    bool touchesRightEdge = (g.right() == term->mainWidget()->geometry().width() - 1);
    if (!touchesRightEdge || !touchesLeftEdge) {
        borders |= Qt::LeftEdge;
        borders |= Qt::RightEdge;
        _winLayout->setRightBorderTopAdjust(0);
        _winLayout->setRightBorderBottomAdjust(0);
        _scrollbarVertical->setTransparent(false);
    } else {
        _winLayout->setRightBorderTopAdjust(-1);
        _winLayout->setRightBorderBottomAdjust(-1);
        _scrollbarVertical->setTransparent(true);
    }
    if (g.bottom() != term->mainWidget()->geometry().height() - 2) {
        borders |= Qt::BottomEdge;
        _scrollbarHorizontal->setTransparent(false);
        _winLayout->setBottomBorderLeftAdjust(borders.testFlag(Qt::LeftEdge) ? 0 : -1);
    } else {
        _winLayout->setBottomBorderLeftAdjust(-1);
        _scrollbarHorizontal->setTransparent(true);
    }
    setBorderEdges(borders);
    // TODO: This needs to work without manually forcing relayout
    QTimer::singleShot(0, [this] {
        terminal()->requestLayout(this);
    });
}

void FileWindow::closeRequested() {
    _file->writeAttributes();
    if(_file->isModified()) {
        ConfirmSave *closeDialog = new ConfirmSave(this->parentWidget(), _file->getFilename(), ConfirmSave::Close, _file->getWritable());

        QObject::connect(closeDialog, &ConfirmSave::exitSelected, this, [this, closeDialog] {
            closeDialog->deleteLater();
            deleteLater();
        });

        QObject::connect(closeDialog, &ConfirmSave::saveSelected, this, [this, closeDialog]{
            closeDialog->deleteLater();
            SaveDialog *q = saveOrSaveas();
            if (q) {
                QObject::connect(q, &SaveDialog::fileSelected, this, [this] {
                    closeSkipCheck({"unsaved"});
                });
            } else {
                closeSkipCheck({"unsaved"});
            }
        });

        QObject::connect(closeDialog, &ConfirmSave::rejected, [=]{
            closeDialog->deleteLater();
        });
    } else {
        deleteLater();
    }
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
        ::close(0);
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

