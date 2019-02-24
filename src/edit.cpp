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
                            { "<m>S</m>earch", "Ctrl-F", "Search", {}},
                            { "<m>R</m>eplace", "Ctrl-R", "Replace", {}},
                            {},
                            { "<m>G</m>oto line", "Ctrl-G", "Gotoline", {}}
                        }
                      },
                      { "<m>O</m>ptions", "", {}, {
                                 { "<m>T</m>ab", "", "Tab", {}},
                                 { "<m>F</m>ormatting characters", "", "Formatting", {}},
                                 { "<m>W</m>rap long lines", "", "Wrap", {}}
                             }
                           },
                      { "Hel<m>p</m>", "", {}, {
                            { "<m>A</m>bout", "", "About", {}}
                        }
                      }
                   });

    QObject::connect(new Tui::ZCommandNotifier("New", this), &Tui::ZCommandNotifier::activated,
         [&] {
            if(file->modified) {
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
            if(file->modified) {
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
            if (file->getformatting_characters())
                file->setFormatting_characters(false);
            else
                file->setFormatting_characters(true);
            update();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Wrap", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->setWrapOption(!file->getWrapOption());
        }
    );

    win = new WindowWidget(this);

    //File
    file = new File(win);
    win->setWindowTitle(file->getFilename());

    StatusBar *s = new StatusBar(this);
    connect(file, &File::cursorPositionChanged, s, &StatusBar::cursorPosition);
    connect(file, &File::scrollPositionChanged, s, &StatusBar::scrollPosition);
    connect(file, &File::modifiedChanged, s, &StatusBar::setModified);

    ScrollBar *sc = new ScrollBar(win);
    connect(file, &File::scrollPositionChanged, sc, &ScrollBar::scrollPosition);
    connect(file, &File::textMax, sc, &ScrollBar::positonMax);

    ScrollBar *scH = new ScrollBar(win);
    connect(file, &File::scrollPositionChanged, scH, &ScrollBar::scrollPosition);
    connect(file, &File::textMax, scH, &ScrollBar::positonMax);

    WindowLayout *winLayout = new WindowLayout();
    win->setLayout(winLayout);
    winLayout->addRightBorderWidget(sc);
    winLayout->addBottomBorderWidget(scH);
    winLayout->addCentralWidget(file);

    VBoxLayout *rootLayout = new VBoxLayout();
    setLayout(rootLayout);
    rootLayout->addWidget(menu);
    rootLayout->addWidget(win);
    rootLayout->addWidget(s);

    SearchDialog* searchDlg = new SearchDialog(this, file);
    QObject::connect(new Tui::ZCommandNotifier("search", this), &Tui::ZCommandNotifier::activated,
                     searchDlg, &SearchDialog::open);
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
    win->setWindowTitle(filename);
    file->saveText();
    win->update();
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
        file->saveText();
        return nullptr;
    }
}

void Editor::quit() {
    if(file->modified) {
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

    // Big Files
    QCommandLineOption bigOption("b",QCoreApplication::translate("main", "Open bigger files then 1MB"));
    parser.addOption(bigOption);

    // Wrap Lines
    QCommandLineOption wraplines("w",QCoreApplication::translate("main", "Wrap log lines"));
    parser.addOption(wraplines);

    // Config File
    QCommandLineOption configOption({"c","config"},
                                    QCoreApplication::translate("main", "The config file"),
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
    root->file->setFormatting_characters(fb);

    bool bigfile = qsettings->value("bigfile", "false").toBool();

    bool wl = qsettings->value("wrap_lines","false").toBool();
    root->file->setWrapOption(wl || parser.isSet(wraplines));

    QString logfile = qsettings->value("logfile", "").toString();
    if (logfile.isEmpty()) {
        QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, false);
    } else {
        Tui::ZSimpleFileLogger::install(logfile);
    }
    qDebug("chr starting");

    // OPEN FILE
    int lineNumber = 0;
    if(!args.isEmpty()) {
        QStringList p = args;
        if (p.first().mid(0,1) == "+") {
            QString n = p.first().mid(1);
            lineNumber = n.toInt();
            p.removeFirst();
        }
        QFileInfo datei(p.first());
        if(datei.isReadable()) {
            if(datei.size() > 10240 && !parser.isSet(bigOption) && !bigfile) {
                //TODO: warn dialog
                out << "The file is bigger then 1MB. Pleas start with -b for big files.\n";
                return 0;
            }
        }
        root->openFile(datei.absoluteFilePath());
    } else {
        root->newFile();
    }

    //Goto Line
    if (lineNumber > 0) {
        root->file->gotoline(lineNumber);
    } else if(parser.isSet(numberOption)) {
        root->file->gotoline(parser.value(numberOption).toInt());
    }

    root->file->setFocus();

    app.exec();
    return 0;
}
