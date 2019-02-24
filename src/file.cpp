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
    setModified(false);
    newfile = true;
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
        setModified(false);
        newfile = false;
        return true;
    }
    //TODO vernünfiges error Händling
    return false;
}

void File::setModified(bool mod) {
    this->modified = mod;
    modifiedChanged(mod);
}

bool File::openText() {
    QFile file(this->filename);
    newfile = false;
    if (file.open(QIODevice::ReadOnly)) {
        _text.clear();
        setModified(false);
        QTextStream in(&file);
        while (!in.atEnd()) {
           _text.append(in.readLine());
        }
        if (_text.isEmpty()) {
            _text.append("");
            setModified(true);
        }
        file.close();
        return true;
    }
    return false;
}

void File::cut() {
    copy();
    delSelect();
    adjustScrollPosition();
    update();
}

void File::cutline() {
    resetSelect();
    select(0,_cursorPositionY);
    select(_text[_cursorPositionY].size(),_cursorPositionY);
    //select(0,_cursorPositionY +1);
    this->cut();
    setModified(true);
}

void File::copy() {
    _clipboard.clear();
    _clipboard.append("");
    for(int y = 0; y < _text.size();y++) {
        for (int x = 0; x < _text[y].size(); x++) {
            if(isSelect(x,y)) {
                _clipboard.back() += _text[y].mid(x,1);
            }
        }
        if(isSelect(_text[y].size(),y)) {
            _clipboard.append("");
        }
    }
}

void File::paste() {
    for (int i=0; i<_clipboard.size(); i++) {
        _text[_cursorPositionY].insert(_cursorPositionX, _clipboard[i]);
        _cursorPositionX += _clipboard[i].size();
        if(i+1<_clipboard.size()) {
            insertLinebreak();
        }
    }
    adjustScrollPosition();
    setModified(true);
    update();
}

bool File::isInsertable() {
    return !_clipboard.isEmpty();
}

void File::insertLinebreak() {
    _text.insert(_cursorPositionY + 1,QString());
    if (_text[_cursorPositionY].size() > _cursorPositionX) {
        _text[_cursorPositionY + 1] = _text[_cursorPositionY].mid(_cursorPositionX);
        _text[_cursorPositionY].resize(_cursorPositionX);
    }
    _cursorPositionX=0;
    _cursorPositionY++;
}

void File::gotoline(int y) {
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

void File::setTabOption(bool tab) {
    this->_tabOption = tab;
}

bool File::getTabOption() {
    return this->_tabOption;
}

bool File::setFormatting_characters(bool fb) {
    this->_formatting_characters = fb;
    return true;
}

bool File::getformatting_characters() {
    return _formatting_characters;
}

void File::setWrapOption(bool wrap) {
    _wrapOption = wrap;
    if (_wrapOption) {
        _scrollPositionX = 0;
    }
    update();
}

bool File::getWrapOption() {
    return _wrapOption;
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

bool File::isSelect() {
    if (startSelectX != -1) {
        return true;
    }
    return false;
}

void File::selectAll() {
    select(0,0);
    select(_text[_text.size()-1].size(),_text.size());
    adjustScrollPosition();
    update();
}

bool File::delSelect() {
    if(!isSelect()) {
        return false;
    }

    int size = 0;
    for(int y=_text.size() - 1; y >= 0 ;y--) {
        for (int x=_text[y].size(); x >= 0; x--) {
            if(isSelect(x,y)) {
                _text[y].remove(x,1);
                if(x == 0) {
                    if(y>0 && isSelect(_text[y-1].size(),y-1)) {
                        size = _text[y -1].size();
                        _text[y -1] += _text[y];
                        _text.removeAt(y);
                        y--;
                        x=size;
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

void File::toggleOverwrite() {
    // TODO: change courser modus _ []
    // TODO: messages to statusbar
   if(isOverwrite()) {
        this->overwrite = false;
    } else {
        this->overwrite = true;
    }
}

bool File::isOverwrite() {
    return this->overwrite;
}

void File::undo() {
    if(_undoSteps.isEmpty()) {
        return;
    }

    _text = _undoSteps.back().text;
    _cursorPositionX = _undoSteps.back().cursorPositionX;
    _cursorPositionY = _undoSteps.back().cursorPositionY;
    adjustScrollPosition();
    update();
    _undoSteps.removeLast(); //TODO verschibe in redo
}

void File::saveUndoStep() {
    _undoSteps.append({ _text, _cursorPositionX, _cursorPositionY});
}

void File::paintEvent(Tui::ZPaintEvent *event) {
    Tui::ZColor bg;
    Tui::ZColor fg;

    bg = getColor("control.bg");
    fg = getColor("control.fg");

    auto *painter = event->painter();
    painter->clear(fg, bg);

    ZTextOption option;
    option.setWrapMode(_wrapOption ? ZTextOption::WrapAnywhere : ZTextOption::NoWrap);
    option.setTabStopDistance(_tabsize);
    if (getformatting_characters()) {
        option.setFlags(ZTextOption::ShowTabsAndSpaces);
    }

    auto startSelect = std::make_pair(startSelectY,startSelectX);
    auto endSelect = std::make_pair(endSelectY,endSelectX);
    if(startSelect > endSelect) {
        std::swap(startSelect,endSelect);
    }
    QVector<TextLayout::FormatRange> selections;
    TextStyle selected{{0x99,0,0}, fg};
    TextStyle base{fg, bg};
    TextStyle formatingChar{Tui::Colors::darkGray, bg};
    TextStyle selectedFormatingChar{Tui::Colors::darkGray, fg};

    int y = 0;
    for (int line = _scrollPositionY; y < rect().height() && line < _text.size(); line++) {
        TextLayout lay(terminal()->textMetrics(), _text[line]);
        lay.setTextOption(option);
        lay.doLayout(rect().width());
        //selection
        selections.clear();
        if (line > startSelect.first && line < endSelect.first) {
            // whole line
            selections.append(TextLayout::FormatRange{0, _text[line].size(), selected, selectedFormatingChar});
        } else if (line > startSelect.first && line == endSelect.first) {
            // selection ends on this line
            selections.append(TextLayout::FormatRange{0, endSelect.second, selected, selectedFormatingChar});
        } else if (line == startSelect.first && line < endSelect.first) {
            // selection starts on this line
            selections.append(TextLayout::FormatRange{startSelect.second, _text[line].size() - startSelect.second, selected, selectedFormatingChar});
        } else if (line == startSelect.first && line == endSelect.first) {
            // selection is contained in this line
            selections.append(TextLayout::FormatRange{startSelect.second, endSelect.second - startSelect.second, selected, selectedFormatingChar});
        }

        lay.draw(*painter, {-_scrollPositionX, y}, base, &formatingChar, selections);
        if (getformatting_characters()) {
            TextLineRef lastLine = lay.lineAt(lay.lineCount()-1);
            if (isSelect(_text[line].size(), line)) {
                painter->writeWithAttributes(-_scrollPositionX + lastLine.width(), y + lastLine.y(), QStringLiteral("¶"),
                                             selectedFormatingChar.foregroundColor(), selectedFormatingChar.backgroundColor(), selectedFormatingChar.attributes());
            } else {
                painter->writeWithAttributes(-_scrollPositionX + lastLine.width(), y + lastLine.y(), QStringLiteral("¶"),
                                             formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
            }
        }
        if (_cursorPositionY == line) {
            if (focus()) {
                lay.showCursor(*painter, {-_scrollPositionX, y}, _cursorPositionX);
            }
        }
        y += lay.lineCount();
    }
    if (y < rect().height() && this->getformatting_characters() && _scrollPositionX == 0) {
        painter->writeWithAttributes(0, y, "♦", formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
    }
}

void File::keyEvent(Tui::ZKeyEvent *event) {
    int height = 19; //Windows size

    QString text = event->text();
    if(event->key() == Qt::Key_Space && event->modifiers() == 0) {
        text = " ";
        saveUndoStep();
    }
    if (isSelect() &&
            event->key() != Qt::Key_Right &&
            event->key() != Qt::Key_Down &&
            event->key() != Qt::Key_Left &&
            event->key() != Qt::Key_Up &&
            event->key() != Qt::Key_Home &&
            event->key() != Qt::Key_End &&
            event->key() != Qt::Key_PageUp &&
            event->key() != Qt::Key_PageDown &&
            event->key() != Qt::Key_Escape &&
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
                event->key() != Qt::Key_End ||
                event->key() != Qt::Key_PageUp ||
                event->key() != Qt::Key_PageDown
            )
        ) && (
            event->modifiers() != Qt::ControlModifier &&
            (
                event->text() != "x" ||
                event->text() != "c" ||
                event->text() != "v" ||
                event->text() != "a"
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
        setModified(true);
        //saveUndoStep();
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
        setModified(true);
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
        setModified(true);
        update();
    } else if(event->key() == Qt::Key_Left && (event->modifiers() & ~(Qt::ShiftModifier | Qt::ControlModifier)) == 0) {
        if(event->modifiers() & Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_cursorPositionX > 0) {
            TextLayout lay(terminal()->textMetrics(), _text[_cursorPositionY]);
            lay.doLayout(rect().width());
            auto mode = event->modifiers() & Qt::ControlModifier ? TextLayout::SkipWords : TextLayout::SkipCharacters;
            _cursorPositionX = lay.previousCursorPosition(_cursorPositionX, mode);
        } else if (_cursorPositionY > 0) {
            _cursorPositionY -= 1;
            _cursorPositionX = _text[_cursorPositionY].size();
        }
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Right && (event->modifiers() & ~(Qt::ShiftModifier | Qt::ControlModifier)) == 0) {
        if(event->modifiers() & Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_cursorPositionX < _text[_cursorPositionY].size()) {
            TextLayout lay(terminal()->textMetrics(), _text[_cursorPositionY]);
            lay.doLayout(rect().width());
            auto mode = event->modifiers() & Qt::ControlModifier ? TextLayout::SkipWords : TextLayout::SkipCharacters;
            _cursorPositionX = lay.nextCursorPosition(_cursorPositionX, mode);
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
    } else if(event->key() == Qt::Key_PageDown && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_text.size() > _cursorPositionY + height) {
            _cursorPositionY += height;
        } else {
            _cursorPositionY = _text.size() -1;
        }
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_PageDown && ( event->modifiers() == Qt::ControlModifier ||
              (event->modifiers() == Qt::ControlModifier && event->modifiers() == Qt::ControlModifier) ) ) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionX = 1;
        _cursorPositionY = 1;
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_PageUp && (event->modifiers() == Qt::ControlModifier ||
              (event->modifiers() == Qt::ControlModifier || event->modifiers() == Qt::ControlModifier) ) ) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionX = _text[_text.size() -1].size();
        _cursorPositionY = _text.size();
        if(event->modifiers() == Qt::ShiftModifier) {
          select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_PageUp && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_cursorPositionY > height) {
            _cursorPositionY += -height;
        } else {
            _cursorPositionY = 0;
            _scrollPositionY = 0;
        }
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        update();
    } else if(event->key() == Qt::Key_Enter && event->modifiers() == 0) {
        insertLinebreak();
        saveUndoStep();
        adjustScrollPosition();
        setModified(true);
        update();
    } else if(event->key() == Qt::Key_Tab && event->modifiers() == 0) {
        if(this->getTabOption()) {
            for (int i=0; i<getTabsize(); i++) {
                if(_cursorPositionX % getTabsize() == 0 && i != 0)
                    break;
                _text[_cursorPositionY].insert(_cursorPositionX, ' ');
                ++_cursorPositionX;
            }
        } else {
            _text[_cursorPositionY].insert(_cursorPositionX, '\t');
            ++_cursorPositionX;
        }
        adjustScrollPosition();
        setModified(true);
        update();
    } else if ((event->text() == "c" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Insert && event->modifiers() == Qt::ControlModifier) ) {
        //STRG + C // Strg+Einfg
        this->copy();
    } else if ((event->text() == "v" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Enter && event->modifiers() == Qt::ShiftModifier)) {
        //STRG + V // Umschalt+Einfg
        this->paste();
    } else if ((event->text() == "x" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Delete && event->modifiers() == Qt::ShiftModifier)) {
        //STRG + X // Umschalt+Entf
        this->cut();
    } else if (event->text() == "z" && event->modifiers() == Qt::ControlModifier) {
        //STRG + z
        this->undo();
    } else if (event->text() == "a" && event->modifiers() == Qt::ControlModifier) {
        //STRG + a
        selectAll();
    } else if (event->text() == "k" && event->modifiers() == Qt::ControlModifier) {
        //STRG + k //cut and copy line
        this->cutline();
    } else if (event->key() == Qt::Key_Insert && event->modifiers() == 0) {
        this->toggleOverwrite();
    } else {
        Tui::ZWidget::keyEvent(event);
    }
}

void File::adjustScrollPosition() {
    //x
    if (!_wrapOption) {
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

    // TODO: das wird vermutlich nicht verwendet :(
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
