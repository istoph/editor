// SPDX-License-Identifier: BSL-1.0

#include "filewindow.h"

#include <unistd.h>

#include <Tui/Misc/SurrogateEscape.h>
#include <Tui/ZSymbol.h>
#include <Tui/ZTerminal.h>

#include "alert.h"
#include "confirmsave.h"


FileWindow::FileWindow(Tui::ZWidget *parent) : Tui::ZWindow(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose
               | Tui::ZWindow::MoveOption | Tui::ZWindow::ResizeOption
               | Tui::ZWindow::AutomaticOption);
    setBorderEdges({ Qt::TopEdge });

    _file = new File(terminal()->textMetrics(), this);

    _scrollbarVertical = new ScrollBar(this);
    _scrollbarVertical->setTransparent(true);
    QObject::connect(_file, &File::scrollPositionChanged, _scrollbarVertical, &ScrollBar::scrollPosition);
    QObject::connect(_file, &File::scrollRangeChanged, _scrollbarVertical, &ScrollBar::positonMax);

    _scrollbarHorizontal = new ScrollBar(this);
    QObject::connect(_file, &File::scrollPositionChanged, _scrollbarHorizontal, &ScrollBar::scrollPosition);
    QObject::connect(_file, &File::scrollRangeChanged, _scrollbarHorizontal, &ScrollBar::positonMax);
    _scrollbarHorizontal->setTransparent(true);

    _winLayout = new Tui::ZWindowLayout();
    setLayout(_winLayout);
    _winLayout->setRightBorderWidget(_scrollbarVertical);
    _winLayout->setRightBorderTopAdjust(-1);
    _winLayout->setRightBorderBottomAdjust(-1);
    _winLayout->setBottomBorderWidget(_scrollbarHorizontal);
    _winLayout->setBottomBorderLeftAdjust(-1);
    _winLayout->setCentralWidget(_file);


    QObject::connect(_file, &File::modifiedChanged, this,
            [this] {
                updateTitle();
            }
    );

    //Wrap
    QObject::connect(new Tui::ZCommandNotifier("Wrap", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated,
                    this, &FileWindow::wrapDialog);

    //Save
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("s"), this, Qt::WindowShortcut), &Tui::ZShortcut::activated,
            this, [this] { saveOrSaveas(); });
    QObject::connect(new Tui::ZCommandNotifier("Save", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated,
                    this, [this] { saveOrSaveas(); } );

    //Save As
    //shortcut does not work in vte, konsole, ...
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("S", Qt::ControlModifier | Qt::ShiftModifier), this, Qt::WindowShortcut), &Tui::ZShortcut::activated,
            this, [this] { saveFileDialog(); });
    QObject::connect(new Tui::ZCommandNotifier("SaveAs", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated,
                     this, [this] { saveFileDialog(); });

    //Reload
    _cmdReload = new Tui::ZCommandNotifier("Reload", this, Qt::WindowShortcut);
    _cmdReload->setEnabled(false);
    QObject::connect(_cmdReload, &Tui::ZCommandNotifier::activated,
         this, [this] {
                if (_file->isModified()) {
                    ConfirmSave *confirmDialog = new ConfirmSave(this->parentWidget(), _file->getFilename(),
                                                                 ConfirmSave::Reload, false);
                    QObject::connect(confirmDialog, &ConfirmSave::discardSelected, [this,confirmDialog] {
                        confirmDialog->deleteLater();
                        reload();
                    });

                    QObject::connect(confirmDialog, &ConfirmSave::rejected, [confirmDialog] {
                        confirmDialog->deleteLater();
                    });
                } else {
                    FileWindow::reload();
                }
    });

    //Close
    //shortcut does not work in vte, konsole, ...
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("Q", Qt::ControlModifier | Qt::ShiftModifier), this, Qt::WindowShortcut),
                     &Tui::ZShortcut::activated, this, [this] {
            closeRequested();
        }
    );
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, "q", {}), this, Qt::WindowShortcut),
                     &Tui::ZShortcut::activated, this, [this] {
            closeRequested();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("Close", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated, this,
        [this] {
            closeRequested();
        }
    );

    //Edit
    QObject::connect(new Tui::ZCommandNotifier("Selectall", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated, this,
         [this] {
            _file->selectAll();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Cutline", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated, this,
         [this] {
            _file->cutline();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("DeleteLine", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated, this,
         [this] {
            _file->deleteLine();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("SelectMode", this, Qt::WindowShortcut), &Tui::ZCommandNotifier::activated, this,
         [this] {
            _file->toggleSelectMode();
        }
    );


    // File follow and pipe
    _cmdInputPipe = new Tui::ZCommandNotifier("StopInputPipe", this, Qt::WindowShortcut);
    _cmdInputPipe->setEnabled(false);
    QObject::connect(_cmdInputPipe, &Tui::ZCommandNotifier::activated, this,
         [this] {
            closePipe();
        }
    );

    _cmdFollow = new Tui::ZCommandNotifier("Following", this, Qt::WindowShortcut);
    _cmdFollow->setEnabled(false);
    QObject::connect(_cmdFollow, &Tui::ZCommandNotifier::activated, this,
         [this] {
            setFollow(!getFollow());
        }
    );

    QObject::connect(this, &FileWindow::readFromStandadInput, this, [this](bool enable) {
        _cmdInputPipe->setEnabled(enable);
        _cmdFollow->setEnabled(enable);
    });


    _watcher = new QFileSystemWatcher();
    QObject::connect(_watcher, &QFileSystemWatcher::fileChanged, this, [this] {
        fileChangedExternally(true);
    });

    _file->newText("");
    backingFileChanged("");
}

File *FileWindow::getFileWidget() {
    return _file;
}

void FileWindow::setWrap(Tui::ZTextOption::WrapMode wrap) {
    _file->setWordWrapMode(wrap);
    if (wrap == Tui::ZTextOption::NoWrap) {
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

bool FileWindow::saveFile(QString filename, std::optional<bool> crlfMode) {
    _file->setFilename(filename);
    backingFileChanged(_file->getFilename());
    watcherRemove();
    if (crlfMode.has_value()) {
        _file->document()->setCrLfMode(*crlfMode);
    }
    const bool ok = _file->saveText();
    if (ok) {
        //windowTitle(filename);
        update();
        fileChangedExternally(false);
    } else {
        Alert *e = new Alert(parentWidget());
        e->setWindowTitle("Error");
        e->setMarkup("file could not be saved");
        e->setGeometry({15, 5, 50, 5});
        e->setDefaultPlacement(Qt::AlignCenter);
        e->setVisible(true);
        e->setFocus();
    }
    watcherAdd();

    _cmdReload->setEnabled(true);
    return ok;
}

WrapDialog *FileWindow::wrapDialog() {
    WrapDialog *wrapDialog = new WrapDialog(parentWidget(), _file);
    return wrapDialog;
}

SaveDialog *FileWindow::saveFileDialog(std::function<void(bool)> callback) {
    SaveDialog *saveDialog = new SaveDialog(parentWidget(), _file);
    QObject::connect(saveDialog, &SaveDialog::fileSelected, this, [this,callback](const QString &filename, bool crlfMode) {
        const bool ok = saveFile(filename, crlfMode);
        if (callback) {
            callback(ok);
        }
    });
    return saveDialog;
}

SaveDialog *FileWindow::saveOrSaveas(std::function<void(bool)> callback) {
    if (_file->isSaveAs()) {
        SaveDialog *q = saveFileDialog(callback);
        return q;
    } else {
        const bool ok = saveFile(_file->getFilename(), std::nullopt);
        if (callback) {
            callback(ok);
        }
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
    if (!_file->openText(filename)) {
        Alert *e = new Alert(parentWidget());
        e->setWindowTitle("Error");
        e->setMarkup("Error while reading file.");
        e->setGeometry({15, 5, 50, 5});
        e->setDefaultPlacement(Qt::AlignCenter);
        e->setVisible(true);
        e->setFocus();
    }
    backingFileChanged(_file->getFilename());
    fileChangedExternally(false);
    watcherAdd();

    _cmdReload->setEnabled(true);
}

void FileWindow::reload() {
    closePipe();
    _file->clearSelection();
    Tui::ZDocumentCursor::Position cursorPosition = _file->cursorPosition();
    watcherRemove();
    if (!_file->openText(_file->getFilename())) {
        Alert *e = new Alert(parentWidget());
        e->setWindowTitle("Error");
        e->setMarkup("Error while reading file.");
        e->setGeometry({15, 5, 50, 5});
        e->setDefaultPlacement(Qt::AlignCenter);
        e->setVisible(true);
        e->setFocus();
    }
    fileChangedExternally(false);
    _file->setCursorPosition(cursorPosition);
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
    updateTitle();
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
}

void FileWindow::updateTitle() {
    if (!terminal()) {
        return;
    }

    QString title = _file->getFilename();
    if (_file->isModified()) {
        title = "*" + title;
    }

    terminal()->setTitle("chr - " + title);
    terminal()->setIconTitle("chr - " + title);

    Tui::ZTextMetrics metrics = terminal()->textMetrics();

    const int maxWidth = geometry().width() - 14;

    const int width = metrics.sizeInColumns(title);

    if (width > maxWidth) {
        auto splitPoint = metrics.splitByColumns(title, width - maxWidth - 1);
        title = "…" + title.mid(splitPoint.codeUnits);
    }

    setWindowTitle(title);
}

void FileWindow::closeRequested() {
    _file->writeAttributes();
    if (_file->isModified()) {
        ConfirmSave *closeDialog = new ConfirmSave(parentWidget(), _file->getFilename(),
                                                   _file->isNewFile() ? ConfirmSave::CloseUnnamed : ConfirmSave::Close,
                                                   _file->getWritable());

        QObject::connect(closeDialog, &ConfirmSave::discardSelected, this, [this, closeDialog] {
            closeDialog->deleteLater();
            deleteLater();
        });

        QObject::connect(closeDialog, &ConfirmSave::saveSelected, this, [this, closeDialog] {
            closeDialog->deleteLater();
            saveOrSaveas([this](bool ok) {
                if (ok) {
                    closeSkipCheck({"unsaved"});
                }
            });
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
    if (filenameInfo.exists()) {
        _watcher->addPath(_file->getFilename());
    }
}
void FileWindow::watcherRemove() {
    if (_watcher->files().count() > 0) {
        _watcher->removePaths(_watcher->files());
    }
}


void FileWindow::closePipe() {
    if (_pipeSocketNotifier != nullptr && _pipeSocketNotifier->isEnabled()) {
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
            _file->appendLine(Tui::Misc::SurrogateEscape::decode(_pipeLineBuffer));
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
            _file->appendLine(Tui::Misc::SurrogateEscape::decode(_pipeLineBuffer.left(index)));
            _pipeLineBuffer = _pipeLineBuffer.mid(index + 1);
        }
        _file->modifiedChanged(true);
    }

    if (_pipeSocketNotifier == nullptr) {
        readFromStandadInput(false);
    } else {
        readFromStandadInput(true);
    }
}


void FileWindow::setFollow(bool follow) {
    _follow = follow;
    _file->setFollowStandardInput(getFollow());
    followStandadInput(getFollow());
}

bool FileWindow::getFollow() {
    return _follow;
}

