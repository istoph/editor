#include "edit.h"

#include <signal.h>

#include <Tui/ZTerminal.h>
#include <Tui/ZImage.h>

Editor::Editor() {
    ensureCommandManager();

    Menubar *menu = new Menubar(this);
    menu->setItems({
                      { "<m>F</m>ile", "", {}, {
                            { "<m>N</m>ew", "Ctrl-N", "New", {}},
                            { "<m>O</m>pen...", "Ctrl-O", "Open", {}},
                            { "<m>S</m>ave", "Ctrl-S", "Save", {}},
                            { "Save <m>a</m>s...", "", "SaveAs", {}},
                            { "<m>R</m>eload", "", "Reload", {}},
                            {},
                            { "<m>Q</m>uit", "Ctrl-Q", "Quit", {}}
                        }
                      },
                      { "<m>E</m>dit", "", "", {
                            { "Cu<m>t</m>", "Ctrl-X", "Cut", {}},
                            { "<m>C</m>opy", "Ctrl-C", "Copy", {}},
                            { "<m>P</m>aste", "Ctrl-V", "Paste", {}},
                            { "Select <m>a</m>ll", "Ctrl-A", "Selectall", {}},
                            { "Cut Line", "Ctrl-K", "Cutline", {}},
                            { "Select Mode", "F4", "SelectMode", {}},
                            {},
                            { "Undo", "Ctrl-Z", "Undo", {}},
                            { "Redo", "Ctrl-Y", "Redo", {}},
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
                                 { "Formatting <m>T</m>ab", "", "Tab", {}},
                                 { "<m>L</m>ine Number", "", "LineNumber", {}},
                                 { "Formatting <m>C</m>haracters", "", "Formatting", {}},
                                 { "<m>W</m>rap long lines", "", "Wrap", {}},
                                 { "Following standard input", "", "Following", {}},
                                 { "Stop Input Pipe", "", "InputPipe", {}},
                                 { "<m>H</m>ighlight Brackets", "", "Brackets", {}},
                                 { "<m>T</m>heme", "", "Theme", {}}
                             }
                      },
                      { "Hel<m>p</m>", "", {}, {
                            { "<m>A</m>bout", "", "About", {}}
                        }
                      }
                   });

    //New
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("n"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, [&] {
                Editor::newFileMenue();
    });
    QObject::connect(new Tui::ZCommandNotifier("New", this), &Tui::ZCommandNotifier::activated,
            [&] {
                Editor::newFileMenue();
    });

    //Open
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("o"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
        this, [&] {
            openFileMenue();
    });
    QObject::connect(new Tui::ZCommandNotifier("Open", this), &Tui::ZCommandNotifier::activated,
        [&] {
            openFileMenue();
        }
    );

    //Save
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("s"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::saveOrSaveas);
    QObject::connect(new Tui::ZCommandNotifier("Save", this), &Tui::ZCommandNotifier::activated,
                    this, &Editor::saveOrSaveas);

    //Save As
    //shortcut dose not work in vte, konsole, ...
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("S", Qt::ControlModifier | Qt::ShiftModifier), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::saveFileDialog);
    QObject::connect(new Tui::ZCommandNotifier("SaveAs", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::saveFileDialog);

    //Reload
    QObject::connect(new Tui::ZCommandNotifier("Reload", this), &Tui::ZCommandNotifier::activated,
         this, [&] {
            if(file->isModified()) {
                ConfirmSave *confirmDialog = new ConfirmSave(this, file->getFilename(), ConfirmSave::Reload);
                QObject::connect(confirmDialog, &ConfirmSave::exitSelected, [=]{
                    delete confirmDialog;
                    Editor::reload();
                });

                QObject::connect(confirmDialog, &ConfirmSave::rejected, [=]{
                    delete confirmDialog;
                });
            } else {
                Editor::reload();
            }

    });

    //Quit
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("q"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::quit);
    QObject::connect(new Tui::ZCommandNotifier("Quit", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::quit);

    //Edit
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
    QObject::connect(new Tui::ZCommandNotifier("SelectMode", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->toggleSelectMode();
        }
    );

    //Search
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

    //Replace
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("r"), this, Qt::ApplicationShortcut),
                     &Tui::ZShortcut::activated, this, &Editor::replaceDialog);
    QObject::connect(new Tui::ZCommandNotifier("Replace", this), &Tui::ZCommandNotifier::activated,
         [&] {
            replaceDialog();
        }
    );

    //InsertCharacter
    QObject::connect(new Tui::ZCommandNotifier("InsertCharacter", this), &Tui::ZCommandNotifier::activated,
         [&] {
            new InsertCharacter(this, file);
        }
    );

    //Goto Line
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("g"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
        this, [&] {
            new GotoLine(this, file);
    });
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

    QObject::connect(new Tui::ZCommandNotifier("Theme", this), &Tui::ZCommandNotifier::activated,
         [&] {
            new ThemeDialog(this);
        }
    );
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forMnemonic("x"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
                     this, &Editor::showCommandLine);


    // Background
    setFillChar(u'▒');

    win = new WindowWidget(this);
    win->setBorderEdges({ Qt::TopEdge });

    //File
    file = new File(win);
    win->setWindowTitle(file->getFilename());

    _commandLineWidget = new CommandLineWidget(this);
    _commandLineWidget->setVisible(false);
    connect(_commandLineWidget, &CommandLineWidget::dismissed, this, &Editor::commandLineDismissed);
    connect(_commandLineWidget, &CommandLineWidget::execute, this, &Editor::commandLineExecute);

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
    connect(file, &File::emitOverwrite, _statusBar, &StatusBar::overwrite);

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

    _rootLayout = new VBoxLayout();
    setLayout(_rootLayout);
    _rootLayout->addWidget(menu);
    _rootLayout->addWidget(win);
    _rootLayout->addWidget(_commandLineWidget);
    _rootLayout->addWidget(_statusBar);

    _searchDialog = new SearchDialog(this, file);
    QObject::connect(new Tui::ZCommandNotifier("search", this), &Tui::ZCommandNotifier::activated,
                     _searchDialog, &SearchDialog::open);

    _replaceDialog = new SearchDialog(this, file, true);
    QObject::connect(new Tui::ZCommandNotifier("replace", this), &Tui::ZCommandNotifier::activated,
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

void Editor::newFileMenue() {
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

void Editor::newFile(QString filename) {
    closePipe();
    watcherRemove();
    windowTitle(filename);
    file->newText();
    file->setFilename(filename, true);
    _statusBar->fileHasBeenChanged(false);
    watcherAdd();
}

void Editor::openFileMenue() {
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

void Editor::openFile(QString filename) {
    closePipe();
    watcherRemove();
    windowTitle(filename);
    //reset couser position
    file->newText();
    file->setFilename(filename);
    file->openText();
    _statusBar->fileHasBeenChanged(false);
    watcherAdd();
}

void Editor::saveFile(QString filename) {
    file->setFilename(filename);
    watcherRemove();
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
        e->setDefaultPlacement(Qt::AlignCenter);
        e->setVisible(true);
        e->setFocus();
    }
    watcherAdd();

}
void Editor::openFileDialog(QString path) {
    OpenDialog *openDialog = new OpenDialog(this, path);
    connect(openDialog, &OpenDialog::fileSelected, this, &Editor::openFile);
}

QObject * Editor::facet(const QMetaObject metaObject) {
    if (metaObject.className()  == Clipboard::staticMetaObject.className()) {
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
    QPoint xy = file->getCursorPosition();

    file->openText();
    _statusBar->fileHasBeenChanged(false);

    file->setCursorPosition(xy);
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

void Editor::setTheme(Theme theme) {
    _theme = theme;
    if (theme == Theme::dark) {
        Tui::ZPalette tmpPalette = Tui::ZPalette::black();
        tmpPalette.setColors({
                                 {"chr.fgbehindText", { 0xaa, 0xaa, 0xaa}},
                                 {"chr.trackBgColor", { 0x80, 0x80, 0x80}},
                                 {"chr.thumbBgColor", { 0xd9, 0xd9, 0xd9}},
                                 {"chr.linenumberFg", { 0xdd, 0xdd, 0xdd}},
                                 {"chr.linenumberBg", { 0x80, 0x80, 0x80}},
                                 {"chr.statusbarBg", Tui::Colors::darkGray},
                             });
        setPalette(tmpPalette);
    } else if (theme == Theme::classic) {
        Tui::ZPalette tmpPalette = Tui::ZPalette::classic();
        tmpPalette.setColors({
                                 {"chr.fgbehindText", { 0xaa, 0xaa, 0xaa}},
                                 {"chr.trackBgColor", { 0,    0,    0x80}},
                                 {"chr.thumbBgColor", { 0,    0,    0xd9}},
                                 {"chr.linenumberFg", { 0xdd, 0xdd, 0xdd}},
                                 {"chr.linenumberBg", { 0,    0,    0x80}},
                                 {"chr.statusbarBg", {0, 0xaa, 0xaa}},
                                 {"root.fg", {0xaa,0xaa,0xaa}},
                                 {"root.bg", {0x0,0x0,0xaa}}
                             });
        setPalette(tmpPalette);
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

void Editor::showCommandLine() {
    _statusBar->setVisible(false);
    _commandLineWidget->setVisible(true);
    // Force relayout, this should not be needed
    _rootLayout->setGeometry(layoutArea());
    _commandLineWidget->grabKeyboard();
}

void Editor::commandLineDismissed() {
    _statusBar->setVisible(true);
    _commandLineWidget->setVisible(false);
    // Force relayout, this should not be needed
    _rootLayout->setGeometry(layoutArea());
}

void Editor::commandLineExecute(QString cmd) {
    commandLineDismissed();
    if (cmd.startsWith("screenshot-tpi ")) {
        QString filename = cmd.split(" ").value(1);
        if (filename.size()) {
            connect(terminal(), &Tui::ZTerminal::afterRendering, this, [this, filename, terminal=terminal()] {
                terminal->grabCurrentImage().save(filename);
                disconnect(terminal, &Tui::ZTerminal::afterRendering, this, 0);
            });
        }
    } else if (cmd == "help") {
        _commandLineWidget->setCmdEntryText("suspend shell");
        showCommandLine();
    } else if (cmd == "suspend") {
        ::raise(SIGTSTP);
    } else if (cmd == "shell") {
        auto term = terminal();
        term->pauseOperation();
        system(qgetenv("SHELL"));
        term->unpauseOperation();
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

void Editor::watcherAdd() {
    QFileInfo filenameInfo(file->getFilename());
    if(filenameInfo.exists()) {
        _watcher->addPath(file->getFilename());
    }
}
void Editor::watcherRemove() {
    if(_watcher->files().count() > 0) {
        _watcher->removePaths(_watcher->files());
    }
}
