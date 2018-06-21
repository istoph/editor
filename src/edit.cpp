#include "edit.h"

Editor::Editor() {
    ensureCommandManager();

    Menubar *menu = new Menubar(this);
    menu->setItems({
                      { "<m>F</m>ile", "", {}, {
                            { "<m>N</m>ew", "Ctrl-N", "New", {}},
                            { "<m>O</m>pen...", "Ctrl-O", "Open", {}},
                            { "<m>S</m>ave", "Ctrl-s", "Save", {}},
                            { "Save <m>a</m>s...", "Ctrl-S", "SaveAs", {}},
                            {},
                            { "E<m>x</m>it", "Ctrl-Q", "Quit", {}}
                        }
                      },
                      { "<m>E</m>dit", "", "", {

                            { "Cu<m>t</m>", "Ctrl-X", "Cut", {}},
                            { "<m>C</m>opy", "Ctrl-C", "Copy", {}},
                            { "<m>P</m>aste", "Ctrl-V", "Paste", {}},
                            { "Select <m>a</m>ll", "Ctrl-A", "selectall", {}},
                            { "Cut Line", "Ctrl-K", "cutline", {}},
                            {},
                            { "<m>S</m>earch...", "Ctrl-F", "Search", {}},
                            {},
                            { "<m>G</m>oto line", "Ctrl-G", "Gotoline", {}}
                        }
                      },
                      { "<m>O</m>ptions", "", {}, {
                                 { "<m>T</m>ab", "", "Tab", {}},
                                 { "<m>F</m>ormatting characters", "", "Formatting", {}}
                             }
                           },
                      { "Hel<m>p</m>", "", {}, {
                            { "<m>A</m>bout", "", "About", {}}
                        }
                      }
                   });

    QObject::connect(new Tui::ZCommandNotifier("New", this), &Tui::ZCommandNotifier::activated,
         [&] {
            if(file->unsave) {
                //TODO: save as dialog
            }
            file->newText();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Save", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->saveText();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Quit", this), &Tui::ZCommandNotifier::activated,
         [&] {
            QuitDialog *quitDialog = new QuitDialog(this, file->getFilename());
            QObject::connect(quitDialog, &QuitDialog::exitSelected, [=]{
                file->saveText();
                QCoreApplication::instance()->quit();
            });

            QObject::connect(quitDialog, &QuitDialog::saveSelected, [=]{
                QCoreApplication::instance()->quit();
            });
            QObject::connect(quitDialog, &QuitDialog::rejected, [=]{
                delete quitDialog;
            });
        }
    );
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
    QObject::connect(new Tui::ZCommandNotifier("selectall", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->selectAll();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("cutline", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->cutline();
        }
    );
    QObject::connect(new Tui::ZCommandNotifier("Gotoline", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file_goto_line = new Dialog(this);
            file_goto_line->setGeometry({20, 2, 40, 8});
            file_goto_line->focus();

            Label *l1 = new Label(file_goto_line);
            l1->setText("Goto line: ");
            l1->setGeometry({1,2,12,1});

            InputBox *i1 = new InputBox(file_goto_line);
            i1->setGeometry({15,2,3,1});
            i1->setFocus();

            Button *b1 = new Button(file_goto_line);
            b1->setGeometry({15, 5, 7, 7});
            b1->setText(" OK");
            b1->setDefault(true);

            QObject::connect(b1, &Button::clicked, [=]{
                if(i1->text().toInt() >= 0) {
                    file->gotoline(i1->text().toInt());
                    file_goto_line->deleteLater();
                }
                //TODO: error message
            });

        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Tab", this), &Tui::ZCommandNotifier::activated,
         [&] {
            //NEW Window
            option_tab = new Dialog(this);
            option_tab->setGeometry({20, 3, 40, 9});
            option_tab->setFocus();
            option_tab->setWindowTitle("Formatting Tab");
            //option_tab->setPaletteClass({"window","cyan"});

            RadioButton *r1 = new RadioButton(option_tab);
            r1->setGeometry({1, 2, 12, 1});
            r1->setMarkup("<m>T</m>ab");
            // TODO: onSelect i1 und b1->setEnabled(false);
            r1->setChecked(!file->getTabOption());

            RadioButton *r2 = new RadioButton(option_tab);
            r2->setGeometry({12, 2, 12, 1});
            r2->setMarkup("<m>B</m>lank");
            r2->setChecked(file->getTabOption());

            Label *l1 = new Label(option_tab);
            l1->setText("Tab Stops: ");
            l1->setGeometry({1,4,12,1});

            InputBox *i1 = new InputBox(option_tab);
            i1->setText(QString::number(file->getTabsize()));
            i1->setGeometry({15,4,6,1});
            i1->setFocus();
            i1->setEnabled(false);

            Button *b1 = new Button(option_tab);
            b1->setGeometry({15, 6, 6, 7});
            b1->setText("OK");
            b1->setDefault(true);

            QObject::connect(b1, &Button::clicked, [=]{
                if(i1->text().toInt() > 0) {
                    file->setTabOption(r2->checked());
                    file->setTabsize(i1->text().toInt());
                    option_tab->deleteLater();
                }
                //TODO: error message
            });

            Button *cancel = new Button(option_tab);
            cancel->setText("Cancel");
            cancel->setGeometry({28,6,10,7});

            QObject::connect(cancel, &Button::clicked, [=]{
                option_tab->deleteLater();
            });
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

    win = new WindowWidget(this);

    //File
    file = new File(win);
    win->setWindowTitle(file->getFilename());

    StatusBar *s = new StatusBar(this);
    connect(file, &File::cursorPositionChanged, s, &StatusBar::cursorPosition);
    connect(file, &File::scrollPositionChanged, s, &StatusBar::scrollPosition);

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

    QObject::connect(new Tui::ZCommandNotifier("Open", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::openFileDialog);

    QObject::connect(new Tui::ZCommandNotifier("SaveAs", this), &Tui::ZCommandNotifier::activated,
                     this, &Editor::saveFileDialog);

}

void Editor::openFile(QString filename) {
    file->setFilename(filename);
    win->setWindowTitle(filename);
    file->openText();
}
void Editor::saveFile(QString filename)
{
    file->setFilename(filename);
    win->setWindowTitle(filename);
    file->saveText();
    win->update();
}
void Editor::openFileDialog() {
    OpenDialog * openDialog = new OpenDialog(this);
    connect(openDialog, &OpenDialog::fileSelected, this, &Editor::openFile);
}

void Editor::saveFileDialog()
{
    SaveDialog * saveDialog = new SaveDialog(this);
    connect(saveDialog, &SaveDialog::fileSelected, this, &Editor::saveFile);
}

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("chr");
    QCoreApplication::setApplicationVersion("0.1");

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

    if(!args.isEmpty()) {
        QFileInfo datei(args.first());
        if(datei.isReadable()) {
            if(datei.size() > 10240 && !parser.isSet(bigOption)) {
                //TODO: warn dialog
                out << "The file is bigger then 1MB. Pleas start with -b for big files.";
                return 0;
            }
            root->file->setFilename(args.first());
            root->file->openText();
        } else {
            root->file->setFilename(args.first());
        }
    } else {
        // TODO path
        root->file->setFilename("dokument.txt");
    }

    if(parser.isSet(numberOption)) {
        //out << parser.value(numberOption);
        root->file->gotoline(parser.value(numberOption).toInt());
    }

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

    root->file->setFocus();

    app.exec();
    return 0;
}
