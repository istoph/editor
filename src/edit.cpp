#include "edit.h"
#include "tabdialog.h"
#include <QLoggingCategory>
#include <Tui/ZSimpleFileLogger.h>

Editor::Editor() {
    ensureCommandManager();

    Menubar *menu = new Menubar(this);
    menu->setItems({
                      { "<m>F</m>ile", "", {}, {
                            { "<m>N</m>ew", "Ctrl-N", "New", {}},
                            { "<m>O</m>pen...", "Ctrl-O", "Open", {}},
                            { "<m>S</m>ave", "Ctrl-S", "Save", {}},
                            { "Save <m>a</m>s...", "Ctrl-Shift-S", "SaveAs", {}},
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
                            { "<m>G</m>oto line", "Ctrl-G", "Gotoline", {}}
                        }
                      },
                      { "<m>O</m>ptions", "", {}, {
                                 { "<m>T</m>ab", "", "Tab", {}},
                                 { "<m>F</m>ormatting characters", "", "Formatting", {}},
                                 { "<m>W</m>rap long lines", "", "Wrap", {}},
                                 { "Following standard input", "", "Following", {}},
                                 { "<m>H</m>ighlight Brackets", "", "Brackets", {}}
                             }
                           },
                      { "Hel<m>p</m>", "", {}, {
                            { "<m>A</m>bout", "", "About", {}}
                        }
                      }
                   });

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
    connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("s"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::saveOrSaveas);
    QObject::connect(new Tui::ZCommandNotifier("Save", this), &Tui::ZCommandNotifier::activated,
                    this, &Editor::saveOrSaveas);

    //SAVE AS
    //shortcut dose not work in vte, konsole, ...
    connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("S"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::saveFileDialog);
    QObject::connect(new Tui::ZCommandNotifier("SaveAs", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::saveFileDialog);

    //QUIT
    connect(new Tui::ZShortcut(Tui::ZKeySequence::forShortcut("q"), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
            this, &Editor::quit);
    QObject::connect(new Tui::ZCommandNotifier("Quit", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::quit);

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
    QObject::connect(new Tui::ZCommandNotifier("Undo", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->undo();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("Redo", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->redo();
        }
    );

    //SEARCH
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, 0), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
          [&] {
            file->searchNext();
          });
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, Qt::ShiftModifier), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
          [&] {
            file->searchPrevious();
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

    QObject::connect(new Tui::ZCommandNotifier("Gotoline", this), &Tui::ZCommandNotifier::activated,
         [&] {
            new GotoLine(this, file);
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Tab", this), &Tui::ZCommandNotifier::activated,
        [&] {
            new TabDialog(file, this);
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Formatting", this), &Tui::ZCommandNotifier::activated,
         [&] {
            if (file->getformattingCharacters())
                file->setFormattingCharacters(false);
            else
                file->setFormattingCharacters(true);
            update();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Wrap", this), &Tui::ZCommandNotifier::activated,
         [&] {
            setWrap(!file->getWrapOption());
        }
    );
    //TODO das muss grau werden wenn stdin nicht aktiv ist
    QObject::connect(new Tui::ZCommandNotifier("Following", this), &Tui::ZCommandNotifier::activated,
         [&] {
            setFollow(!getFollow());
            file->followStandardInput(getFollow());
        }
    );

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

    StatusBar *s = new StatusBar(this);
    connect(file, &File::cursorPositionChanged, s, &StatusBar::cursorPosition);
    connect(file, &File::scrollPositionChanged, s, &StatusBar::scrollPosition);
    connect(file, &File::modifiedChanged, s, &StatusBar::setModified);
    connect(this, &Editor::readFromStandadInput, s, &StatusBar::readFromStandardInput);
    connect(this, &Editor::followStandadInput, s, &StatusBar::followStandardInput);
    connect(file, &File::setWritable, s, &StatusBar::setWritable);

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
    rootLayout->addWidget(s);

    _searchDialog = new SearchDialog(this, file);
    QObject::connect(new Tui::ZCommandNotifier("search", this), &Tui::ZCommandNotifier::activated,
                     _searchDialog, &SearchDialog::open);

    _replaceDialog = new SearchDialog(this, file, true);
    QObject::connect(new Tui::ZCommandNotifier("search", this), &Tui::ZCommandNotifier::activated,
                     _replaceDialog, &SearchDialog::open);
}

void Editor::watchPipe() {
    _pipeSocketNotifier = new QSocketNotifier(0, QSocketNotifier::Type::Read, this);
    QObject::connect(_pipeSocketNotifier, &QSocketNotifier::activated, this, &Editor::inputPipeReadable);
}

void Editor::searchDialog() {
    _searchDialog->open();
    _searchDialog->setSearchText(file->getSelectText());
}

void Editor::replaceDialog() {
    _replaceDialog->open();
    _replaceDialog->setSearchText(file->getSelectText());
}

void Editor::newFile(QString filename) {
    win->setWindowTitle(filename);
    file->newText();
    file->setFilename(filename);
}

void Editor::openFile(QString filename) {
    win->setWindowTitle(filename);
    //reset couser position
    file->newText();
    file->setFilename(filename);
    file->openText();
}
void Editor::saveFile(QString filename) {
    file->setFilename(filename);
    if(file->saveText()) {
        win->setWindowTitle(filename);
        win->update();
    } else {
        Alert *e = new Alert(this);
        e->setGeometry({10,5,60,15});
        e->setVisible(true);
        e->setFocus();
    }

}
void Editor::openFileDialog() {
    OpenDialog * openDialog = new OpenDialog(this);
    connect(openDialog, &OpenDialog::fileSelected, this, &Editor::openFile);
}

SaveDialog * Editor::saveFileDialog() {
    SaveDialog * saveDialog = new SaveDialog(this);
    connect(saveDialog, &SaveDialog::fileSelected, this, &Editor::saveFile);
    return saveDialog;
}

SaveDialog * Editor::saveOrSaveas() {
    if (file->newfile) {
        SaveDialog *q = saveFileDialog();
        return q;
    } else {
        if(!file->saveText()) {
            Alert *e = new Alert(this);
            e->addPaletteClass("red");
            e->setWindowTitle("Error");
            e->setMarkup("file could not be saved"); //Die Datei konnte nicht gespeicher werden!
            e->setGeometry({15,5,50,5});
            e->setVisible(true);
            e->setFocus();
        }
        return nullptr;
    }
}

void Editor::quit() {
    if(file->isModified()) {
        ConfirmSave *quitDialog = new ConfirmSave(this, file->getFilename(), ConfirmSave::Quit);

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
            file->appendLine(QString::fromUtf8(_pipeLineBuffer));
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
            file->appendLine(QString::fromUtf8(_pipeLineBuffer.left(index)));
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
    followStandadInput(_follow);
}

bool Editor::getFollow() {
    return _follow;
}

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("chr");
    QCoreApplication::setApplicationVersion("%VERSION%");

    QCommandLineParser parser;
    parser.setApplicationDescription("chr.edit");
    parser.addHelpOption();
    parser.addVersionOption();

    // Line Number
    QCommandLineOption numberOption({"n","number"},
                                    QCoreApplication::translate("main", "The cursor will be positioned on line \"num\".  If \"num\" is missing, the cursor will be positioned on the last line."),
                                    QCoreApplication::translate("main", "num"));
    parser.addOption(numberOption);

    QCommandLineOption append({"a","append"},
                              QCoreApplication::translate("main", "Only with read from standard input, then append to a file"),
                              QCoreApplication::translate("main", "file"));
    parser.addOption(append);

    QCommandLineOption follow({"f","follow"},QCoreApplication::translate("main", "Only with read from standard input, then follow the input stream"));
    parser.addOption(follow);

    // Big Files
    QCommandLineOption bigOption("b",QCoreApplication::translate("main", "Open bigger files then 1MB"));
    parser.addOption(bigOption);

    // Wrap Lines
    QCommandLineOption wraplines("w",QCoreApplication::translate("main", "Wrap log lines"));
    parser.addOption(wraplines);

    // Config File
    QCommandLineOption configOption({"c","config"},
                                    QCoreApplication::translate("main", "Load customized config file. The default if it exist is ~/.config/chr"),
                                    QCoreApplication::translate("main", "config"));
    parser.addOption(configOption);

    // Dokument
    parser.addPositionalArgument("file", QCoreApplication::translate("main", "The file to open."));
    parser.process(app);

    parser.parse(QCoreApplication::arguments());
    const QStringList args = parser.positionalArguments();
    QTextStream out(stdout);

    // START EDITOR
    Tui::ZTerminal terminal;
    Editor *root = new Editor();
    terminal.setMainWidget(root);

    // READ CONFIG FILE AND SET DEFAULT OPTIONS
    QString configDir = "";
    if(parser.isSet(configOption)) {
        configDir = parser.value(configOption);
    } else {
        //default
        configDir = QDir::homePath() +"/.config/chr";
    }

    // Default settings
    QSettings * qsettings = new QSettings(configDir, QSettings::IniFormat);
    int tabsize = qsettings->value("tabsize","4").toInt();
    root->file->setTabsize(tabsize);

    bool tab = qsettings->value("tab","true").toBool();
    root->file->setTabOption(tab);

    bool fb = qsettings->value("formatting_characters","1").toBool();
    root->file->setFormattingCharacters(fb);

    bool bigfile = qsettings->value("bigfile", "false").toBool();

    bool wl = qsettings->value("wrap_lines","false").toBool();
    root->setWrap(wl || parser.isSet(wraplines));

    bool hb = qsettings->value("highlight_bracket","false").toBool();
    root->file->setHighlightBracket(hb);

    QString logfile = qsettings->value("logfile", "").toString();
    if (logfile.isEmpty()) {
        QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, false);
    } else {
        Tui::ZSimpleFileLogger::install(logfile);
    }
    qDebug("chr starting");

    // OPEN FILE
    int lineNumber = 0, lineChar = 0;
    if(!args.isEmpty()) {
        QStringList p = args;
        if (p.first().mid(0,1) == "+") {
            QString n = p.first().mid(1);
            QStringList list1 = n.split(',');
            if(list1.count()>0) {
                lineNumber = list1[0].toInt();
            }
            if(list1.count()>1) {
                lineChar = list1[1].toInt() -1;
            }
            p.removeFirst();
        }
        if (!p.isEmpty()) {
            if (p.first() != "-") {
                QFileInfo datei(p.first());
                if(datei.isReadable()) {
                    if(datei.size() > 1024000 && !parser.isSet(bigOption) && !bigfile) {
                        //TODO: warn dialog
                        out << "The file is bigger then 1MB ("<< datei.size() <<"). Please start with -b for big files.\n";
                        return 0;
                    }
                }
                root->openFile(datei.absoluteFilePath());
            } else {
                if(parser.isSet(append)) {
                    root->openFile(parser.value(append));
                } else {
                    root->newFile();
                }
                root->watchPipe();
            }
        } else {
            out << "Got file offset without file name.\n";
            return 0;
        }
    } else {
        root->newFile();
    }

    //Goto Line
    if (lineNumber > 0 || lineChar > 0) {
        root->file->gotoline(lineNumber,lineChar);
    } else if(parser.isSet(numberOption)) {
        QString n = parser.value(numberOption);
        QStringList list1 = n.split(',');
        if(list1.count()>0) {
            lineNumber = list1[0].toInt();
        }
        if(list1.count()>1) {
            lineChar = list1[1].toInt() -1;
        }
        root->file->gotoline(lineNumber,lineChar);
    }

    if(parser.isSet(follow)) {
        root->file->followStandardInput(true);
    }

    root->file->setFocus();
    root->file->setCursorStyle(Tui::CursorStyle::Underline);

    app.exec();
    return 0;
}
