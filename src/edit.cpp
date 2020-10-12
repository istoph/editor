#include "edit.h"
#include "tabdialog.h"

Editor::Editor() {
    ensureCommandManager();

    Menubar *menu = new Menubar(this);
    menu->setItems({
                      { "<m>F</m>ile", "", {}, {
                            { "<m>N</m>ew", "Ctrl-N", "New", {}},
                            { "<m>O</m>pen...", "Ctrl-O", "Open", {}},
                            { "<m>S</m>ave", "Ctrl-S", "Save", {}},
                            { "Save <m>a</m>s...", "Ctrl-Shift-S", "SaveAs", {}},
                            { "<m>R</m>eload", "", "Reload", {}},
                            {},
                            { "<m>Q</m>uit", "Ctrl-q", "Quit", {}}
                        }
                      },
                      { "<m>E</m>dit", "", "", {
                            { "Cu<m>t</m>", "Ctrl-X", "Cut", {}},
                            { "<m>C</m>opy", "Ctrl-C", "Copy", {}},
                            { "<m>P</m>aste", "Ctrl-V", "Paste", {}},
                            { "Select <m>a</m>ll", "Ctrl-A", "Selectall", {}},
                            { "Cut Line", "Ctrl-K", "Cutline", {}},
                            {},
                            { "Undo", "Ctrl-z", "Undo", {}},
                            { "Redo", "Ctrl-y", "Redo", {}},
                            {},
                            { "<m>S</m>earch", "Ctrl-F", "Search", {}},
                            { "Search <m>N</m>ext", "F3", "Search Next", {}},
                            { "Search <m>P</m>revious", "Shift-F3", "Search Previous", {}},
                            { "<m>R</m>eplace", "Ctrl-R", "Replace", {}},
                            {},
                            { "Insert C<m>h</m>aracter...", "", "InsertCharacter", {}},
                            {},
                            { "<m>G</m>oto line", "Ctrl-G", "Gotoline", {}}
                        }
                      },
                      { "<m>O</m>ptions", "", {}, {
                                 { "<m>T</m>ab", "", "Tab", {}},
                                 { "<m>L</m>ine Number", "", "LineNumber", {}},
                                 { "<m>F</m>ormatting characters", "", "Formatting", {}},
                                 { "<m>W</m>rap long lines", "", "Wrap", {}},
                                 { "Following standard input", "", "Following", {}},
                                 { "Stop Input Pipe", "", "InputPipe", {}},
                                 { "<m>H</m>ighlight Brackets", "", "Brackets", {}}
                             }
                           },
                      { "Hel<m>p</m>", "", {}, {
                            { "<m>A</m>bout", "", "About", {}}
                        }
                      }
                   });


    //File
    QObject::connect(new Tui::ZCommandNotifier("New", this), &Tui::ZCommandNotifier::activated,
         [&] {
            if(file->isModified()) {
                ConfirmSave *confirmDialog = new ConfirmSave(this, file->getFilename(), ConfirmSave::New);
                QObject::connect(confirmDialog, &ConfirmSave::exitSelected, [=]{
                    delete confirmDialog;
                    newFile();
                });

                QObject::connect(confirmDialog, &ConfirmSave::saveSelected, [=]{
                    saveOrSaveas();
                    delete confirmDialog;
                    newFile();
                });
                QObject::connect(confirmDialog, &ConfirmSave::rejected, [=]{
                    delete confirmDialog;
                });
            } else {
                newFile();
            }

        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Open", this), &Tui::ZCommandNotifier::activated,
        [&] {
            if(file->isModified()) {
                ConfirmSave *confirmDialog = new ConfirmSave(this, file->getFilename(), ConfirmSave::Open);
                QObject::connect(confirmDialog, &ConfirmSave::exitSelected, [=]{
                    delete confirmDialog;
                    openFileDialog();
                });

                QObject::connect(confirmDialog, &ConfirmSave::saveSelected, [=]{
                    saveOrSaveas();
                    delete confirmDialog;
                    openFileDialog();
                });

                QObject::connect(confirmDialog, &ConfirmSave::rejected, [=]{
                    delete confirmDialog;
                });
            } else {
                openFileDialog();
            }
        }
    );

    //SAVE
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("s"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::saveOrSaveas);
    QObject::connect(new Tui::ZCommandNotifier("Save", this), &Tui::ZCommandNotifier::activated,
                    this, &Editor::saveOrSaveas);

    //SAVE AS
    //shortcut dose not work in vte, konsole, ...
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("S"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::saveFileDialog);
    QObject::connect(new Tui::ZCommandNotifier("SaveAs", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::saveFileDialog);

    //Reload
    QObject::connect(new Tui::ZCommandNotifier("Reload", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::reload);

    //QUIT
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("q"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::quit);
    QObject::connect(new Tui::ZCommandNotifier("Quit", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::quit);

    //Edit
    QObject::connect(new Tui::ZCommandNotifier("Cut", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->cut();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("Copy", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->copy();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("Paste", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->paste();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("Selectall", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->selectAll();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("Cutline", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->cutline();
        }
    );

    //SEARCH
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, 0), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
          [&] {
            file->runSearch(false);
          });
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, Qt::ShiftModifier), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
          [&] {
            file->runSearch(true);
          });
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("f"), this, Qt::ApplicationShortcut),
                     &Tui::ZShortcut::activated, this, &Editor::searchDialog);
    QObject::connect(new Tui::ZCommandNotifier("Search", this), &Tui::ZCommandNotifier::activated,
         [&] {
            searchDialog();
        }
    );
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("r"), this, Qt::ApplicationShortcut),
                     &Tui::ZShortcut::activated, this, &Editor::replaceDialog);
    QObject::connect(new Tui::ZCommandNotifier("Replace", this), &Tui::ZCommandNotifier::activated,
         [&] {
            replaceDialog();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("InsertCharacter", this), &Tui::ZCommandNotifier::activated,
         [&] {
            new InsertCharacter(this, file);
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("Gotoline", this), &Tui::ZCommandNotifier::activated,
         [&] {
            new GotoLine(this, file);
        }
    );

    //Options
    QObject::connect(new Tui::ZCommandNotifier("Tab", this), &Tui::ZCommandNotifier::activated,
        [&] {
            new TabDialog(file, this);
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("LineNumber", this), &Tui::ZCommandNotifier::activated,
        [&] {
            file->toggleLineNumber();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Formatting", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->setFormattingCharacters(!file->getformattingCharacters());
            update();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Wrap", this), &Tui::ZCommandNotifier::activated,
         [&] {
            setWrap(!file->getWrapOption());
        }
    );

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

    QObject::connect(this, &Editor::readFromStandadInput, this, [=](bool enable){
        _cmdInputPipe->setEnabled(enable);
        _cmdFollow->setEnabled(enable);
    });

    QObject::connect(new Tui::ZCommandNotifier("Brackets", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->setHighlightBracket(!file->getHighlightBracket());
        }
    );

    win = new WindowWidget(this);
    win->setBorderEdges({ Qt::TopEdge });

    //File
    file = new File(win);
    win->setWindowTitle(file->getFilename());

    _statusBar = new StatusBar(this);
    connect(file, &File::cursorPositionChanged, _statusBar, &StatusBar::cursorPosition);
    connect(file, &File::scrollPositionChanged, _statusBar, &StatusBar::scrollPosition);
    connect(file, &File::modifiedChanged, _statusBar, &StatusBar::setModified);
    connect(file, &File::modifiedChanged, this,
            [&] {
                windowTitle(file->getFilename());
            }
    );
    connect(this, &Editor::readFromStandadInput, _statusBar, &StatusBar::readFromStandardInput);
    connect(this, &Editor::followStandadInput, _statusBar, &StatusBar::followStandardInput);
    connect(file, &File::setWritable, _statusBar, &StatusBar::setWritable);
    connect(file, &File::msdosMode, _statusBar, &StatusBar::msdosMode);
    connect(file, &File::modifiedSelectMode, _statusBar, &StatusBar::modifiedSelectMode);
    connect(file, &File::emitSearchCount, _statusBar, &StatusBar::searchCount);
    connect(file, &File::emitSearchText, _statusBar, &StatusBar::searchText);

    ScrollBar *sc = new ScrollBar(win);
    sc->setTransparent(true);
    connect(file, &File::scrollPositionChanged, sc, &ScrollBar::scrollPosition);
    connect(file, &File::textMax, sc, &ScrollBar::positonMax);

    _scrollbarHorizontal = new ScrollBar(win);
    connect(file, &File::scrollPositionChanged, _scrollbarHorizontal, &ScrollBar::scrollPosition);
    connect(file, &File::textMax, _scrollbarHorizontal, &ScrollBar::positonMax);
    _scrollbarHorizontal->setTransparent(true);

    _winLayout = new WindowLayout();
    win->setLayout(_winLayout);
    _winLayout->addRightBorderWidget(sc);
    _winLayout->setRightBorderTopAdjust(-1);
    _winLayout->setRightBorderBottomAdjust(-1);
    _winLayout->addBottomBorderWidget(_scrollbarHorizontal);
    _winLayout->setBottomBorderLeftAdjust(-1);
    _winLayout->addCentralWidget(file);

    VBoxLayout *rootLayout = new VBoxLayout();
    setLayout(rootLayout);
    rootLayout->addWidget(menu);
    rootLayout->addWidget(win);
    rootLayout->addWidget(_statusBar);

    _searchDialog = new SearchDialog(this, file);
    QObject::connect(new Tui::ZCommandNotifier("search", this), &Tui::ZCommandNotifier::activated,
                     _searchDialog, &SearchDialog::open);

    _replaceDialog = new SearchDialog(this, file, true);
    QObject::connect(new Tui::ZCommandNotifier("search", this), &Tui::ZCommandNotifier::activated,
                     _replaceDialog, &SearchDialog::open);

    _watcher = new QFileSystemWatcher();
    QObject::connect(_watcher, &QFileSystemWatcher::fileChanged, this, [=]{
        _statusBar->fileHasBeenChanged(true);
    });
}

void Editor::closePipe() {
    if(_pipeSocketNotifier != nullptr && _pipeSocketNotifier->isEnabled()) {
        _pipeSocketNotifier->setEnabled(false);
        _pipeSocketNotifier->deleteLater();
        _pipeSocketNotifier = nullptr;
        close(0);
        readFromStandadInput(false);
    }
}

void Editor::watchPipe() {
    _pipeSocketNotifier = new QSocketNotifier(0, QSocketNotifier::Type::Read, this);
    QObject::connect(_pipeSocketNotifier, &QSocketNotifier::activated, this, &Editor::inputPipeReadable);
    readFromStandadInput(true);
}

void Editor::searchDialog() {
    _searchDialog->open();
    _searchDialog->setSearchText(file->getSelectText());
}

void Editor::replaceDialog() {
    _replaceDialog->open();
    _replaceDialog->setSearchText(file->getSelectText());
}

void Editor::windowTitle(QString filename) {
    terminal()->setTitle("chr - "+ filename);
    if(file->isModified()) {
        filename = "*" + filename;
    }
    win->setWindowTitle(filename);
}

void Editor::newFile(QString filename) {
    windowTitle(filename);
    file->newText();
    file->setFilename(filename, true);
    closePipe();
    _statusBar->fileHasBeenChanged(false);
    _watcher->addPath(file->getFilename());
}

void Editor::openFile(QString filename) {
    windowTitle(filename);
    //reset couser position
    file->newText();
    file->setFilename(filename);
    file->openText();
    closePipe();
    _statusBar->fileHasBeenChanged(false);
    _watcher->addPath(file->getFilename());
}

void Editor::saveFile(QString filename) {
    file->setFilename(filename);
    _watcher->removePaths(_watcher->files());
    if(file->saveText()) {
        windowTitle(filename);
        win->update();
        _statusBar->fileHasBeenChanged(false);
    } else {
        Alert *e = new Alert(this);
        e->addPaletteClass("red");
        e->setWindowTitle("Error");
        e->setMarkup("file could not be saved"); //Die Datei konnte nicht gespeicher werden!
        e->setGeometry({15,5,50,5});
        e->setVisible(true);
        e->setFocus();
    }
    _watcher->addPath(file->getFilename());

}
void Editor::openFileDialog(QString path) {
    OpenDialog * openDialog = new OpenDialog(this, path);
    connect(openDialog, &OpenDialog::fileSelected, this, &Editor::openFile);
}

QObject * Editor::facet(const QMetaObject metaObject) {
    if (metaObject.className()  == Clipboard::staticMetaObject.className()
            ||metaObject.inherits(&Clipboard::staticMetaObject)) {
        return &_clipboard;
    } else {
        return ZRoot::facet(metaObject);
    }
}

SaveDialog * Editor::saveFileDialog() {
    SaveDialog * saveDialog = new SaveDialog(this, file);
    connect(saveDialog, &SaveDialog::fileSelected, this, &Editor::saveFile);
    return saveDialog;
}

SaveDialog * Editor::saveOrSaveas() {
    if (file->newfile) {
        SaveDialog *q = saveFileDialog();
        return q;
    } else {
        saveFile(file->getFilename());
        return nullptr;
    }
}

void Editor::reload() {
    //TODO: ask if discard
    int x = file->_cursorPositionX;
    int y = file->_cursorPositionY;

    file->newText();
    file->openText();
    _statusBar->fileHasBeenChanged(false);

    file->_cursorPositionX = x;
    file->_cursorPositionY = y;
    //file->adjustScrollPosition();
}

void Editor::quit() {
    file->writeAttributes();
    if(file->isModified()) {
        ConfirmSave *quitDialog = new ConfirmSave(this, file->getFilename(), ConfirmSave::Quit, file->getWritable());

        QObject::connect(quitDialog, &ConfirmSave::exitSelected, [=]{
            QCoreApplication::instance()->quit();
        });

        QObject::connect(quitDialog, &ConfirmSave::saveSelected, [=]{
            quitDialog->deleteLater();
            SaveDialog *q = saveOrSaveas();
            if (q) {
                connect(q, &SaveDialog::fileSelected, QCoreApplication::instance(), &QCoreApplication::quit);
            } else {
                QCoreApplication::instance()->quit();
            }
        });

        QObject::connect(quitDialog, &ConfirmSave::rejected, [=]{
            quitDialog->deleteLater();
        });
    } else {
        QCoreApplication::instance()->quit();
    }
}

void Editor::setWrap(bool wrap) {
    if (wrap) {
        file->setWrapOption(true);
        _scrollbarHorizontal->setVisible(false);
        _winLayout->setRightBorderBottomAdjust(-2);
    } else {
        file->setWrapOption(false);
        _scrollbarHorizontal->setVisible(true);
        _winLayout->setRightBorderBottomAdjust(-1);
    }
}

void Editor::inputPipeReadable(int socket) {
    char buff[1024];
    int bytes = read(socket, buff, 1024);
    if (bytes == 0) {
        // EOF
        if (!_pipeLineBuffer.isEmpty()) {
            file->appendLine(surrogate_escape::decode(_pipeLineBuffer));
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
            file->appendLine(surrogate_escape::decode(_pipeLineBuffer.left(index)));
            _pipeLineBuffer = _pipeLineBuffer.mid(index + 1);
        }
        file->modifiedChanged(true);
    }

    if(_pipeSocketNotifier == nullptr) {
        readFromStandadInput(false);
    } else {
        readFromStandadInput(true);
    }
}

void Editor::setFollow(bool follow) {
    _follow = follow;
    file->followStandardInput(getFollow());
    followStandadInput(getFollow());
}

bool Editor::getFollow() {
    return _follow;
}
