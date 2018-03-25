#include "edit.h"

Editor::Editor() {
    ensureCommandManager();

    Menubar *menu = new Menubar(this);
    menu->setItems({
                      { "<m>F</m>ile", "", {}, {
                            { "<m>N</m>ew", "Ctrl-N", "New", {}},
                            { "<m>O</m>pen...", "Ctrl-O", "Open", {}},
                            { "<m>S</m>ave", "Ctrl-S", "Save", {}},
                            { "Save <m>a</m>s...", "Ctrl-S", "SaveAs", {}},
                            {},
                            { "E<m>x</m>it", "Ctrl-Q", "Quit", {}}
                        }
                      },
                      { "<m>E</m>dit", "", "", {

                            { "Cu<m>t</m>", "Ctrl-X", "cut", {}},
                            { "<m>C</m>opy", "Ctrl-C", "copy", {}},
                            { "<m>P</m>aste", "Ctrl-V", "paste", {}}

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
            file->newText();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Open", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->openText();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Save", this), &Tui::ZCommandNotifier::activated,
         [&] {
            file->saveText();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Quit", this), &Tui::ZCommandNotifier::activated,
         [&] {
            QCoreApplication::instance()->quit();
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Tab", this), &Tui::ZCommandNotifier::activated,
         [&] {
            //NEW Window
            option_tab = new Dialog(this);
            option_tab->setGeometry({20, 2, 40, 8});
            option_tab->setFocus();
            //option_tab->setPaletteClass({"window","cyan"});

            Label *l1 = new Label(option_tab);
            l1->setText("Tab Stops: ");
            l1->setGeometry({1,2,12,1});

            InputBox *i1 = new InputBox(option_tab);
            i1->setText(QString::number(this->tab));
            i1->setGeometry({15,2,3,1});
            i1->setFocus();

            Button *b1 = new Button(option_tab);
            b1->setGeometry({15, 5, 6, 7});
            b1->setText(" OK");
            b1->setDefault(true);

            QObject::connect(b1, &Button::clicked, [=]{
                if(i1->text().toInt() > 0) {
                    this->tab = i1->text().toInt();
                    option_tab->deleteLater();
                }
                //TODO: error message
            });

            Button *cancel = new Button(option_tab);
            cancel->setText("Cancel");
            cancel->setGeometry({28,5,10,7});

            QObject::connect(cancel, &Button::clicked, [=]{
                option_tab->deleteLater();
            });
        }
    );

    QObject::connect(new Tui::ZCommandNotifier("Formatting", this), &Tui::ZCommandNotifier::activated,
         [&] {
            if (file->formatting_characters)
                file->formatting_characters = false;
            else
                file->formatting_characters = true;
            update();
        }
    );

    win = new WindowWidget(this);

    //File
    file = new File(win);
    file_name = new TextLine(win);
    file_name->setText(file->getFilename());
    file_name->setContentsMargins({1, 0, 1, 0});

    Tui::ZPalette p = file_name->palette();
    p.setColors({
                    { "control.bg", {0xaa, 0xaa, 0xaa}},
                    { "control.fg", {0xff, 0xff, 0xff}},
                });
    file_name->setPalette(p);

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
    winLayout->addTopBorderWidget(file_name);
    winLayout->addRightBorderWidget(sc);
    winLayout->addBottomBorderWidget(scH);
    winLayout->addCentralWidget(file);

    VBoxLayout *rootLayout = new VBoxLayout();
    setLayout(rootLayout);
    rootLayout->addWidget(menu);
    rootLayout->addWidget(win);
    rootLayout->addWidget(s);

}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    Tui::ZTerminal terminal;

    Editor *root = new Editor();

    terminal.setMainWidget(root);

    if (argc > 1) {
        root->file->setFilename(argv[1]);
        QFileInfo datei(argv[1]);
        if(datei.isReadable()) {
            if(!root->file->openText()) {
                //qFatal() << "Error to open file";
                return 1;
            }
        }
    } else {
        //TODO path
        root->file->setFilename("dokument.txt");
    }

    root->tab = 8;
    root->file->setFocus();

    app.exec();
    return 0;
}
