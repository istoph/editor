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

void File::cut()
{
    _clipboard = "";
    for(int y = 0; y < _text.size();y++) {
        for (int x = 0; x < _text[y].size(); x++) {
            if(isSelect(x,y)) {
                _clipboard += _text[y].mid(x,1);
            }
        }
    }
    delSelect();
    adjustScrollPosition();
    update();
}

void File::copy()
{
    _clipboard = "";
    for(int y = 0; y < _text.size();y++) {
        for (int x = 0; x < _text[y].size(); x++) {
            if(isSelect(x,y)) {
                _clipboard += _text[y].mid(x,1);
            }
        }
    }
}

void File::paste()
{
    _text[_cursorPositionY].insert(_cursorPositionX, _clipboard);
    // TODO: check multiline
    _cursorPositionX += _clipboard.size();
    adjustScrollPosition();
    update();
}

bool File::isInsertable()
{
    if (_clipboard != "") {
        return true;
    }
    return false;
}

void File::gotoline(int y)
{
    _cursorPositionX = 0;
    if(y <= 0) {
        _cursorPositionY = 0;
    } else if (_text.size() < y) {
        _cursorPositionY = _text.size() -1;
    } else {
        _cursorPositionY = y -1;
    }
    adjustScrollPosition();
    update();
    this->focus(); // TODO: fix it ;)
}

bool File::setTabsize(int tab) {
    this->_tabsize = tab;
    return true;
}

int File::getTabsize() {
    return this->_tabsize;
}

void File::setTabOption(bool tab)
{
    this->_tabOption = tab;
}

bool File::getTabOption()
{
    return this->_tabOption;
}

bool File::setFormatting_characters(bool fb) {
    this->_formatting_characters = fb;
    return true;
}

bool File::getformatting_characters() {
    return _formatting_characters;
}

void File::select(int x, int y) {
    if(startSelectX == -1) {
        startSelectX = x;
        startSelectY = y;
    }
    endSelectX = x;
    endSelectY = y;
}

void File::resetSelect() {
    startSelectX = startSelectY = endSelectX = endSelectY = -1;
}

bool File::isSelect(int x, int y) {
    auto startSelect = std::make_pair(startSelectY,startSelectX);
    auto endSelect = std::make_pair(endSelectY,endSelectX);
    auto current = std::make_pair(y,x);

    if(startSelect > endSelect) {
        std::swap(startSelect,endSelect);
    }

    if(startSelect <= current && current < endSelect) {
        return true;
    }
    return false;
}

bool File::isSelect()
{
    if (startSelectX != -1) {
        return true;
    }
    return false;
}

bool File::delSelect()
{
    if(!isSelect()) {
        return false;
    }

    int size = 0;
    for(int y=_text.size() - 1; y >= 0 ;y--) {
        for (int x=_text[y].size() -1; x >= 0; x--) {
            if(isSelect(x,y)) {
                _text[y].remove(x,1);
                if(x == 0) {
                    if(y>0) {
                        size = _text[y -1].size();
                        _text[y -1] += _text[y];
                        _text.removeAt(y);
                        y--;
                        x=size;
                    } else {
                        _text.removeAt(0);
                    }
                }
                _cursorPositionY = y;
                _cursorPositionX = x;
            }
        }
    }
    resetSelect();
    return true;
}



void File::toggleOverwrite()
{
    // TODO: change courser modus _ []
   if(isOverwrite()) {
        this->overwrite = false;
    } else {
        this->overwrite = true;
    }
}

bool File::isOverwrite()
{
    return this->overwrite;
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

        for(int x=0; x<=_text[y].size(); x++) {
            if(isSelect(x,y)) {
                painter->writeWithColors(x - _scrollPositionX, y - _scrollPositionY, text.mid(x,1), {0x99,0,0}, fg);
            }
        }

/*
        if(startSelectX <= y && endSelectY >= y) {
            int start = 0;
            if(startSelectY == y) {
                start = startSelectX;
            }
            int end = text.size();
            if(endSelectY == y) {
                end = endSelectY;
            }

        }
*/
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
    if (isSelect() &&
            event->key() != Qt::Key_Right &&
            event->key() != Qt::Key_Down &&
            event->key() != Qt::Key_Left &&
            event->key() != Qt::Key_Up &&
            event->key() != Qt::Key_Home &&
            event->key() != Qt::Key_End &&
            // TODO: bild up/down
            event->modifiers() != Qt::ControlModifier
            ) {
        //Markierte Zeichen Löschen
        this->delSelect();
        resetSelect();
        adjustScrollPosition();
        update();
    }
    if ( (
            event->modifiers() != Qt::ShiftModifier &&
            (
                event->key() != Qt::Key_Right ||
                event->key() != Qt::Key_Down ||
                event->key() != Qt::Key_Left ||
                event->key() != Qt::Key_Up ||
                event->key() != Qt::Key_Home ||
                event->key() != Qt::Key_End
                // TODO: bild up/down
            )
        ) && (
            event->modifiers() != Qt::ControlModifier &&
            (
                event->text() != "x" ||
                event->text() != "c" ||
                event->text() != "v"
            )
       ) ) {

        resetSelect();
        adjustScrollPosition();
        update();
    }
    if(text.size() && event->modifiers() == 0) {
        if(_text[_cursorPositionY].size() < _cursorPositionX) {
            _text[_cursorPositionY].resize(_cursorPositionX, ' ');
        }
        if (isOverwrite()) {
            _text[_cursorPositionY].replace(_cursorPositionX,text.size(), text);
        } else {
            _text[_cursorPositionY].insert(_cursorPositionX, text);
        }
        _cursorPositionX += text.size();
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Backspace && event->modifiers() == 0) {
        if (_cursorPositionX > 0) {
            _text[_cursorPositionY].remove(_cursorPositionX -1, 1);
            _cursorPositionX -= 1;
        } else if (_cursorPositionY > 0) {
            _cursorPositionX = _text[_cursorPositionY -1].size();
            _text[_cursorPositionY -1] += _text[_cursorPositionY];
            _text.removeAt(_cursorPositionY);
            --_cursorPositionY;
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
    } else if(event->key() == Qt::Key_Left && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_cursorPositionX > 0) {
            --_cursorPositionX;
        } else if (_cursorPositionY > 0) {
            _cursorPositionY -= 1;
            _cursorPositionX = _text[_cursorPositionY].size();
        }
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Right && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_cursorPositionX < _text[_cursorPositionY].size()) {
            ++_cursorPositionX;
        } else if (_cursorPositionY + 1 < _text.size()) {
            ++_cursorPositionY;
            _cursorPositionX = 0;
        }
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Down && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_text.size() -1 > _cursorPositionY) {
            ++_cursorPositionY;
        } /* else {
            ++_cursorPositionY;
            _text.append(QString());
            _cursorPositionX = 0;
        } */
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Up && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_cursorPositionY > 0) {
            --_cursorPositionY;
         }
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Home && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionX = 0;
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_End && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionX = _text[_cursorPositionY].size();
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
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
        if(this->getTabOption()) {
            for (int i=getTabsize(); i>0; i--) {
                _text[_cursorPositionY].insert(_cursorPositionX, ' ');
                ++_cursorPositionX;
            }
        } else {
            _text[_cursorPositionY].insert(_cursorPositionX, '\t');
            ++_cursorPositionX;
        }
        adjustScrollPosition();
        update();
    } else if (event->text() == "c" && event->modifiers() == Qt::ControlModifier ||
               event->key() == Qt::Key_Insert && event->modifiers() == Qt::ControlModifier) {
        //STRG + C // Strg+Einfg
        this->copy();
    } else if (event->text() == "v" && event->modifiers() == Qt::ControlModifier ||
               event->key() == Qt::Key_Enter && event->modifiers() == Qt::ShiftModifier) {
        //STRG + V // Umschalt+Einfg
        this->paste();
    } else if (event->text() == "x" && event->modifiers() == Qt::ControlModifier ||
               event->key() == Qt::Key_Delete && event->modifiers() == Qt::ShiftModifier) {
        //STRG + X // Umschalt+Entf
        this->cut();
    } else if (event->text() == "s" && event->modifiers() == Qt::ControlModifier) {
        //STRG + s
        // TODO: wurde noch nicht gespeichert: dialog öffnen
        this->saveText();
    } else if (event->key() == Qt::Key_Insert && event->modifiers() == 0) {
        this->toggleOverwrite();
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
