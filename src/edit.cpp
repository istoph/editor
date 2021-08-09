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


    //Quit
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("q"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::quit);
    QObject::connect(new Tui::ZCommandNotifier("Quit", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::quit);

    //Search
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, 0), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
          [&] {
            _file->runSearch(false);
          });
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, Qt::ShiftModifier), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
          [&] {
            _file->runSearch(true);
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
            new InsertCharacter(this, _file);
        }
    );

    //Goto Line
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("g"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
        this, [&] {
            new GotoLine(this, _file);
    });
    QObject::connect(new Tui::ZCommandNotifier("Gotoline", this), &Tui::ZCommandNotifier::activated,
         [&] {
            new GotoLine(this, _file);
        }
    );

    //Options
    QObject::connect(new Tui::ZCommandNotifier("Tab", this), &Tui::ZCommandNotifier::activated,
        [&] {
            new TabDialog(_file, this);
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("LineNumber", this), &Tui::ZCommandNotifier::activated,
        [&] {
            _file->toggleLineNumber();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Formatting", this), &Tui::ZCommandNotifier::activated,
         [&] {
            _file->setFormattingCharacters(!_file->getformattingCharacters());
            update();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Brackets", this), &Tui::ZCommandNotifier::activated,
         [&] {
            _file->setHighlightBracket(!_file->getHighlightBracket());
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

    _win = new FileWindow(this);
    _file = _win->getFileWidget();

    _commandLineWidget = new CommandLineWidget(this);
    _commandLineWidget->setVisible(false);
    connect(_commandLineWidget, &CommandLineWidget::dismissed, this, &Editor::commandLineDismissed);
    connect(_commandLineWidget, &CommandLineWidget::execute, this, &Editor::commandLineExecute);

    _statusBar = new StatusBar(this);
    connect(_file, &File::cursorPositionChanged, _statusBar, &StatusBar::cursorPosition);
    connect(_file, &File::scrollPositionChanged, _statusBar, &StatusBar::scrollPosition);
    connect(_file, &File::modifiedChanged, _statusBar, &StatusBar::setModified);
    connect(_win, &FileWindow::readFromStandadInput, _statusBar, &StatusBar::readFromStandardInput);
    connect(_win, &FileWindow::followStandadInput, _statusBar, &StatusBar::followStandardInput);
    connect(_file, &File::setWritable, _statusBar, &StatusBar::setWritable);
    connect(_file, &File::msdosMode, _statusBar, &StatusBar::msdosMode);
    connect(_file, &File::modifiedSelectMode, _statusBar, &StatusBar::modifiedSelectMode);
    connect(_file, &File::emitSearchCount, _statusBar, &StatusBar::searchCount);
    connect(_file, &File::emitSearchText, _statusBar, &StatusBar::searchText);
    connect(_file, &File::emitOverwrite, _statusBar, &StatusBar::overwrite);
    connect(_win, &FileWindow::fileChangedExternally, _statusBar, &StatusBar::fileHasBeenChangedExternally);


    VBoxLayout *rootLayout = new VBoxLayout();
    setLayout(rootLayout);
    rootLayout->addWidget(menu);
    rootLayout->addWidget(_win);
    rootLayout->addWidget(_commandLineWidget);
    rootLayout->addWidget(_statusBar);

    _searchDialog = new SearchDialog(this, _file);
    QObject::connect(new Tui::ZCommandNotifier("search", this), &Tui::ZCommandNotifier::activated,
                     _searchDialog, &SearchDialog::open);

    _replaceDialog = new SearchDialog(this, _file, true);
    QObject::connect(new Tui::ZCommandNotifier("replace", this), &Tui::ZCommandNotifier::activated,
                     _replaceDialog, &SearchDialog::open);
}


void Editor::searchDialog() {
    _searchDialog->open();
    _searchDialog->setSearchText(_file->getSelectText());
}

void Editor::replaceDialog() {
    _replaceDialog->open();
    _replaceDialog->setSearchText(_file->getSelectText());
}

void Editor::newFileMenue() {
    if(_file->isModified()) {
        ConfirmSave *confirmDialog = new ConfirmSave(this, _file->getFilename(), ConfirmSave::New);
        QObject::connect(confirmDialog, &ConfirmSave::exitSelected, [=]{
            delete confirmDialog;
            _win->newFile("");
        });

        QObject::connect(confirmDialog, &ConfirmSave::saveSelected, [=]{
            _win->saveOrSaveas();
            delete confirmDialog;
            _win->newFile("");
        });
        QObject::connect(confirmDialog, &ConfirmSave::rejected, [=]{
            delete confirmDialog;
        });
    } else {
        _win->newFile("");
    }
}

void Editor::openFileMenue() {
    if(_file->isModified()) {
        ConfirmSave *confirmDialog = new ConfirmSave(this, _file->getFilename(), ConfirmSave::Open);
        QObject::connect(confirmDialog, &ConfirmSave::exitSelected, [=]{
            delete confirmDialog;
            openFileDialog();
        });

        QObject::connect(confirmDialog, &ConfirmSave::saveSelected, [=]{
            _win->saveOrSaveas();
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


void Editor::openFileDialog(QString path) {
    OpenDialog *openDialog = new OpenDialog(this, path);
    connect(openDialog, &OpenDialog::fileSelected, _win, &FileWindow::openFile);
}

QObject * Editor::facet(const QMetaObject metaObject) {
    if (metaObject.className()  == Clipboard::staticMetaObject.className()) {
        return &_clipboard;
    } else {
        return ZRoot::facet(metaObject);
    }
}

void Editor::quit() {
    _file->writeAttributes();
    if(_file->isModified()) {
        ConfirmSave *quitDialog = new ConfirmSave(this, _file->getFilename(), ConfirmSave::Quit, _file->getWritable());

        QObject::connect(quitDialog, &ConfirmSave::exitSelected, [=]{
            QCoreApplication::instance()->quit();
        });

        QObject::connect(quitDialog, &ConfirmSave::saveSelected, [=]{
            quitDialog->deleteLater();
            SaveDialog *q = _win->saveOrSaveas();
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
