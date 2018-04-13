#include "file.h"

File::File() {
    setFocusPolicy(Qt::StrongFocus);
    //_text.append(QString());
    newText();
}

File::File(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    //_text.append(QString());
    newText();
}

bool File::setFilename(QString filename) {
    //TODO: check if file readable
    this->filename = filename;
    return true;
}
QString File::getFilename() {
    return this->filename;
}

bool File::newText() {
    //TODO: with path
    this->setFilename("dokument.txt");
    //TODO: ask if save
    _text.clear();
    _text.append(QString());
    _scrollPositionX = 0;
    _cursorPositionX = 0;
    _cursorPositionY = 0;
    update();
    return true;
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
        _text.clear();
        QTextStream in(&file);
        while (!in.atEnd()) {
           _text.append(in.readLine());
        }
        file.close();
        return true;
    }
    return false;
}

bool File::setTabsize(int tab) {
    this->_tabsize = tab;
    return true;
}

int File::getTabsize() {
    return this->_tabsize;
}

bool File::setFormatting_characters(bool fb) {
    this->_formatting_characters = fb;
    return true;
}

bool File::getformatting_characters() {
    return _formatting_characters;
}

void File::paintEvent(Tui::ZPaintEvent *event) {
    Tui::ZColor bg;
    Tui::ZColor fg;

    bg = getColor("control.bg");
    fg = getColor("control.fg");

    auto *painter = event->painter();
    painter->clear(fg, bg);
    QString text;

    for (int y = _scrollPositionY; y < _text.size(); y++) {
        if (this->getformatting_characters()) {
            text = _text[y] + "¶";
            text.replace(" ","·");
            text.replace("\t","→");
        } else {
            text = _text[y];
        }
        painter->writeWithColors(0, y - _scrollPositionY, (text.mid(_scrollPositionX)).toUtf8(), fg, bg);
        if (focus()) {
            showCursor({_cursorPositionX - _scrollPositionX, _cursorPositionY - _scrollPositionY});
        }
    }
    if (this->getformatting_characters() && _scrollPositionX == 0) {
        painter->writeWithColors(0, _text.count() -_scrollPositionY, "♦", fg, bg);
    }
}

void File::keyEvent(Tui::ZKeyEvent *event) {
    int height = 19; //Windows size

    QString text = event->text();
    if(event->key() == Qt::Key_Space && event->modifiers() == 0) {
        text = " ";
    }
    if(text.size() && event->modifiers() == 0) {
        if(_text[_cursorPositionY].size() < _cursorPositionX) {
            _text[_cursorPositionY].resize(_cursorPositionX, ' ');
        }
        _text[_cursorPositionY].insert(_cursorPositionX, text);
        _cursorPositionX += text.size();
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Backspace && event->modifiers() == 0) {
        if (_cursorPositionX > 0) {
            _text[_cursorPositionY].remove(_cursorPositionX -1, 1);
            _cursorPositionX -= 1;
        } else if (_cursorPositionY > 0) {
            _text[_cursorPositionY -1] += _text[_cursorPositionY];
            _text.removeAt(_cursorPositionY);
            --_cursorPositionY;
            _cursorPositionX = _text[_cursorPositionY].size();
            //FIX POSITION
        }
        adjustScrollPosition();
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
            //_lastScrollPosition = _cursorPositionX;
            adjustScrollPosition();
            update();
        }
    } else if(event->key() == Qt::Key_Right && event->modifiers() == 0) {
        //if (_cursorPositionX < _text[_cursorPositionY].size()) {
            ++_cursorPositionX;
            //_lastScrollPosition = _cursorPositionX;
            adjustScrollPosition();
            update();
        //}
    } else if(event->key() == Qt::Key_Down && event->modifiers() == 0) {
        if (_text.size() -1 > _cursorPositionY) {
            ++_cursorPositionY;
        } /* else {
            ++_cursorPositionY;
            _text.append(QString());
            _cursorPositionX = 0;
        } */
        adjustScrollPosition();
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
    } else if(event->key() == Qt::Key_PageDown && event->modifiers() == 0) {
        if (_text.size() > _cursorPositionY + height) {
            _cursorPositionY += height;
        } else {
            _cursorPositionY = _text.size() -1;
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_PageUp && event->modifiers() == 0) {
        if (_cursorPositionY > height) {
            _cursorPositionY += -height;
        } else {
            _cursorPositionY = 0;
            _scrollPositionY = 0;
        }
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
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Tab && event->modifiers() == 0) {
        for (int i=getTabsize(); i>0; i--) {
            _text[_cursorPositionY].insert(_cursorPositionX, ' ');
            ++_cursorPositionX;
        }
        adjustScrollPosition();
        update();

        //TODO:
        //SHIFT LEFT / SHIFT RIGHT

    } else if(event->key() == Qt::Key_Left && event->modifiers() == Qt::ShiftModifier) {
        _text[_cursorPositionY].insert(_cursorPositionX, '<');
        adjustScrollPosition();
        update();
    } else {
        Tui::ZWidget::keyEvent(event);
    }
}

void File::adjustScrollPosition() {
    //x
    if (_cursorPositionX - _scrollPositionX >= geometry().width()) {
         _scrollPositionX = _cursorPositionX - geometry().width() + 1;
    }
    if (_cursorPositionX > 0) {
        if (_cursorPositionX - _scrollPositionX < 1) {
            _scrollPositionX = _cursorPositionX - 1;
        }
    } else {
        _scrollPositionX = 0;
    }

    //y
    if (_cursorPositionY - _scrollPositionY >= geometry().height() -1) {
        _scrollPositionY = _cursorPositionY - geometry().height() +1;
    }
    if (_text.count() - _scrollPositionY < geometry().height() -1) {
        _scrollPositionY = std::max(0,_text.count() - geometry().height() +1);
    }
    if(_cursorPositionY > 0) {
        if (_cursorPositionY - _scrollPositionY < 1) {
            _scrollPositionY = _cursorPositionY - 1;
        }
    }

    //TODO: einstellbar machen
    if (_text[_cursorPositionY].size() < _cursorPositionX) {
        //MERKEN
        //_lastScrollPosition = _cursorPositionX;
        _cursorPositionX = _text[_cursorPositionY].size();


    } else if (_cursorPositionX < _lastCursorPositionX) {
        if (_text[_cursorPositionY].size() < _lastCursorPositionX) {
            _cursorPositionX = _text[_cursorPositionY].size();
        } else {
            _cursorPositionX = _lastCursorPositionX;
        }
    }
/*
        if (_text[_cursorPositionY].size() < _lastScrollPosition) {
            _cursorPositionX = _text[_cursorPositionY].size();
        } else {
            _cursorPositionX = _lastScrollPosition;
        }
*/

        /*
        if(_lastScrollPosition > 0) {
            if (_text[_cursorPositionY].size() < _lastScrollPosition) {
                _cursorPositionX = _text[_cursorPositionY].size();
            } else {
                _cursorPositionX = _lastScrollPosition;
            }
        } else {
            _lastScrollPosition = _cursorPositionX;
            _cursorPositionX = _text[_cursorPositionY].size();
        }
        */


    cursorPositionChanged(_cursorPositionX, _cursorPositionY);
    scrollPositionChanged(_scrollPositionX, _scrollPositionY);

    //TODO: make it faster
    int max=0;
    for (int i=0; i < _text.count(); i++) {
        if(max < _text[i].count())
            max = _text[i].count();
    }
    textMax(max - geometry().width(),_text.count() - geometry().height());

}
