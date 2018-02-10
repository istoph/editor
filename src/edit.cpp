#include "edit.h"

#include "testtui_lib.h"


File::File() {
    setFocusPolicy(Qt::StrongFocus);
    _text.append(QString());
}

File::File(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    _text.append(QString());
}

//QString File::text() const
//{
//    return _text;
//}

//void File::setText(const QString &t)
//{
//    _text = t;
//    _cursorPositionX = _text.size();
//    adjustScrollPosition();
//    update();
//}
bool File::setFilename(QString filename) {
    //TODO: check if file readable
    this->filename = filename;
    return true;
}
QString File::getFilename() {
    return this->filename;
}

bool File::saveText() {
    QFile file(this->filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        QString text;
        foreach (text, _text) {
            stream << text << endl;
        }
        file.close();
        return true;
    }
    //TODO vernünfiges error Händling
    return false;
}

bool File::openText() {
    QFile file(this->filename);
    if (file.open(QIODevice::ReadOnly)) {
        //TODO: destoy _text
        QTextStream in(&file);
        while (!in.atEnd()) {
           _text.append(in.readLine());
        }
        file.close();

        /*
        foreach (text, file.readLine(4096)) {
            _text.insert(_text.end(), text);
        }
        */
        return true;
    }
    return false;
}

void File::paintEvent(Tui::ZPaintEvent *event)
{
    Tui::ZColor bg;
    Tui::ZColor fg;

    bg = getColor("control.bg");
    fg = getColor("control.fg");

    auto *painter = event->painter();
    painter->clear(fg, bg);
    QString text;
    for (int y = 0; y < _text.size(); y++) {
        //TODO: Make it optional
        if (true) {
            text = _text[y] + "¶";
            text.replace(" ","·");
        } else {
            text = _text[y];
        }
        painter->writeWithColors(0, y, (text.mid(_scrollPositionX)).toUtf8(), fg, bg);
        if (focus()) {
            showCursor({_cursorPositionX - _scrollPositionX, _cursorPositionY});
        }
    }

}

void File::keyEvent(Tui::ZKeyEvent *event)
{
    QString text = event->text();
    if(event->key() == Qt::Key_Space && event->modifiers() == 0) {
        text = " ";
    }
    if(text.size() && event->modifiers() == 0) {
        if(_text[_cursorPositionY].size() < _cursorPositionX) {
            _text[_cursorPositionY].resize(_cursorPositionX,' ');
        }
        _text[_cursorPositionY].insert(_cursorPositionX, text);
        _cursorPositionX += text.size();
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Backspace && event->modifiers() == 0) {
        if (_cursorPositionX > 0) {
            _text[_cursorPositionY].remove(_cursorPositionX -1, 1);
            _cursorPositionX -= 1;
            adjustScrollPosition();

        } else if (_cursorPositionY > 0) {

            _text[_cursorPositionY -1] += _text[_cursorPositionY];
            _text.removeAt(_cursorPositionY);

            --_cursorPositionY;
            _cursorPositionX = _text[_cursorPositionY].size();
            //FIX POSITION
        }
        update();
    } else if(event->key() == Qt::Key_Delete && event->modifiers() == 0) {
        if(_text[_cursorPositionY].size() > _cursorPositionX) {
            _text[_cursorPositionY].remove(_cursorPositionX, 1);

        } else if(_text.count() > _cursorPositionY +1) {
            if(_text[_cursorPositionY].size() < _cursorPositionX) {
                _text[_cursorPositionY].resize(_cursorPositionX,' ');
            }
            _text[_cursorPositionY] += _text[_cursorPositionY + 1];
            _text.removeAt(_cursorPositionY +1);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Left && event->modifiers() == 0) {
        if (_cursorPositionX > 0) {
            --_cursorPositionX;
            adjustScrollPosition();
            update();
        }
    } else if(event->key() == Qt::Key_Right && event->modifiers() == 0) {
        //if (_cursorPositionX < _text[_cursorPositionY].size()) {
            ++_cursorPositionX;
            adjustScrollPosition();
            update();
        //}
    } else if(event->key() == Qt::Key_Down && event->modifiers() == 0) {
        if (_text.size() -1 > _cursorPositionY) {
            ++_cursorPositionY;
            adjustScrollPosition();
        } else {
            ++_cursorPositionY;
            _text.append(QString());
            _cursorPositionX = 0;
        }
        update();
    } else if(event->key() == Qt::Key_Up && event->modifiers() == 0) {
        if (_cursorPositionY > 0) {
            --_cursorPositionY;
            adjustScrollPosition();
            update();
        }
    } else if(event->key() == Qt::Key_Home && event->modifiers() == 0) {
        _cursorPositionX = 0;
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_End && event->modifiers() == 0) {
        _cursorPositionX = _text[_cursorPositionY].size();
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Enter && event->modifiers() == 0) {
        _text.insert(_cursorPositionY + 1,QString());
        if (_text[_cursorPositionY].size() > _cursorPositionX) {
            _text[_cursorPositionY + 1] = _text[_cursorPositionY].mid(_cursorPositionX);
            _text[_cursorPositionY].resize(_cursorPositionX);
        }
        ++_cursorPositionY;
        _cursorPositionX = 0;
        update();
    } else {
        Tui::ZWidget::keyEvent(event);
    }
}

void File::adjustScrollPosition() {
    if (_cursorPositionX - _scrollPositionX >= geometry().width()) {
         _scrollPositionX = _cursorPositionX - geometry().width() + 1;
    }
    if (_text.length() - _scrollPositionX < geometry().width()-1) {
         _scrollPositionX = std::max(0, _text.length() - geometry().width() + 1);
    }
    if (_cursorPositionX > 0) {
        if (_cursorPositionX - _scrollPositionX < 1) {
            _scrollPositionX = _cursorPositionX - 1;
        }
    } else {
        _scrollPositionX = 0;
    }
}



Editor::Editor() {
    ensureCommandManager();


    Menubar *menu = new Menubar(this);
    menu->setGeometry({0, 0, 80, 1});
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

                        }},
                      { "Hel<m>p</m>", "", {}, {
                            { "<m>A</m>bout", "", "About", {}}
                        }
                      }
                   });


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

    win = new WindowWidget(this);
    win->setGeometry({0, 1, 80, 22});

    //File
    file = new File(win);
    file->setGeometry({1,1,78,20});
    file->setFilename("dokument.txt");

    file_name = new Label(file);
    file_name->setText(" "+ file->getFilename() +" ");
    file_name->setGeometry({(78-file_name->text().size())/2, -1, file_name->text().size() +1, 1});

    Tui::ZPalette p = file_name->palette();
    p.setColors({
                    { "control.bg", {0xaa, 0xaa, 0xaa}},
                    { "control.fg", {0xff, 0xff, 0xff}},
                });
    file_name->setPalette(p);

    //StatusBar Left
    statusBarL = new Label(this);
    statusBarL->setGeometry({0, 23, 50, 1});
    statusBarL->setText(" Unsave UTF-8");
    //Tui::ZPalette p = statusBar->palette();
    p.setColors({
                    { "control.bg", {0, 0xaa, 0xaa}},
                    { "control.fg", {0xff, 0xff, 0xff}},
                });
    statusBarL->setPalette(p);


    //StatusBar Right
    statusBar = new Label(this);
    statusBar->setGeometry({50, 23, 80, 1});
    statusBar->setText("| " + QString::number(file->_cursorPositionY) +":"+ QString::number(file->_cursorPositionX));
    //Tui::ZPalette p = statusBar->palette();
    p.setColors({
                    { "control.bg", {0, 0xaa, 0xaa}},
                    { "control.fg", {0, 0, 0}},
                });
    statusBar->setPalette(p);




}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    Tui::ZTerminal terminal;

    Editor *root = new Editor();

    terminal.setMainWidget(root);
    root->file->setFocus();

    app.exec();
    return 0;
}
