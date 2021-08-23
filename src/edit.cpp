#include "edit.h"

#include <signal.h>

#include <Tui/ZTerminal.h>
#include <Tui/ZImage.h>

#include "confirmsave.h"

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
                            { "<m>C</m>lose", "", "Close", {}},
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
                Editor::newFile();
    });
    QObject::connect(new Tui::ZCommandNotifier("New", this), &Tui::ZCommandNotifier::activated,
            [&] {
                Editor::newFile();
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


    //Quit
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("q"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::quit);
    QObject::connect(new Tui::ZCommandNotifier("Quit", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::quit);

    //Search
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
         [this] {
            QObject::connect(new InsertCharacter(this), &InsertCharacter::characterSelected, this, [this] (QString str) {
                QVector<QString> vec;
                vec.append(str);
                if (_file) {
                    _file->insertAtCursorPosition(vec);
                }
            });
        }
    );

    //Goto Line
    auto gotoLine = [this] {
        QObject::connect(new GotoLine(this), &GotoLine::lineSelected, this, [this] (QString line) {
            if (_file) {
                _file->gotoline(line);
            }
        });

    };
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("g"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
        this, gotoLine);
    QObject::connect(new Tui::ZCommandNotifier("Gotoline", this), &Tui::ZCommandNotifier::activated, this, gotoLine);

    //Options
    QObject::connect(new Tui::ZCommandNotifier("Tab", this), &Tui::ZCommandNotifier::activated,
        [&] {
            if (_tabDialog) {
                _tabDialog->setFocus();
                _tabDialog->raise();
            } else {
                _tabDialog = new TabDialog(this);
                _tabDialog->updateSettings(!_file->getTabOption(), _file->getTabsize());
                QObject::connect(_tabDialog, &TabDialog::convert, this, [this] (bool useTabs, int indentSize) {
                    if (_file) {
                        _file->setTabOption(!useTabs);
                        _file->setTabsize(indentSize);
                        _file->tabToSpace();
                    }
                });
                QObject::connect(_tabDialog, &TabDialog::settingsChanged, this, [this] (bool useTabs, int indentSize) {
                    if (_file) {
                        _file->setTabOption(!useTabs);
                        _file->setTabsize(indentSize);
                    }
                });
            }
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("LineNumber", this), &Tui::ZCommandNotifier::activated,
        [&] {
            if (_file) {
                _file->toggleLineNumber();
            }
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Formatting", this), &Tui::ZCommandNotifier::activated,
         [&] {
            if (_file) {
                _file->setFormattingCharacters(!_file->getformattingCharacters());
            }
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Brackets", this), &Tui::ZCommandNotifier::activated,
         [&] {
            if (_file) {
                _file->setHighlightBracket(!_file->getHighlightBracket());
            }
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


    _commandLineWidget = new CommandLineWidget(this);
    _commandLineWidget->setVisible(false);
    connect(_commandLineWidget, &CommandLineWidget::dismissed, this, &Editor::commandLineDismissed);
    connect(_commandLineWidget, &CommandLineWidget::execute, this, &Editor::commandLineExecute);

    _statusBar = new StatusBar(this);

    VBoxLayout *rootLayout = new VBoxLayout();
    setLayout(rootLayout);
    rootLayout->addWidget(menu);
    _mdiLayout = new MdiLayout();
    rootLayout->add(_mdiLayout);
    rootLayout->addWidget(_commandLineWidget);
    rootLayout->addWidget(_statusBar);
    setupSearchDialogs();
}

Editor::~Editor() {
    // Delete children here manually, instead of leaving it to QObject,
    // to avoid children observing already destructed parent.
    for (QObject *child : children()) {
        delete child;
    }
}

FileWindow *Editor::createFileWindow() {
    FileWindow *win = new FileWindow(this);
    File *file = win->getFileWidget();

    _mux.connect(win, file, &File::cursorPositionChanged, _statusBar, &StatusBar::cursorPosition, 0, 0, 0);
    _mux.connect(win, file, &File::scrollPositionChanged, _statusBar, &StatusBar::scrollPosition, 0, 0);
    _mux.connect(win, file, &File::modifiedChanged, _statusBar, &StatusBar::setModified, false);
    _mux.connect(win, win, &FileWindow::readFromStandadInput, _statusBar, &StatusBar::readFromStandardInput, false);
    _mux.connect(win, win, &FileWindow::followStandadInput, _statusBar, &StatusBar::followStandardInput, false);
    _mux.connect(win, file, &File::setWritable, _statusBar, &StatusBar::setWritable, true);
    _mux.connect(win, file, &File::msdosMode, _statusBar, &StatusBar::msdosMode, false);
    _mux.connect(win, file, &File::modifiedSelectMode, _statusBar, &StatusBar::modifiedSelectMode, false);
    _mux.connect(win, file, &File::emitSearchCount, _statusBar, &StatusBar::searchCount, -1);
    _mux.connect(win, file, &File::emitSearchText, _statusBar, &StatusBar::searchText, QString());
    _mux.connect(win, file, &File::emitOverwrite, _statusBar, &StatusBar::overwrite, false);
    _mux.connect(win, win, &FileWindow::fileChangedExternally, _statusBar, &StatusBar::fileHasBeenChangedExternally, false);

    _allWindows.append(win);

    QObject::connect(win, &FileWindow::backingFileChanged, this, [this, win] (QString filename) {
        QMutableMapIterator<QString, FileWindow*> iter(_nameToWindow);
        while (iter.hasNext()) {
            iter.next();
            if (iter.value() == win) {
                if (iter.key() == filename) {
                    // Nothing changed
                    return;
                } else {
                    iter.remove();
                    break;
                }
            }
        }
        if (filename.size()) {
            Q_ASSERT(!_nameToWindow.contains(filename));
            _nameToWindow[filename] = win;
        }
    });

    QObject::connect(win, &QObject::destroyed, this, [this, win] {
        // remove from _nameToWindow map
        QMutableMapIterator<QString, FileWindow*> iter(_nameToWindow);
        while (iter.hasNext()) {
            iter.next();
            if (iter.value() == win) {
                iter.remove();
                break;
            }
        }
        if (_win == win) {
            _win = nullptr;
            _file = nullptr;
        }
        _allWindows.removeAll(win);
    });

    if (_win) {
        file->setTabsize(_file->getTabsize());
        file->setLineNumber(_file->getLineNumber());
        file->setTabOption(_file->getTabOption());
        file->setFormattingCharacters(_file->getformattingCharacters());
        win->setWrap(_file->getWrapOption());
        file->setHighlightBracket(_file->getHighlightBracket());
        file->setAttributesfile(_file->getAttributesfile());
        file->select_cursor_position_x0 = _file->select_cursor_position_x0;
    } else {
        file->setTabsize(initialFileSettings.tabSize);
        file->setLineNumber(initialFileSettings.showLineNumber);
        file->setTabOption(initialFileSettings.tabOption);
        file->setFormattingCharacters(initialFileSettings.formattingCharacters);
        win->setWrap(initialFileSettings.wrap);
        file->setHighlightBracket(initialFileSettings.highlightBracket);
        file->setAttributesfile(initialFileSettings.attributesFile);
        file->select_cursor_position_x0 = initialFileSettings.select_cursor_position_x0;
    }

    return win;
}

void Editor::setupSearchDialogs() {
    auto searchCancled = [this] {
        if (_file) {
            _file->setSearchText("");
            _file->resetSelect();
        }
    };

    auto searchCaseSensitiveChanged = [this](bool value) {
        if (_file) {
            if(value) {
                _file->searchCaseSensitivity = Qt::CaseInsensitive;
            } else {
                _file->searchCaseSensitivity = Qt::CaseSensitive;
            }
        }
    };

    auto searchWrapChanged = [this](bool value) {
        if (_file) {
            if (value) {
                _file->setSearchWrap(true);
            } else {
                _file->setSearchWrap(false);
            }
        }
    };

    auto searchForwardChanged = [this](bool value) {
        if (_file) {
            _file->setSearchDirection(value);
        }
    };

    auto liveSearch = [this](QString text, bool forward) {
        if (_file) {
            _file->setSearchText(text);
            _file->setSearchDirection(forward);
            if(forward) {
                _file->setCursorPosition({_file->getCursorPosition().x() - text.size(), _file->getCursorPosition().y()});
            } else {
                _file->setCursorPosition({_file->getCursorPosition().x() + text.size(), _file->getCursorPosition().y()});
            }
            _file->runSearch(false);
        }
    };

    auto searchNext = [this](QString text, bool regex, bool forward) {
        if (_file) {
            _file->setSearchText(text);
            _file->setRegex(regex);
            _file->setSearchDirection(forward);
            _file->runSearch(false);
        }
    };

    _searchDialog = new SearchDialog(this);
    QObject::connect(new Tui::ZCommandNotifier("search", this), &Tui::ZCommandNotifier::activated,
                     _searchDialog, &SearchDialog::open);
    QObject::connect(_searchDialog, &SearchDialog::canceled, this, searchCancled);
    QObject::connect(_searchDialog, &SearchDialog::caseSensitiveChanged, this, searchCaseSensitiveChanged);
    QObject::connect(_searchDialog, &SearchDialog::wrapChanged, this, searchWrapChanged);
    QObject::connect(_searchDialog, &SearchDialog::forwardChanged, this, searchForwardChanged);
    QObject::connect(_searchDialog, &SearchDialog::liveSearch, this, liveSearch);
    QObject::connect(_searchDialog, &SearchDialog::findNext, this, searchNext);

    _replaceDialog = new SearchDialog(this, true);
    QObject::connect(new Tui::ZCommandNotifier("replace", this), &Tui::ZCommandNotifier::activated,
                     _replaceDialog, &SearchDialog::open);
    QObject::connect(_replaceDialog, &SearchDialog::canceled, this, searchCancled);
    QObject::connect(_replaceDialog, &SearchDialog::caseSensitiveChanged, this, searchCaseSensitiveChanged);
    QObject::connect(_replaceDialog, &SearchDialog::wrapChanged, this, searchWrapChanged);
    QObject::connect(_replaceDialog, &SearchDialog::forwardChanged, this, searchForwardChanged);
    QObject::connect(_replaceDialog, &SearchDialog::liveSearch, this, liveSearch);
    QObject::connect(_replaceDialog, &SearchDialog::findNext, this, searchNext);
    QObject::connect(_replaceDialog, &SearchDialog::replace1, this, [this] (QString text, QString replacement, bool regex, bool forward) {
        if (_file) {
            _file->setSearchText(text);
            _file->setReplaceText(replacement);
            _file->setRegex(regex);
            _file->setReplaceSelected();
            _file->setSearchDirection(forward);
            _file->runSearch(false);
        }
    });
    QObject::connect(_replaceDialog, &SearchDialog::replaceAll, this, [this] (QString text, QString replacement, bool regex) {
        if (_file) {
            _file->setRegex(regex);
            _file->replaceAll(text, replacement);
        }
    });
}

void Editor::searchDialog() {
    _searchDialog->open();
    _searchDialog->setSearchText(_file->getSelectText());
}

void Editor::replaceDialog() {
    _replaceDialog->open();
    _replaceDialog->setSearchText(_file->getSelectText());
}

void Editor::newFile(QString filename) {
    FileWindow *win = createFileWindow();
    if (filename.size()) {
        win->getFileWidget()->newText(filename);
    }
    _mdiLayout->addWindow(win);
    win->getFileWidget()->setFocus();
}

void Editor::openFileMenue() {
    openFileDialog();
}

void Editor::gotoLineInCurrentFile(QString lineInfo) {
    if (_file) {
        _file->gotoline(lineInfo);
    }
}

void Editor::followInCurrentFile() {
    if (_win) {
        _win->setFollow(true);
    }
}

void Editor::watchPipe() {
    if (_win) {
        _win->watchPipe();
    }
}

void Editor::openFile(QString fileName) {
    QFileInfo filenameInfo(fileName);
    QString absFileName = filenameInfo.absoluteFilePath();
    if (_nameToWindow.contains(absFileName)) {
        _nameToWindow.value(absFileName)->getFileWidget()->setFocus();
        return;
    }

    if (_win && !_win->getFileWidget()->isModified() && _win->getFileWidget()->getFilename() == "NEWFILE") {
        _win->openFile(fileName);
    } else {
        FileWindow *win = createFileWindow();
        _mdiLayout->addWindow(win);
        win->openFile(fileName);
        win->getFileWidget()->setFocus();
    }
}

void Editor::openFileDialog(QString path) {
    OpenDialog *openDialog = new OpenDialog(this, path);
    connect(openDialog, &OpenDialog::fileSelected, this, [this] (QString fileName) {
        openFile(fileName);
    });
}

QObject * Editor::facet(const QMetaObject metaObject) {
    if (metaObject.className()  == Clipboard::staticMetaObject.className()) {
        return &_clipboard;
    } else {
        return ZRoot::facet(metaObject);
    }
}

void Editor::quitImpl(int i) {
    if (i >= _allWindows.size()) {
        QCoreApplication::instance()->quit();
        return;
    }

    auto handleNext = [this, i] {
        quitImpl(i + 1);
    };

    auto *win = _allWindows[i];
    auto *file = win->getFileWidget();
    file->writeAttributes();
    if(file->isModified()) {
        ConfirmSave *quitDialog = new ConfirmSave(this, file->getFilename(), ConfirmSave::Quit, file->getWritable());

        QObject::connect(quitDialog, &ConfirmSave::exitSelected, [=]{
            quitDialog->deleteLater();
            handleNext();
        });

        QObject::connect(quitDialog, &ConfirmSave::saveSelected, [=]{
            quitDialog->deleteLater();
            SaveDialog *q = win->saveOrSaveas();
            if (q) {
                connect(q, &SaveDialog::fileSelected, this, handleNext);
            } else {
                handleNext();
            }
        });

        QObject::connect(quitDialog, &ConfirmSave::rejected, [=]{
            quitDialog->deleteLater();
        });
    } else {
        handleNext();
    }
}

void Editor::quit() {
    if (_allWindows.isEmpty()) {
        QCoreApplication::instance()->quit();
        return;
    }

    quitImpl(0);
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

void Editor::setInitialFileSettings(const Settings &initial) {
    initialFileSettings = initial;
}

void Editor::showCommandLine() {
    _statusBar->setVisible(false);
    _commandLineWidget->setVisible(true);
    // Force relayout, this should not be needed
    _commandLineWidget->grabKeyboard();
}

void Editor::commandLineDismissed() {
    _statusBar->setVisible(true);
    _commandLineWidget->setVisible(false);
    // Force relayout, this should not be needed
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

void Editor::terminalChanged() {
    QObject::connect(terminal(), &Tui::ZTerminal::focusChanged, this, [this] {
        Tui::ZWidget *w = terminal()->focusWidget();
        while (w && !qobject_cast<FileWindow*>(w)) {
            w = w->parentWidget();
        }
        if (qobject_cast<FileWindow*>(w)) {
            _win = qobject_cast<FileWindow*>(w);
            if (_file != _win->getFileWidget()) {
                _file = _win->getFileWidget();
                if (_tabDialog) {
                    _tabDialog->updateSettings(!_file->getTabOption(), _file->getTabsize());
                }
            }
            _mux.selectInput(_win);
        }
    });
}
