// SPDX-License-Identifier: BSL-1.0

#include "edit.h"

#include <signal.h>

#include <QCoreApplication>

#include <Tui/ZCommandNotifier.h>
#include <Tui/ZImage.h>
#include <Tui/ZMenubar.h>
#include <Tui/ZPalette.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTerminalDiagnosticsDialog.h>
#include <Tui/ZTextLine.h>
#include <Tui/ZVBoxLayout.h>

#include "aboutdialog.h"
#include "confirmsave.h"
#include "formattingdialog.h"
#include "gotoline.h"
#include "insertcharacter.h"
#include "opendialog.h"


Editor::Editor() {
}

Editor::~Editor() {
    // Delete children here manually, instead of leaving it to QObject,
    // to avoid children observing already destructed parent.
    for (QObject *child : children()) {
        delete child;
    }
}

void Editor::setupUi() {

    ensureCommandManager();

    Tui::ZMenubar *menu = new Tui::ZMenubar(this);
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
                            { "<m>D</m>elete Line", "Ctrl-D", "DeleteLine", {}},
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
                            { "<m>G</m>oto Line", "Ctrl-G", "Gotoline", {}},
                            {},
                            { "Sort Selcted Lines", "Alt-Shift-S", "SortSelectedLines", {}}
                        }
                      },
                      { "<m>O</m>ptions", "", {}, {
                                 { "Formatting Ta<m>b</m>", "", "Tab", {}},
                                 { "<m>L</m>ine Number", "", "LineNumber", {}},
                                 { "<m>F</m>ormatting", "", "Formatting", {}},
                                 { "<m>W</m>rap long lines", "", "Wrap", {}},
                                 { "Following (standard input)", "", "Following", {}},
                                 { "Stop Input Pipe", "", "InputPipe", {}},
                                 { "<m>H</m>ighlight Brackets", "", "Brackets", {}},
                                 { "<m>S</m>yntax Highlighting", "", "SyntaxHighlighting", {}},
                                 { "<m>T</m>heme", "", "Theme", {}}
                             }
                      },
                      {"<m>W</m>indow", this, [this] { return createWindowMenu(); }},
                      { "Hel<m>p</m>", "", {}, {
                            { "Terminal diagnostics", "", "TerminalDiagnostics", {}},
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
        this, [this] {
            openFileMenu();
    });
    QObject::connect(new Tui::ZCommandNotifier("Open", this), &Tui::ZCommandNotifier::activated,
        this, [this] {
            openFileMenu();
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
    _cmdSearch = new Tui::ZCommandNotifier("Search", this);
    QObject::connect(_cmdSearch, &Tui::ZCommandNotifier::activated, this, &Editor::searchDialog);

    //Replace
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("r"), this, Qt::ApplicationShortcut),
                     &Tui::ZShortcut::activated, this, &Editor::replaceDialog);
    _cmdReplace = new Tui::ZCommandNotifier("Replace", this);
    QObject::connect(_cmdReplace, &Tui::ZCommandNotifier::activated, this, &Editor::replaceDialog);

    //InsertCharacter
    _cmdInsertCharacter = new Tui::ZCommandNotifier("InsertCharacter", this);
    QObject::connect(_cmdInsertCharacter, &Tui::ZCommandNotifier::activated, this, [this] {
            QObject::connect(new InsertCharacter(this), &InsertCharacter::characterSelected, this, [this] (QString str) {
                if (_file) {
                    _file->insertText(str);
                }
            });
        }
    );

    //Goto Line
    auto gotoLine = [this] {
        QObject::connect(new GotoLine(this), &GotoLine::lineSelected, this, [this] (QString line) {
            if (_file) {
                _file->gotoLine(line);
            }
        });
    };
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("g"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
        this, gotoLine);
    _cmdGotoLine = new Tui::ZCommandNotifier("Gotoline", this);
    QObject::connect(_cmdGotoLine, &Tui::ZCommandNotifier::activated, this, gotoLine);

    //Sort Seleced Lines
    _cmdSortSelectedLines = new Tui::ZCommandNotifier("SortSelectedLines", this);
    QObject::connect(_cmdSortSelectedLines, &Tui::ZCommandNotifier::activated, this, [this] {
        if (_file) {
            _file->sortSelecedLines();
        }
    });

    //Options
    _cmdTab = new Tui::ZCommandNotifier("Tab", this);
    QObject::connect(_cmdTab, &Tui::ZCommandNotifier::activated, this, [this] {
            if (_tabDialog) {
                _tabDialog->raise();
                _tabDialog->setVisible(true);
                _tabDialog->placeFocus()->setFocus();
            } else {
                _tabDialog = new TabDialog(this);
                if (_file) {
                    _tabDialog->updateSettings(!_file->useTabChar(), _file->tabStopDistance(), _file->eatSpaceBeforeTabs());
                }
                QObject::connect(_tabDialog, &TabDialog::convert, this, [this] (bool useTabs, int indentSize) {
                    if (_file) {
                        _file->setUseTabChar(useTabs);
                        _file->setTabStopDistance(indentSize);
                        _file->convertTabsToSpaces();
                    }
                });
                QObject::connect(_tabDialog, &TabDialog::settingsChanged, this, [this] (bool useTabs, int indentSize, bool eatSpaceBeforeTabs) {
                    if (_file) {
                        _file->setUseTabChar(useTabs);
                        _file->setTabStopDistance(indentSize);
                        _file->setEatSpaceBeforeTabs(eatSpaceBeforeTabs);
                    }
                });
            }
        }
    );

    _cmdLineNumbers = new Tui::ZCommandNotifier("LineNumber", this);
    QObject::connect(_cmdLineNumbers, &Tui::ZCommandNotifier::activated, [this] {
            if (_file) {
                _file->toggleShowLineNumbers();
            }
        }
    );

    _cmdFormatting = new Tui::ZCommandNotifier("Formatting", this);
    QObject::connect(_cmdFormatting, &Tui::ZCommandNotifier::activated, this, [this] {
            FormattingDialog *_formattingDialog = new FormattingDialog(this);
            if (_file) {
                _formattingDialog->updateSettings(_file->formattingCharacters(), _file->colorTabs(), _file->colorTrailingSpaces());
            } else {
                _formattingDialog->updateSettings(_initialFileSettings.formattingCharacters, _initialFileSettings.colorTabs, _initialFileSettings.colorSpaceEnd);
            }
            QObject::connect(_formattingDialog, &FormattingDialog::settingsChanged, this,
                         [this] (bool formattingCharacters, bool colorTabs, bool colorSpaceEnd) {
                             if (_file) {
                                 _file->setFormattingCharacters(formattingCharacters);
                                 _file->setColorTabs(colorTabs);
                                 _file->setColorTrailingSpaces(colorSpaceEnd);
                             } else {
                                 _initialFileSettings.formattingCharacters = formattingCharacters;
                                 _initialFileSettings.colorTabs = colorTabs;
                                 _initialFileSettings.colorSpaceEnd = colorSpaceEnd;
                             }
                         });
        }
    );

    _cmdBrackets = new Tui::ZCommandNotifier("Brackets", this);
    QObject::connect(_cmdBrackets, &Tui::ZCommandNotifier::activated, this, [this] {
            if (_file) {
                _file->setHighlightBracket(!_file->highlightBracket());
            }
        }
    );

#ifdef SYNTAX_HIGHLIGHTING
    _cmdSyntaxHighlight = new Tui::ZCommandNotifier("SyntaxHighlighting", this);
    QObject::connect(_cmdSyntaxHighlight, &Tui::ZCommandNotifier::activated, this, [this] {
            if (_syntaxHighlightDialog) {
                _syntaxHighlightDialog->raise();
                _syntaxHighlightDialog->setVisible(true);
                _syntaxHighlightDialog->placeFocus()->setFocus();
            } else {
                _syntaxHighlightDialog = new SyntaxHighlightDialog(this);
                QObject::connect(_syntaxHighlightDialog, &SyntaxHighlightDialog::settingsChanged, this, [this] (bool enable, QString lang) {
                    if (_file) {
                        _file->setSyntaxHighlightingActive(enable);
                        _file->setSyntaxHighlightingLanguage(lang);
                    }
                });
            }
            if (_file) {
                _syntaxHighlightDialog->updateSettings(_file->syntaxHighlightingActive(), _file->syntaxHighlightingLanguage());
            }
        }
    );
#endif

    QObject::connect(new Tui::ZCommandNotifier("Theme", this), &Tui::ZCommandNotifier::activated,
         [&] {
            new ThemeDialog(this);
        }
    );
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forMnemonic("x"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
                     this, &Editor::showCommandLine);

    QObject::connect(new Tui::ZCommandNotifier("NextWindow", this), &Tui::ZCommandNotifier::activated,
        [this] {
            activateNextWindow();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("PreviousWindow", this), &Tui::ZCommandNotifier::activated,
        [this] {
            activatePreviousWindow();
        }
    );

    _cmdTileVert = new Tui::ZCommandNotifier("TileVert", this);
    QObject::connect(_cmdTileVert, &Tui::ZCommandNotifier::activated, this, [this] {
            _mdiLayout->setMode(MdiLayout::LayoutMode::TileV);
        }
    );

    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, "v", {}), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
        [this] {
            _mdiLayout->setMode(MdiLayout::LayoutMode::TileV);
        }
    );

    _cmdTileHorz = new Tui::ZCommandNotifier("TileHorz", this);
    QObject::connect(_cmdTileHorz, &Tui::ZCommandNotifier::activated, this, [this] {
            _mdiLayout->setMode(MdiLayout::LayoutMode::TileH);
        }
    );

    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, "h", {}), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
        [this] {
            _mdiLayout->setMode(MdiLayout::LayoutMode::TileH);
        }
    );

    _cmdTileFull = new Tui::ZCommandNotifier("TileFull", this);
    QObject::connect(_cmdTileFull, &Tui::ZCommandNotifier::activated, this, [this] {
            _mdiLayout->setMode(MdiLayout::LayoutMode::Base);
        }
    );

    ensureWindowCommands(24);

    enableFileCommands(false);

    QObject::connect(new Tui::ZCommandNotifier("About", this), &Tui::ZCommandNotifier::activated,
        [this] {
            new AboutDialog(this);
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("TerminalDiagnostics", this), &Tui::ZCommandNotifier::activated, this,
        [this] {
            new Tui::ZTerminalDiagnosticsDialog(this);
        }
    );

    // Background
    setFillChar(u'▒');

    _commandLineWidget = new CommandLineWidget(this);
    _commandLineWidget->setVisible(false);
    QObject::connect(_commandLineWidget, &CommandLineWidget::dismissed, this, &Editor::commandLineDismissed);
    QObject::connect(_commandLineWidget, &CommandLineWidget::execute, this, &Editor::commandLineExecute);

    _statusBar = new StatusBar(this);

    Tui::ZVBoxLayout *rootLayout = new Tui::ZVBoxLayout();
    setLayout(rootLayout);
    rootLayout->addWidget(menu);
    _mdiLayout = new MdiLayout();
    rootLayout->add(_mdiLayout);
    rootLayout->addWidget(_commandLineWidget);
    rootLayout->addWidget(_statusBar);
}

void Editor::terminalChanged() {
    setupUi();

    QObject::connect(terminal(), &Tui::ZTerminal::focusChanged, this, [this] {
        Tui::ZWidget *w = terminal()->focusWidget();
        while (w && !qobject_cast<FileWindow*>(w)) {
            w = w->parentWidget();
        }
        if (qobject_cast<FileWindow*>(w)) {
            _win = qobject_cast<FileWindow*>(w);
            if (_file != _win->getFileWidget()) {
                _file = _win->getFileWidget();
                enableFileCommands(true);
                if (_tabDialog) {
                    _tabDialog->updateSettings(!_file->useTabChar(), _file->tabStopDistance(), _file->eatSpaceBeforeTabs());
                }
                if (_syntaxHighlightDialog) {
                    _syntaxHighlightDialog->updateSettings(_file->syntaxHighlightingActive(), _file->syntaxHighlightingLanguage());
                }
            }
            _mux.selectInput(_win);
        }
    });

    Tui::ZPendingKeySequenceCallbacks pending;
    pending.setPendingSequenceStarted([this] {
        _pendingKeySequenceTimer.setInterval(2000);
        _pendingKeySequenceTimer.setSingleShot(true);
        _pendingKeySequenceTimer.start();
    });
    pending.setPendingSequenceFinished([this] (bool matched) {
        (void)matched;
        _pendingKeySequenceTimer.stop();
        if (_pendingKeySequence) {
            _pendingKeySequence->deleteLater();
            _pendingKeySequence = nullptr;
        }
    });
    terminal()->registerPendingKeySequenceCallbacks(pending);

    QObject::connect(&_pendingKeySequenceTimer, &QTimer::timeout, this, [this] {
        _pendingKeySequence = new Tui::ZWindow(this);
        auto *layout = new Tui::ZWindowLayout();
        _pendingKeySequence->setLayout(layout);
        layout->setCentralWidget(new Tui::ZTextLine("Incomplete key sequence. Press 2nd key.", _pendingKeySequence));
        _pendingKeySequence->setGeometry({{0,0}, _pendingKeySequence->sizeHint()});
        _pendingKeySequence->setDefaultPlacement(Qt::AlignCenter);
    });
    setupSearchDialogs();

    for (auto &action: _startActions) {
        action();
    }
}

FileWindow *Editor::createFileWindow() {
    FileWindow *win = new FileWindow(this);
    File *file = win->getFileWidget();

    _mux.connect(win, file, &File::cursorPositionChanged, _statusBar, &StatusBar::cursorPosition, 0, 0, 0, 0);
    _mux.connect(win, file, &File::scrollPositionChanged, _statusBar, &StatusBar::scrollPosition, 0, 0);
    _mux.connect(win, file, &File::modifiedChanged, _statusBar, &StatusBar::setModified, false);
    _mux.connect(win, win, &FileWindow::readFromStandadInput, _statusBar, &StatusBar::readFromStandardInput, false);
    _mux.connect(win, win, &FileWindow::followStandadInput, _statusBar, &StatusBar::followStandardInput, false);
    _mux.connect(win, file, &File::followStandardInputChanged, _statusBar, &StatusBar::followStandardInput, false);
    _mux.connect(win, file, &File::writableChanged, _statusBar, &StatusBar::setWritable, true);
    _mux.connect(win, file->document(), &Tui::ZDocument::crLfModeChanged, _statusBar, &StatusBar::crlfMode, false);
    _mux.connect(win, file, &File::selectModeChanged, _statusBar, &StatusBar::modifiedSelectMode, false);
    _mux.connect(win, file, &File::searchCountChanged, _statusBar, &StatusBar::searchCount, -1);
    _mux.connect(win, file, &File::searchTextChanged, _statusBar, &StatusBar::searchText, QString());
    _mux.connect(win, file, &File::searchVisibleChanged, _statusBar, &StatusBar::searchVisible, false);
    _mux.connect(win, file, &File::overwriteModeChanged, _statusBar, &StatusBar::overwrite, false);
    _mux.connect(win, win, &FileWindow::fileChangedExternally, _statusBar, &StatusBar::fileHasBeenChangedExternally, false);
    _mux.connect(win, file, &File::syntaxHighlightingEnabledChanged, _statusBar, &StatusBar::syntaxHighlightingEnabled, false);
    _mux.connect(win, file, &File::syntaxHighlightingLanguageChanged, _statusBar, &StatusBar::language, QString());

    _allWindows.append(win);
    ensureWindowCommands(_allWindows.size());

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
        if (_allWindows.size() == 0) {
            enableFileCommands(false);
        }
    });

    if (_win) {
        file->setTabStopDistance(_file->tabStopDistance());
        file->setShowLineNumbers(_file->showLineNumbers());
        file->setUseTabChar(_file->useTabChar());
        file->setEatSpaceBeforeTabs(_file->eatSpaceBeforeTabs());
        file->setFormattingCharacters(_file->formattingCharacters());
        file->setColorTabs(_file->colorTabs());
        file->setColorTrailingSpaces(_file->colorTrailingSpaces());
        win->setWrap(_file->wordWrapMode());
        file->setRightMarginHint(_file->rightMarginHint());
        file->setHighlightBracket(_file->highlightBracket());
        file->setAttributesFile(_file->attributesFile());
        file->setSyntaxHighlightingTheme(_initialFileSettings.syntaxHighlightingTheme);
        file->setSyntaxHighlightingActive(_file->syntaxHighlightingActive());
    } else {
        file->setTabStopDistance(_initialFileSettings.tabSize);
        file->setShowLineNumbers(_initialFileSettings.showLineNumber);
        file->setUseTabChar(_initialFileSettings.tabOption);
        file->setEatSpaceBeforeTabs(_initialFileSettings.eatSpaceBeforeTabs);
        file->setFormattingCharacters(_initialFileSettings.formattingCharacters);
        file->setColorTabs(_initialFileSettings.colorTabs);
        file->setColorTrailingSpaces(_initialFileSettings.colorSpaceEnd);
        win->setWrap(_initialFileSettings.wrap);
        file->setRightMarginHint(_initialFileSettings.rightMarginHint);
        file->setHighlightBracket(_initialFileSettings.highlightBracket);
        file->setAttributesFile(_initialFileSettings.attributesFile);
        file->setSyntaxHighlightingTheme(_initialFileSettings.syntaxHighlightingTheme);
        file->setSyntaxHighlightingActive(!_initialFileSettings.disableSyntaxHighlighting);
    }

    return win;
}

QVector<Tui::ZMenuItem> Editor::createWindowMenu() {
    QVector<Tui::ZMenuItem> subitems = {
        {"<m>N</m>ext", "F6", "NextWindow", {}},
        {"<m>P</m>revious", "Shift-F6", "PreviousWindow", {}},
        {"Tile <m>V</m>ertically", "", "TileVert", {}},
        {"Tile <m>H</m>orizontally", "", "TileHorz", {}},
        {"Tile <m>F</m>ullscreen", "", "TileFull", {}},
    };

    if (_allWindows.size()) {
        subitems.append(Tui::ZMenuItem{});
        for (int i = 0; i < _allWindows.size(); i++) {
            if (i < 9) {
                subitems.append(Tui::ZMenuItem{_allWindows[i]->getFileWidget()->getFilename(),
                                         QStringLiteral("Alt-%0").arg(QString::number(i + 1)),
                                         QStringLiteral("SwitchToWindow%0").arg(QString::number(i + 1)),
                                         {}});
            } else {
                subitems.append(Tui::ZMenuItem{_allWindows[i]->getFileWidget()->getFilename(),
                                         "",
                                         QStringLiteral("SwitchToWindow%0").arg(QString::number(i + 1)),
                                         {}});
            }
        }

    }

    return subitems;
}

void Editor::setupSearchDialogs() {
    auto searchCancled = [this] {
        if (_file) {
            _file->setSearchText("");
            _file->clearSelection();
        }
    };

    auto searchCaseSensitiveChanged = [this](bool value) {
        if (_file) {
            if(value) {
                _file->setSearchCaseSensitivity(Qt::CaseSensitive);
            } else {
                _file->setSearchCaseSensitivity(Qt::CaseInsensitive);
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
            if (text.size() == 0) {
                _file->setSearchText("");
                _file->clearSelection();
            } else {
                _file->setSearchText(text);
                _file->setSearchDirection(forward);
                if(forward) {
                    _file->setCursorPosition({_file->cursorPosition().codeUnit - text.size(), _file->cursorPosition().line});
                } else {
                    _file->setCursorPosition({_file->cursorPosition().codeUnit + text.size(), _file->cursorPosition().line});
                }
                _file->runSearch(false);
            }
        }
    };

    auto searchNext = [this](QString text, bool forward) {
        if (_file) {
            _file->setSearchText(text);
            _file->setSearchDirection(forward);
            _file->runSearch(false);
        }
    };

    auto regex = [this] (bool regex) {
         if (_file) {
            _file->setRegex(regex);
         }
    };

    _searchDialog = new SearchDialog(this);
    QObject::connect(_searchDialog, &SearchDialog::searchCanceled, this, searchCancled);
    QObject::connect(_searchDialog, &SearchDialog::searchCaseSensitiveChanged, this, searchCaseSensitiveChanged);
    QObject::connect(_searchDialog, &SearchDialog::searchWrapChanged, this, searchWrapChanged);
    QObject::connect(_searchDialog, &SearchDialog::searchDirectionChanged, this, searchForwardChanged);
    QObject::connect(_searchDialog, &SearchDialog::liveSearch, this, liveSearch);
    QObject::connect(_searchDialog, &SearchDialog::searchFindNext, this, searchNext);
    QObject::connect(_searchDialog, &SearchDialog::searchRegexChanged, this, regex);

    _replaceDialog = new SearchDialog(this, true);
    QObject::connect(_replaceDialog, &SearchDialog::searchCanceled, this, searchCancled);
    QObject::connect(_replaceDialog, &SearchDialog::searchCaseSensitiveChanged, this, searchCaseSensitiveChanged);
    QObject::connect(_replaceDialog, &SearchDialog::searchWrapChanged, this, searchWrapChanged);
    QObject::connect(_replaceDialog, &SearchDialog::searchDirectionChanged, this, searchForwardChanged);
    QObject::connect(_replaceDialog, &SearchDialog::liveSearch, this, liveSearch);
    QObject::connect(_replaceDialog, &SearchDialog::searchFindNext, this, searchNext);
    QObject::connect(_replaceDialog, &SearchDialog::searchRegexChanged, this, regex);
    QObject::connect(_replaceDialog, &SearchDialog::searchReplace, this, [this] (QString text, QString replacement, bool forward) {
        if (_file) {
            if (_file->isSearchMatchSelected()) {
                _file->setSearchText(text);
                _file->setReplaceText(replacement);
                _file->setReplaceSelected();
                _file->setSearchDirection(forward);
            }
            _file->runSearch(false);
        }
    });
    QObject::connect(_replaceDialog, &SearchDialog::searchReplaceAll, this, [this] (QString text, QString replacement) {
        if (_file) {
            _file->replaceAll(text, replacement);
        }
    });
}

void Editor::ensureWindowCommands(int count) {
    if (count <= _windowCommandsCreated) {
        return;
    }

    for (int i = _windowCommandsCreated; i < count; i++) {
        auto code = [this, windowNumber = i] {
            if (windowNumber < _allWindows.size()) {
                auto *win = _allWindows[windowNumber];
                raiseOnActivate(win);
                auto *widget = win->placeFocus();
                if (widget) {
                    widget->setFocus();
                }
            }
        };

        QObject::connect(new Tui::ZCommandNotifier(QStringLiteral("SwitchToWindow%0").arg(QString::number(i + 1)), this),
                         &Tui::ZCommandNotifier::activated, code);
        if (i < 9) {
            QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forMnemonic(QString::number(i + 1)), this, Qt::ApplicationShortcut),
                         &Tui::ZShortcut::activated, this, code);
        }
    }
    _windowCommandsCreated = count;
}

void Editor::enableFileCommands(bool enable) {
    _cmdInsertCharacter->setEnabled(enable);
    _cmdGotoLine->setEnabled(enable);
    _cmdSortSelectedLines->setEnabled(enable);
    _cmdTab->setEnabled(enable);
    _cmdLineNumbers->setEnabled(enable);
    //_cmdFormatting->setEnabled(enable);
    _cmdBrackets->setEnabled(enable);
    _cmdSyntaxHighlight->setEnabled(enable);
    _cmdTileVert->setEnabled(enable);
    _cmdTileHorz->setEnabled(enable);
    _cmdTileFull->setEnabled(enable);
    _cmdSearch->setEnabled(enable);
    _cmdReplace->setEnabled(enable);
}

void Editor::searchDialog() {
    if (_file) {
        _searchDialog->open();
        _searchDialog->setSearchText(_file->selectedText());
    }
}

void Editor::replaceDialog() {
    if (_file) {
        _replaceDialog->open();
       _replaceDialog->setSearchText(_file->selectedText());
    }
}

void Editor::newFile(QString filename) {
    FileWindow *win = createFileWindow();
    if (filename.size()) {
        win->getFileWidget()->newText(filename);
    }
    _mdiLayout->addWindow(win);
    win->getFileWidget()->setFocus();
}

void Editor::openFileMenu() {
    if (_file) {
        openFileDialog(_file->getFilename());
    } else {
        openFileDialog();
    }
}

void Editor::gotoLineInCurrentFile(QString lineInfo) {
    if (_file) {
        _file->gotoLine(lineInfo);
    }
}

void Editor::followInCurrentFile(bool follow) {
    if (_win) {
        _win->setFollow(follow);
    }
}

void Editor::watchPipe() {
    if (_win) {
        _win->watchPipe();
    }
}

void Editor::setStartActions(std::vector<std::function<void ()>> actions) {
    _startActions = actions;
}

FileWindow* Editor::openFile(QString fileName) {
    QFileInfo filenameInfo(fileName);
    QString absFileName = filenameInfo.absoluteFilePath();
    if (_nameToWindow.contains(absFileName)) {
        _nameToWindow.value(absFileName)->getFileWidget()->setFocus();
        return _nameToWindow.value(absFileName);
    }

    if (_win && !_win->getFileWidget()->isModified() && _win->getFileWidget()->getFilename() == "NEWFILE") {
        _win->openFile(fileName);
        return _win;
    } else {
        FileWindow *win = createFileWindow();
        _mdiLayout->addWindow(win);
        win->getFileWidget()->setFocus();
        win->openFile(fileName);
        return win;
    }
}

void Editor::openFileDialog(QString path) {
    OpenDialog *openDialog = new OpenDialog(this, path);
    QObject::connect(openDialog, &OpenDialog::fileSelected, this, [this] (QString fileName) {
        openFile(fileName);
    });
}

void Editor::quitImpl(int i) {
    if (i >= _allWindows.size()) {
        // delete all windows so they can signal work in QtConcurrent to quit also.
        for (auto* win: _allWindows) {
            win->deleteLater();
        }
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
        ConfirmSave *quitDialog = new ConfirmSave(this, file->getFilename(),
                                                  file->isNewFile() ? ConfirmSave::QuitUnnamed : ConfirmSave::Quit,
                                                  file->getWritable());

        QObject::connect(quitDialog, &ConfirmSave::discardSelected, [=] {
            quitDialog->deleteLater();
            handleNext();
        });

        QObject::connect(quitDialog, &ConfirmSave::saveSelected, [=] {
            quitDialog->deleteLater();
            SaveDialog *q = win->saveOrSaveas();
            if (q) {
                QObject::connect(q, &SaveDialog::fileSelected, this, handleNext);
            } else {
                handleNext();
            }
        });

        QObject::connect(quitDialog, &ConfirmSave::rejected, [=] {
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
                                 {"chr.fgbehindText", { 0xdd, 0xdd, 0xdd}},
                                 {"chr.trackBgColor", { 0x80, 0x80, 0x80}},
                                 {"chr.thumbBgColor", { 0x69, 0x69, 0x69}},
                                 {"chr.trackFgColor", Tui::Colors::brightWhite},
                                 {"chr.linenumberFg", { 0xdd, 0xdd, 0xdd}},
                                 {"chr.linenumberBg", { 0x22, 0x22, 0x22}},
                                 {"chr.statusbarBg", Tui::Colors::darkGray},
                                 {"chr.editFg", {0xff, 0xff, 0xff}},
                                 {"chr.editBg", {0x0, 0x0, 0x0}},
                             });
        setPalette(tmpPalette);
        if (_initialFileSettings.syntaxHighlightingTheme == "chr-blackbg"
                || _initialFileSettings.syntaxHighlightingTheme == "chr-bluebg") {
            _initialFileSettings.syntaxHighlightingTheme = "chr-blackbg";
        }
    } else if (theme == Theme::classic) {
        Tui::ZPalette tmpPalette = Tui::ZPalette::classic();
        tmpPalette.setColors({
                                 {"chr.fgbehindText", { 0xdd, 0xdd, 0xdd}},
                                 {"chr.trackBgColor", { 0,    0,    0x80}},
                                 {"chr.thumbBgColor", { 0,    0,    0xd9}},
                                 {"chr.trackFgColor", Tui::Colors::brightWhite},
                                 {"chr.linenumberFg", { 0xdd, 0xdd, 0xdd}},
                                 {"chr.linenumberBg", { 0,    0,    0x80}},
                                 {"chr.statusbarBg", {0, 0xaa, 0xaa}},
                                 {"chr.editFg", {0xff, 0xff, 0xff}},
                                 {"chr.editBg", {0, 0, 0xaa}},
                                 {"root.fg", {0xaa, 0xaa, 0xaa}},
                                 {"root.bg", {0, 0, 0xaa}}
                             });
        setPalette(tmpPalette);
        if (_initialFileSettings.syntaxHighlightingTheme == "chr-blackbg"
                || _initialFileSettings.syntaxHighlightingTheme == "chr-bluebg") {
            _initialFileSettings.syntaxHighlightingTheme = "chr-bluebg";
        }
    }
    for (auto window: _allWindows) {
        window->getFileWidget()->setSyntaxHighlightingTheme(_initialFileSettings.syntaxHighlightingTheme);
    }
}

void Editor::setInitialFileSettings(const Settings &initial) {
    _initialFileSettings = initial;
}

void Editor::showCommandLine() {
    _statusBar->setVisible(false);
    _commandLineWidget->setVisible(true);
    _commandLineWidget->grabKeyboard();
}

void Editor::commandLineDismissed() {
    _statusBar->setVisible(true);
    _commandLineWidget->setVisible(false);
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
        // ignoring result because this is a interactive shell
        (void)!system(qgetenv("SHELL"));
        term->unpauseOperation();
    }
}
