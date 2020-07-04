#include "file.h"

#include <Tui/ZCommandNotifier.h>

File::File(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setCursorStyle(Tui::CursorStyle::Bar);
    setCursorColor(255, 255, 255);
    //_text.append(QString());
    newText();
    _cmdUndo = new Tui::ZCommandNotifier("Undo", this);
    QObject::connect(_cmdUndo, &Tui::ZCommandNotifier::activated, this, [this]{undo();});
    _cmdUndo->setEnabled(false);
    _cmdRedo = new Tui::ZCommandNotifier("Redo", this);
    QObject::connect(_cmdRedo, &Tui::ZCommandNotifier::activated, this, [this]{redo();});
    _cmdRedo->setEnabled(false);

    _cmdSearchNext = new Tui::ZCommandNotifier("Search Next", this);
    QObject::connect(_cmdSearchNext, &Tui::ZCommandNotifier::activated, this, [this]{searchNext();});
    _cmdSearchNext->setEnabled(false);
    _cmdSearchPrevious = new Tui::ZCommandNotifier("Search Previous", this);
    QObject::connect(_cmdSearchPrevious, &Tui::ZCommandNotifier::activated, this, [this]{searchPrevious();});
    _cmdSearchPrevious->setEnabled(false);

}

bool File::readAttributes() {
    if(_attributesfile.isEmpty()) {
        return false;
    }
    QFile fileRead(_attributesfile);
    if(!fileRead.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //qDebug() << "File open error";
        return false;
    }
    QString val = fileRead.readAll();
    fileRead.close();

    QJsonDocument jd = QJsonDocument::fromJson(val.toUtf8());
    _jo = jd.object();
    return true;
}

void File::getAttributes() {
    if(_attributesfile.isEmpty() || _cursorPositionX || _cursorPositionY) {
        return;
    }
    if(readAttributes()) {
        QJsonObject data = _jo.value(_filename).toObject();
        if(_text.size() > data.value("cursorPositionY").toInt() && _text[data.value("cursorPositionY").toInt()].size() +1 > data.value("cursorPositionX").toInt()) {
            _cursorPositionX = data.value("cursorPositionX").toInt();
            _cursorPositionY = data.value("cursorPositionY").toInt();
            _scrollPositionX = data.value("scrollPositionX").toInt();
            _scrollPositionY = data.value("scrollPositionY").toInt();

            adjustScrollPosition();
        }
    }
}

void File::writeAttributes() {
    QFileInfo filenameInfo(_filename);
    if(!filenameInfo.exists() || _attributesfile.isEmpty()) {
        return;
    }
    readAttributes();

    QJsonObject data;
    data.insert("cursorPositionX",_cursorPositionX);
    data.insert("cursorPositionY",_cursorPositionY);
    data.insert("scrollPositionX",_scrollPositionX);
    data.insert("scrollPositionY",_scrollPositionY);
    _jo.insert(filenameInfo.absoluteFilePath(), data);

    QJsonDocument jsonDoc;
    jsonDoc.setObject(_jo);

    //Save
    QSaveFile file(_attributesfile);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        //qDebug() << "File open error";
    }
    file.write(jsonDoc.toJson());
    file.commit();
}

void File::setAttributesfile(QString attributesfile) {
    //TODO is creatable
    if(attributesfile.isEmpty() || attributesfile.isNull()) {
        _attributesfile = "";
    } else {
        QFileInfo i(attributesfile);
        _attributesfile = i.absoluteFilePath();
    }
}

bool File::setFilename(QString filename, bool newfile) {
    if(newfile) {
        _filename = filename;
        this->newfile = false;
        saveUndoStep();
    } else {
        QFileInfo filenameInfo(filename);
        _filename = filenameInfo.absoluteFilePath();
    }
    return true;
}
QString File::getFilename() {
    return _filename;
}

bool File::newText() {
    if(getFilename().isEmpty()) {
        setFilename("dokument.txt");
        modifiedChanged(false);
        newfile = true;
    }
    _text.clear();
    _text.append(QString());
    _undoSteps.clear();
    _currentUndoStep = -1;
    _collapseUndoStep = false;
    _scrollPositionX = 0;
    _cursorPositionX = 0;
    _cursorPositionY = 0;
    _saveCursorPositionX = 0;
    saveUndoStep();
    _savedUndoStep = _currentUndoStep;
    update();
    return true;
}

bool File::saveText() {
    QFile file(_filename);
    if (file.open(QIODevice::WriteOnly)) {
        for (int i=0; i<_text.size(); i++) {
            file.write(surrogate_escape::encode(_text[i]));
            if(i+1 == _text.size() && _nonewline) {
                // omit newline
            } else {
                file.write("\n", 1);
            }
        }

        file.close();
        saveUndoStep();
        _savedUndoStep = _currentUndoStep;
        modifiedChanged(false);
        newfile = false;
        update();
        writeAttributes();
        return true;
    }
    //TODO vernünfiges error Händling
    return false;
}

void File::checkWritable() {
    setWritable(getWritable());
}

bool File::getWritable() {
    QFileInfo file(_filename);
    return file.isWritable();
}

void File::setHighlightBracket(bool hb) {
    _bracket = hb;
}

bool File::getHighlightBracket() {
    return _bracket;
}

bool File::openText() {
    QFile file(_filename);
    newfile = false;
    if (file.open(QIODevice::ReadOnly)) {
        _text.clear();
        QByteArray lineBuf;
        lineBuf.resize(16384);
        while (!file.atEnd()) { // each line
            int lineBytes = 0;
            _nonewline = true;
            while (!file.atEnd()) { // chunks of the line
                int res = file.readLine(lineBuf.data() + lineBytes, lineBuf.size() - 1 - lineBytes);
                if (res < 0) {
                    // Some kind of error
                    return false;
                } else if (res > 0) {
                    lineBytes += res;
                    if (lineBuf[lineBytes - 1] == '\n') {
                        --lineBytes; // remove \n
                        _nonewline = false;
                        break;
                    } else if (lineBytes == lineBuf.size() - 2) {
                        lineBuf.resize(lineBuf.size() * 2);
                    } else {
                        break;
                    }
                }
            }

            QString text = surrogate_escape::decode(lineBuf.constData(), lineBytes);

            _text.append(text);
        }

        file.close();
        _undoSteps.clear();
        _currentUndoStep = -1;
        if (_text.isEmpty()) {
            _text.append("");
            _nonewline = true;
            saveUndoStep();
            _savedUndoStep = -1;
        } else {
            saveUndoStep();
            _savedUndoStep = _currentUndoStep;
        }
        checkWritable();
        getAttributes();
        return true;
    }
    return false;
}

void File::cut() {
    copy();
    delSelect();
    adjustScrollPosition();
}

void File::cutline() {
    resetSelect();
    select(0,_cursorPositionY);
    select(_text[_cursorPositionY].size(),_cursorPositionY);
    //select(0,_cursorPositionY +1);
    cut();
    saveUndoStep();
}

void File::copy() {
    if(isSelect()) {
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
}

void File::paste() {
    if(isSelect()){
        delSelect();
    }
    for (int i=0; i<_clipboard.size(); i++) {
        _text[_cursorPositionY].insert(_cursorPositionX, _clipboard[i]);
        _cursorPositionX += _clipboard[i].size();
        if(i+1<_clipboard.size()) {
            insertLinebreak();
        }
    }
    safeCursorPosition();
    adjustScrollPosition();
    saveUndoStep();
}

bool File::isInsertable() {
    return !_clipboard.isEmpty();
}

void File::insertLinebreak() {
    if(_nonewline && _cursorPositionY == _text.size() -1 && _text[_cursorPositionY].size() == _cursorPositionX) {
        _nonewline = false;
    } else {
        _text.insert(_cursorPositionY + 1,QString());
        if (_text[_cursorPositionY].size() > _cursorPositionX) {
            _text[_cursorPositionY + 1] = _text[_cursorPositionY].mid(_cursorPositionX);
            _text[_cursorPositionY].resize(_cursorPositionX);
        }
        _cursorPositionX=0;
        _cursorPositionY++;
        safeCursorPosition();
        saveUndoStep();
    }
}

void File::gotoline(int y, int x) {
    _cursorPositionX = 0;
    if(y <= 0) {
        _cursorPositionY = 0;
    } else if (_text.size() < y) {
        _cursorPositionY = _text.size() -1;
    } else {
        _cursorPositionY = y -1;
    }
    if(x <= 0) {
        _cursorPositionX = 0;
    } else if (_text[_cursorPositionY].size() < x) {
        _cursorPositionX = _text[_cursorPositionY].size() -1;
    } else {
        _cursorPositionX = x;
    }
    safeCursorPosition();
    adjustScrollPosition();
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

bool File::setFormattingCharacters(bool fb) {
    this->_formatting_characters = fb;
    return true;
}

bool File::getformattingCharacters() {
    return _formatting_characters;
}

void File::setWrapOption(bool wrap) {
    _wrapOption = wrap;
    if (_wrapOption) {
        _scrollPositionX = 0;
    }
    adjustScrollPosition();
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

QString File::getSelectText() {
    QString selectText = "";
    if(isSelect()) {
        for(int y = 0; y < _text.size();y++) {
            for (int x = 0; x < _text[y].size(); x++) {
                if(isSelect(x,y)) {
                    selectText += _text[y].mid(x,1);
                }
            }
            if(isSelect(_text[y].size(),y)) {
                selectText += "\n";
            }
        }
    }
    return selectText;
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
    resetSelect();
    select(0,0);
    select(_text[_text.size()-1].size(),_text.size());
    adjustScrollPosition();
}

bool File::delSelect() {
    if(!isSelect()) {
        return false;
    }

    auto startSelect = Position(startSelectX, startSelectY);
    auto endSelect = Position(endSelectX, endSelectY);

    if (startSelect > endSelect) {
        std::swap(startSelect, endSelect);
    }

    if (startSelect.y == endSelect.y) {
        // selection only on one line
        _text[startSelect.y].remove(startSelect.x, endSelect.x - startSelect.x);
    } else {
        _text[startSelect.y].remove(startSelect.x, _text[startSelect.y].size() - startSelect.x);
        const auto orignalTextLines = _text.size();
        if (startSelect.y + 1 < endSelect.y) {
            _text.remove(startSelect.y + 1, endSelect.y - startSelect.y - 1);
        }
        if (endSelect.y == orignalTextLines) {
            // selected until the end of buffer, no last selection line to edit
        } else {
            _text[startSelect.y].append(_text[startSelect.y + 1].midRef(endSelect.x));
            if (startSelect.y + 1 < _text.size()) {
                _text.removeAt(startSelect.y + 1);
            } else {
                _text[startSelect.y + 1].clear();
            }
        }
    }

    _cursorPositionX = startSelect.x;
    _cursorPositionY = startSelect.y;

    safeCursorPosition();
    resetSelect();
    saveUndoStep();
    return true;
}

void File::toggleOverwrite() {
    // TODO: change courser modus _ []
    // TODO: messages to statusbar
   if(isOverwrite()) {
       this->overwrite = false;
       setCursorStyle(Tui::CursorStyle::Underline);
    } else {
       this->overwrite = true;
       setCursorStyle(Tui::CursorStyle::Block);
    }
}

bool File::isOverwrite() {
    return this->overwrite;
}

void File::undo() {
    if(_undoSteps.isEmpty()) {
        return;
    }

    if (_currentUndoStep == 0) {
        return;
    }

    --_currentUndoStep;

    _text = _undoSteps[_currentUndoStep].text;
    _cursorPositionX = _undoSteps[_currentUndoStep].cursorPositionX;
    _cursorPositionY = _undoSteps[_currentUndoStep].cursorPositionY;
    modifiedChanged(isModified());
    checkUndo();
    safeCursorPosition();
    adjustScrollPosition();
}

void File::redo() {
    if(_undoSteps.isEmpty()) {
        return;
    }

    if (_currentUndoStep + 1 >= _undoSteps.size()) {
        return;
    }

    ++_currentUndoStep;

    _text = _undoSteps[_currentUndoStep].text;
    _cursorPositionX = _undoSteps[_currentUndoStep].cursorPositionX;
    _cursorPositionY = _undoSteps[_currentUndoStep].cursorPositionY;
    modifiedChanged(isModified());
    checkUndo();
    safeCursorPosition();
    adjustScrollPosition();
}

void File::setGroupUndo(bool onoff) {
    if(onoff) {
        _groupUndo++;
    } else if(_groupUndo <= 1) {
        _groupUndo = 0;
        saveUndoStep();
    } else {
        _groupUndo--;
    }
}

int File::getGroupUndo() {
    return _groupUndo;
}

bool File::isModified() const {
    return _currentUndoStep != _savedUndoStep;
}

void File::checkUndo() {
    if(_cmdUndo != nullptr) {
        if(_currentUndoStep != _savedUndoStep) {
            _cmdUndo->setEnabled(true);
        } else {
            _cmdUndo->setEnabled(false);
        }
        if(_undoSteps.size() > _currentUndoStep +1) {
            _cmdRedo->setEnabled(true);
        } else {
            _cmdRedo->setEnabled(false);
        }
    }
}

void File::setSearchText(QString searchText) {
    if(searchText == "") {
        _cmdSearchNext->setEnabled(false);
        _cmdSearchPrevious->setEnabled(false);
    } else {
        _cmdSearchNext->setEnabled(true);
        _cmdSearchPrevious->setEnabled(true);
    }
    _searchText = searchText.replace("\\t","\t");
}

void File::setReplaceText(QString replaceText) {
   _replaceText = replaceText.replace("\\t","\t");
}

void File::setSearchWrap(bool wrap) {
    _searchWrap = wrap;
}

bool File::getSearchWrap() {
    return _searchWrap;
}

void File::followStandardInput(bool follow) {
    _follow = follow;
}

void File::setReplaceSelected() {
    if(isSelect()){
        setGroupUndo(true);
        delSelect();
        for (int i=0; i<_replaceText.size(); i++) {
            _text[_cursorPositionY].insert(_cursorPositionX, _replaceText[i]);
            _cursorPositionX++;
            //_cursorPositionX += _replaceText[i].size();
            //if(i+1<_replaceText.size()) {
            //    insertLinebreak();
            //}
        }
        safeCursorPosition();
        adjustScrollPosition();
        setGroupUndo(false);
    }
}

void File::searchNext(int line) {
    if(_searchText == "" && isSelect()) {
        setSearchText(getSelectText());
    }
    if(_searchText != "") {
        int found = -1;
        bool loop = false;
        if (line < 0) {
            line = _cursorPositionY;
            found = _cursorPositionX;
        } else {
            loop = true;
        }

        for (; line < _text.size(); line++) {
            found = _text[line].indexOf(_searchText, found + 1, searchCaseSensitivity);
            if (found != -1) {
                _cursorPositionY = line;
                _cursorPositionX = found;
                adjustScrollPosition();
                resetSelect();
                select(found, _cursorPositionY);
                _cursorPositionX += _searchText.size();
                select(_cursorPositionX, _cursorPositionY);
                safeCursorPosition();
                if(_cursorPositionY - 1 > 0) {
                    _scrollPositionY = _cursorPositionY -1;
                }
                adjustScrollPosition();
                return;
            }
        }
        if(_searchWrap && !loop) {
            searchNext(0);
        }
        resetSelect();
    }
}

void File::searchPrevious(int line) {
    if(_searchText == "" && isSelect()) {
        setSearchText(getSelectText());
    }
    if(_searchText != "") {
        //int found = _text[_text.size()-1].size();
        int found = 0;
        bool loop = false;
        if (line < 0) {
            line = _cursorPositionY;
            found = _cursorPositionX;
        } else {
            loop = true;
        }

        for (; line >= 0; line--) {
            if(_cursorPositionX == 0) {
                _cursorPositionX++;
                continue;
            }
            found = _text[line].lastIndexOf(_searchText, found -1, searchCaseSensitivity);
            if (found != -1) {
                _cursorPositionY = line;
                _cursorPositionX = found -1;
                adjustScrollPosition();

                resetSelect();
                select(_cursorPositionX + _searchText.size() +1, _cursorPositionY);
                if(_cursorPositionX < 0) {
                    _cursorPositionX = 0;
                    select(1, _cursorPositionY);
                } else {
                    select(found, _cursorPositionY);
                }
                safeCursorPosition();
                adjustScrollPosition();
                return;
            }
        }
        if(_searchWrap && !loop) {
            searchPrevious(_text.size() -1);
        }
        resetSelect();
    }
}

void File::saveUndoStep(bool collapsable) {
    if(getGroupUndo() == 0) {
        if (_currentUndoStep + 1 != _undoSteps.size()) {
            _undoSteps.resize(_currentUndoStep + 1);
        } else if (_collapseUndoStep && collapsable && _currentUndoStep != _savedUndoStep) {
            _undoSteps.removeLast();
        }
        _undoSteps.append({ _text, _cursorPositionX, _cursorPositionY});
        _currentUndoStep = _undoSteps.size() - 1;
        _collapseUndoStep = collapsable;
        modifiedChanged(true);
        checkUndo();
    }
}

ZTextOption File::getTextOption() {
    ZTextOption option;
    option.setWrapMode(_wrapOption ? ZTextOption::WrapAnywhere : ZTextOption::NoWrap);
    option.setTabStopDistance(_tabsize);
    if (getformattingCharacters()) {
        option.setFlags(ZTextOption::ShowTabsAndSpaces);
    }

    return option;
}

TextLayout File::getTextLayoutForLine(const ZTextOption &option, int line) {
    TextLayout lay(terminal()->textMetrics(), _text[line]);
    lay.setTextOption(option);
    lay.doLayout(rect().width());

    return lay;
}

bool File::highlightBracket() {
    QString openBracket = "{[(<";
    QString closeBracket = "}])>";

    if(getHighlightBracket()) {
        for(int i=0; i<openBracket.size(); i++) {
            if (_text[_cursorPositionY][_cursorPositionX] == openBracket[i]) {
                int y = 0;
                int counter = 0;
                int startX = _cursorPositionX+1;
                for (int line = _cursorPositionY; y++ < rect().height() && line < _text.size(); line++) {
                    for(; startX < _text[line].size(); startX++) {
                        if (_text[line][startX] == openBracket[i]) {
                            counter++;
                        } else if (_text[line][startX] == closeBracket[i]) {
                            if(counter>0) {
                                counter--;
                            } else {
                                _bracketY=line;
                                _bracketX=startX;
                                return true;
                            }
                        }
                    }
                    startX=0;
                }
            }

            if (_text[_cursorPositionY][_cursorPositionX] == closeBracket[i]) {
                int counter = 0;
                int startX = _cursorPositionX-1;
                for (int line = _cursorPositionY; line >= 0;) {
                    for(; startX >= 0; startX--) {
                        if (_text[line][startX] == closeBracket[i]) {
                            counter++;
                        } else if (_text[line][startX] == openBracket[i]) {
                            if(counter>0) {
                                counter--;
                            } else {
                                _bracketY=line;
                                _bracketX=startX;
                                return true;
                            }
                        }
                    }
                    if(--line >= 0) {
                        startX=_text[line].size();
                    }
                }
            }
        }
    }
    _bracketX=-1;
    _bracketY=-1;
    return false;
}

void File::paintEvent(Tui::ZPaintEvent *event) {
    Tui::ZColor bg;
    Tui::ZColor fg;
    highlightBracket();

    bg = getColor("control.bg");
    //fg = getColor("control.fg");
    fg = {0xff, 0xff, 0xff};

    auto *painter = event->painter();
    painter->clear(fg, bg);

    ZTextOption option = getTextOption();

    auto startSelect = std::make_pair(startSelectY,startSelectX);
    auto endSelect = std::make_pair(endSelectY,endSelectX);
    if(startSelect > endSelect) {
        std::swap(startSelect,endSelect);
    }
    QVector<TextLayout::FormatRange> highlights;
    TextStyle base{fg, bg};
    TextStyle formatingChar{Tui::Colors::darkGray, bg};
    TextStyle selected{Tui::Colors::darkGray,fg,Tui::ZPainter::Attribute::Bold};
    TextStyle selectedFormatingChar{Tui::Colors::darkGray, fg};

    int y = 0;
    for (int line = _scrollPositionY; y < rect().height() && line < _text.size(); line++) {
        TextLayout lay = getTextLayoutForLine(option, line);

        // highlights
        highlights.clear();
        // search matches
        if(_searchText != "") {
            int found = -1;
            while ((found = _text[line].indexOf(_searchText, found + 1, searchCaseSensitivity)) != -1) {
                highlights.append(TextLayout::FormatRange{found, _searchText.size(), {Tui::Colors::darkGray,{0xff,0xdd,0},Tui::ZPainter::Attribute::Bold}, selectedFormatingChar});
            }
        }
        if(_bracketX >= 0) {
            if (_bracketY == line) {
                highlights.append(TextLayout::FormatRange{_bracketX, 1, {Tui::Colors::cyan, bg,Tui::ZPainter::Attribute::Bold}, selectedFormatingChar});
            }
            if (line == _cursorPositionY) {
                highlights.append(TextLayout::FormatRange{_cursorPositionX, 1, {Tui::Colors::cyan, bg,Tui::ZPainter::Attribute::Bold}, selectedFormatingChar});
            }
        }

        // selection
        if (line > startSelect.first && line < endSelect.first) {
            // whole line
            highlights.append(TextLayout::FormatRange{0, _text[line].size(), selected, selectedFormatingChar});
        } else if (line > startSelect.first && line == endSelect.first) {
            // selection ends on this line
            highlights.append(TextLayout::FormatRange{0, endSelect.second, selected, selectedFormatingChar});
        } else if (line == startSelect.first && line < endSelect.first) {
            // selection starts on this line
            highlights.append(TextLayout::FormatRange{startSelect.second, _text[line].size() - startSelect.second, selected, selectedFormatingChar});
        } else if (line == startSelect.first && line == endSelect.first) {
            // selection is contained in this line
            highlights.append(TextLayout::FormatRange{startSelect.second, endSelect.second - startSelect.second, selected, selectedFormatingChar});
        }

        lay.draw(*painter, {-_scrollPositionX, y}, base, &formatingChar, highlights);
        if (getformattingCharacters()) {
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
    if (_nonewline) {
        if (getformattingCharacters() && y < rect().height() && _scrollPositionX == 0) {
            painter->writeWithAttributes(_text[_text.size() -1].size(), y-1, "♦", formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
        }
        painter->writeWithAttributes(0, y, "\\ No newline at end of file", formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
    } else {
        if (getformattingCharacters() && y < rect().height() && _scrollPositionX == 0) {
            painter->writeWithAttributes(0, y, "♦", formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
        }
    }
}

void File::deletePreviousCharacterOrWord(TextLayout::CursorMode mode) {
    if (_cursorPositionX > 0) {
        TextLayout lay(terminal()->textMetrics(), _text[_cursorPositionY]);
        lay.doLayout(rect().width());
        int leftBoundary = lay.previousCursorPosition(_cursorPositionX, mode);
        _text[_cursorPositionY].remove(leftBoundary, _cursorPositionX - leftBoundary);
        _cursorPositionX = leftBoundary;
        safeCursorPosition();
        saveUndoStep();
    } else if (_cursorPositionY > 0) {
        _cursorPositionX = _text[_cursorPositionY -1].size();
        _text[_cursorPositionY -1] += _text[_cursorPositionY];
        _text.removeAt(_cursorPositionY);
        --_cursorPositionY;
        safeCursorPosition();
        saveUndoStep();
    }
}

void File::deleteNextCharacterOrWord(TextLayout::CursorMode mode) {
    if(_cursorPositionY == _text.size() -1 && _text[_cursorPositionY].size() == _cursorPositionX) {
        _nonewline = true;
    } else if(_text[_cursorPositionY].size() > _cursorPositionX) {
        TextLayout lay(terminal()->textMetrics(), _text[_cursorPositionY]);
        lay.doLayout(rect().width());
        int rightBoundary = lay.nextCursorPosition(_cursorPositionX, mode);
        _text[_cursorPositionY].remove(_cursorPositionX, rightBoundary - _cursorPositionX);
        saveUndoStep();
    } else if(_text.count() > _cursorPositionY +1) {
        if(_text[_cursorPositionY].size() < _cursorPositionX) {
            _text[_cursorPositionY].resize(_cursorPositionX,' ');
        }
        _text[_cursorPositionY] += _text[_cursorPositionY + 1];
        _text.removeAt(_cursorPositionY +1);
        saveUndoStep();
    }
}

int File::getVisibleLines() {
    if(getWrapOption()) {
        return geometry().height() - 2;
    }
    return geometry().height() - 1;
}

void File::appendLine(const QString &line) {
    // TODO richtig machen, z.B. saved state, ggf undo
    _text.append(line);
    if(_follow) {
        _cursorPositionY = _text.size() -1;
    }
    saveUndoStep();
    adjustScrollPosition();
}

void File::keyEvent(Tui::ZKeyEvent *event) {
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
            event->key() != Qt::Key_PageUp &&
            event->key() != Qt::Key_PageDown &&
            event->key() != Qt::Key_Escape &&
            event->key() != Qt::Key_F3 &&
            event->key() != Qt::Key_Tab &&
            event->modifiers() != Qt::ControlModifier
            ) {
        //Markierte Zeichen Löschen
        delSelect();
        resetSelect();
        adjustScrollPosition();
    } else if(event->key() == Qt::Key_Backspace && (event->modifiers() == 0 || event->modifiers() == Qt::ControlModifier)) {
        deletePreviousCharacterOrWord(event->modifiers() & Qt::ControlModifier ? TextLayout::SkipWords : TextLayout::SkipCharacters);
        adjustScrollPosition();
    } else if(event->key() == Qt::Key_Delete && (event->modifiers() == 0 || event->modifiers() == Qt::ControlModifier)) {
        deleteNextCharacterOrWord(event->modifiers() & Qt::ControlModifier ? TextLayout::SkipWords : TextLayout::SkipCharacters);
        adjustScrollPosition();
    }

    if (
        event->key() != Qt::Key_Tab &&
        (

            event->modifiers() != Qt::ShiftModifier &&
            (
                event->key() != Qt::Key_Right ||
                event->key() != Qt::Key_Down ||
                event->key() != Qt::Key_Left ||
                event->key() != Qt::Key_Up ||
                event->key() != Qt::Key_Home ||
                event->key() != Qt::Key_End ||
                event->key() != Qt::Key_PageUp ||
                event->key() != Qt::Key_PageDown ||
                event->key() != Qt::Key_Tab
            )
        ) && (
            event->modifiers() != Qt::ControlModifier &&
            (
                event->text() != "x" ||
                event->text() != "c" ||
                event->text() != "v" ||
                event->text() != "a"
            )
        ) && (
             event->modifiers() != (Qt::ControlModifier | Qt::ShiftModifier) &&
             (
                 event->key() != Qt::Key_Right ||
                 event->key() != Qt::Key_Left
             )
        )) {
        resetSelect();
        adjustScrollPosition();
    }

    if (_formatting_characters && (event->text() == "·" || event->text() == "→") && event->modifiers() == 0) {
        _text[_cursorPositionY].insert(_cursorPositionX, ' ');
        ++_cursorPositionX;
        safeCursorPosition();
    } else if (_formatting_characters && event->text() == "¶" && event->modifiers() == 0) {
        true;
    } else if(text.size() && event->modifiers() == 0) {
        if(_text[_cursorPositionY].size() < _cursorPositionX) {
            _text[_cursorPositionY].resize(_cursorPositionX, ' ');
        }
        if (isOverwrite() && _text[_cursorPositionY].size() > _cursorPositionX) {
            deleteNextCharacterOrWord(TextLayout::SkipCharacters);
        }
        _text[_cursorPositionY].insert(_cursorPositionX, text);
        _cursorPositionX += text.size();
        safeCursorPosition();
        adjustScrollPosition();
        saveUndoStep(text != " ");
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
        safeCursorPosition();
        adjustScrollPosition();
        if(event->modifiers() & Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _collapseUndoStep = false;
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
        safeCursorPosition();
        adjustScrollPosition();
        if(event->modifiers() & Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Down && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_text.size() -1 > _cursorPositionY) {
            ++_cursorPositionY;
            TextLayout lay = getTextLayoutForLine(getTextOption(), _cursorPositionY);
            TextLineRef la = lay.lineAt(0);
            _cursorPositionX = la.xToCursor(_saveCursorPositionX);
        }
        adjustScrollPosition();
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Up && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_cursorPositionY > 0) {
            --_cursorPositionY;
            TextLayout lay = getTextLayoutForLine(getTextOption(), _cursorPositionY);
            TextLineRef la = lay.lineAt(0);
            _cursorPositionX = la.xToCursor(_saveCursorPositionX);
        }
        adjustScrollPosition();
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Home && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if(_cursorPositionX == 0) {
            for (int i = 0; i < _text[_cursorPositionY].size()-1; i++) {
                if(_text[_cursorPositionY][i] != ' ' && _text[_cursorPositionY][i] != '\t') {
                    _cursorPositionX = i;
                    break;
                }
            }
        } else {
            _cursorPositionX = 0;
        }
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        safeCursorPosition();
        adjustScrollPosition();
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Home && (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) )) {
        if(event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionY = _cursorPositionX = _scrollPositionY = 0;
        if(event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
            select(_cursorPositionX, _cursorPositionY);
        }
        safeCursorPosition();
        adjustScrollPosition();
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_End && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionX = _text[_cursorPositionY].size();
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        safeCursorPosition();
        adjustScrollPosition();
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_End && (event->modifiers() == Qt::ControlModifier || (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)))) {
        if(event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionY = _text.size() -1;
        _cursorPositionX = _text[_cursorPositionY].size();

        if(event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
            select(_cursorPositionX, _cursorPositionY);
        }
        safeCursorPosition();
        adjustScrollPosition();
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_PageDown && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_text.size() > _cursorPositionY + getVisibleLines()) {
            _cursorPositionY += getVisibleLines();
        } else {
            _cursorPositionY = _text.size() -1;
        }
        if(event->modifiers() == Qt::ShiftModifier) {
            _cursorPositionX = _text[_cursorPositionY].size();
            select(_cursorPositionX, _cursorPositionY);
        }
        safeCursorPosition();
        adjustScrollPosition();
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_PageDown && ( event->modifiers() == Qt::ControlModifier ||
              (event->modifiers() == Qt::ControlModifier && event->modifiers() == Qt::ControlModifier) ) ) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionX = 1;
        _cursorPositionY = 1;
        if(event->modifiers() == Qt::ShiftModifier) {
            _cursorPositionX = _text[_cursorPositionY].size();
            select(_cursorPositionX, _cursorPositionY);
        }
        safeCursorPosition();
        adjustScrollPosition();
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_PageUp && (event->modifiers() == Qt::ControlModifier ||
              (event->modifiers() == Qt::ControlModifier || event->modifiers() == Qt::ControlModifier) ) ) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionX = _text[_text.size() -1].size();
        _cursorPositionY = _text.size();
        if(event->modifiers() == Qt::ShiftModifier) {
          _cursorPositionX = 0;
          select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_PageUp && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        if(event->modifiers() == Qt::ShiftModifier) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_cursorPositionY > getVisibleLines()) {
            _cursorPositionY -= getVisibleLines();
        } else {
            _cursorPositionY = 0;
            _scrollPositionY = 0;
        }
        if(event->modifiers() == Qt::ShiftModifier) {
            _cursorPositionX = 0;
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        _collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Enter && event->modifiers() == 0) {
        insertLinebreak();
        adjustScrollPosition();
    } else if(event->key() == Qt::Key_Tab && event->modifiers() == 0) {
        if(isSelect()) {
            int t_startSelectX, t_startSelectY, t_endSelectX, t_endSelectY;
            if(startSelectY <= endSelectY) {
                t_startSelectX = startSelectX;
                t_startSelectY = startSelectY;
                t_endSelectX = endSelectX;
                t_endSelectY = endSelectY;
            } else {
                t_startSelectX = endSelectX;
                t_startSelectY = endSelectY;
                t_endSelectX = startSelectX;
                t_endSelectY = startSelectY;
            }
            for(int selectedLine = t_startSelectY; selectedLine<=t_endSelectY; selectedLine++) {
                if(getTabOption()) {
                    QString l = " ";
                    _text[selectedLine].insert(0, l.repeated(getTabsize()));
                } else {
                    _text[selectedLine].insert(0, '\t');
                }
            }
            resetSelect();
            if(getTabOption()) {
                _cursorPositionX += getTabsize();
                select(t_startSelectX + getTabsize(), t_startSelectY);
                select(t_endSelectX + getTabsize(), t_endSelectY);
            } else {
                ++_cursorPositionX;
                select(t_startSelectX + 1, t_startSelectY);
                select(t_endSelectX + 1, t_endSelectY);
            }
        } else {
            if(getTabOption()) {
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
        }
        safeCursorPosition();
        adjustScrollPosition();
        saveUndoStep();
    } else if(event->key() == Qt::Key_Tab && event->modifiers() == Qt::ShiftModifier) {
        if(isSelect()) {
            QString l = " ";
            int t_startSelectX = startSelectX;
            int t_startSelectY = startSelectY;
            int t_endSelectX = endSelectX;
            int t_endSelectY = endSelectY;

            for(int selectedLine = startSelectY; selectedLine<=endSelectY; selectedLine++) {
                if(_text[selectedLine][0] == '\t') {
                   _text[selectedLine].remove(0,1);
                   if(selectedLine == startSelectY) {
                       --_cursorPositionX;
                       t_startSelectX -= 1;
                   }
                   if (selectedLine == endSelectY) {
                       t_endSelectX = -1;
                   }
                } else if (_text[selectedLine].mid(0,getTabsize()) == l.repeated(getTabsize())) {
                    _text[selectedLine].remove(0,getTabsize());
                    if(selectedLine == startSelectY) {
                        _cursorPositionX -= getTabsize();
                        t_startSelectX -= getTabsize();
                    }
                    if (selectedLine == endSelectY) {
                        t_endSelectX -= getTabsize();
                    }
                }
            }
            resetSelect();
            select(t_startSelectX, t_startSelectY);
            select(t_endSelectX, t_endSelectY);
            safeCursorPosition();
            adjustScrollPosition();
            saveUndoStep();
        }
    } else if ((event->text() == "c" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Insert && event->modifiers() == Qt::ControlModifier) ) {
        //STRG + C // Strg+Einfg
        copy();
    } else if ((event->text() == "v" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Enter && event->modifiers() == Qt::ShiftModifier)) {
        //STRG + V // Umschalt+Einfg
        paste();
    } else if ((event->text() == "x" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Delete && event->modifiers() == Qt::ShiftModifier)) {
        //STRG + X // Umschalt+Entf
        cut();
    } else if (event->text() == "z" && event->modifiers() == Qt::ControlModifier) {
        undo();
    } else if (event->text() == "y" && event->modifiers() == Qt::ControlModifier) {
        redo();
    } else if (event->text() == "a" && event->modifiers() == Qt::ControlModifier) {
        //STRG + a
        selectAll();
        _collapseUndoStep = false;
    } else if (event->text() == "k" && event->modifiers() == Qt::ControlModifier) {
        //STRG + k //cut and copy line
        cutline();
    } else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Up) {
        // Fenster hoch Scrolen
        if (_scrollPositionY > 0) {
            --_scrollPositionY;
        }
        adjustScrollPosition();
    } else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Down) {
        // Fenster runter Scrolen
        if (_text.size() -1 > _scrollPositionY) {
            ++_scrollPositionY;
        }
        adjustScrollPosition();
    } else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier) && event->key() == Qt::Key_Up) {
        //Zeilen verschiben

        //Nich markierte Zeile verschiben
        bool _resetSelect = false;
        if(!isSelect()) {
            startSelectY = endSelectY = _cursorPositionY;
            _resetSelect = true;
        }
        if(startSelectY > 0) {
            //Zeilen Markieren
            startSelectX = 0;
            endSelectX = _text[endSelectY].size();

            //Nach oben verschieben
            _text.insert(endSelectY + 1, _text[startSelectY -1]);
            _text.remove(startSelectY -1);

            //Courser nachzeihen
            if (_cursorPositionY > 0) {
                --_cursorPositionY;
            }

            //Markirung Nachziehen
            startSelectY -= 1;
            endSelectY -= 1;
            if (_resetSelect) {
                resetSelect();
            }
            saveUndoStep();
            adjustScrollPosition();
        }
    } else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier) && event->key() == Qt::Key_Down) {
        //Zeilen verschiben

        //Nich markierte Zeile verschiben
        bool _resetSelect = false;
        if(!isSelect()) {
            startSelectY = endSelectY = _cursorPositionY;
            _resetSelect = true;
        }
        if(endSelectY + 1 < _text.size()) {
            //Zeilen Markieren
            startSelectX = 0;
            endSelectX = _text[endSelectY].size();

            //Nach unten verschieben
            _text.insert(startSelectY, _text[endSelectY + 1]);
            _text.remove(endSelectY +2);

            //Courser nachzeihen
            if (_cursorPositionY < _text.size()) {
                ++_cursorPositionY;
            }

            //Markirung Nachziehen
            startSelectY += 1;
            endSelectY += 1;
            if (_resetSelect) {
                resetSelect();
            }
            saveUndoStep();
            adjustScrollPosition();
        }
    } else if (event->key() == Qt::Key_Escape && event->modifiers() == 0) {
        _searchText = "";
    } else if (event->key() == Qt::Key_Insert && event->modifiers() == 0) {
        toggleOverwrite();
    } else {
        Tui::ZWidget::keyEvent(event);
    }
}

void File::adjustScrollPosition() {
    ZTextOption option = getTextOption();
    option.setWrapMode(ZTextOption::NoWrap);
    TextLayout lay = getTextLayoutForLine(option, _cursorPositionY);
    int cursorColumn = lay.lineAt(0).cursorToX(_cursorPositionX, TextLineRef::Leading);

    //x
    if (!_wrapOption) {
        if (cursorColumn - _scrollPositionX >= geometry().width()) {
             _scrollPositionX = cursorColumn - geometry().width() + 1;
        }
        if (cursorColumn > 0) {
            if (cursorColumn - _scrollPositionX < 1) {
                _scrollPositionX = cursorColumn - 1;
            }
        } else {
            _scrollPositionX = 0;
        }
    }

    //y
    if(_cursorPositionY >= 0) {
        if (_cursorPositionY - _scrollPositionY < 1) {
            _scrollPositionY = _cursorPositionY;
        }
    }
    if(!_wrapOption) {
        if (_cursorPositionY - _scrollPositionY >= geometry().height() -1) {
            _scrollPositionY = _cursorPositionY - geometry().height() +2;
        }

        if (_text.size() - _scrollPositionY < geometry().height() -1) {
            _scrollPositionY = std::max(0,_text.size() - geometry().height() +1);
        }
    } else {
        option = getTextOption();
        int y = 0;
        QVector<int> sizes;
        for (int line = _scrollPositionY; line <= _cursorPositionY && line < _text.size(); line++) {
            TextLayout lay = getTextLayoutForLine(option, line);
            if (_cursorPositionY == line) {
                int cursorPhysicalLineInViewport = y + lay.lineForTextPosition(_cursorPositionX).y();
                while (sizes.size() && cursorPhysicalLineInViewport >= rect().height() - 1) {
                    cursorPhysicalLineInViewport -= sizes.takeFirst();
                    _scrollPositionY += 1;
                }
                break;
            }
            sizes.append(lay.lineCount());
            y += lay.lineCount();
        }
    }


    if (_text[_cursorPositionY].size() < _cursorPositionX) {
        _cursorPositionX = _text[_cursorPositionY].size();
    }

    int _utf8PositionX = _text[_cursorPositionY].left(_cursorPositionX).toUtf8().size();
    cursorPositionChanged(cursorColumn, _utf8PositionX, _cursorPositionY);
    scrollPositionChanged(_scrollPositionX, _scrollPositionY);

    int max=0;
    for (int i=_scrollPositionY; i < _text.count() && i < _scrollPositionY + geometry().height(); i++) {
        if(max < _text[i].count())
            max = _text[i].count();
    }
    textMax(max - geometry().width(),_text.count() - geometry().height());

    update();
}

void File::safeCursorPosition() {
    TextLayout lay = getTextLayoutForLine(getTextOption(), _cursorPositionY);
    TextLineRef tlr = lay.lineForTextPosition(_cursorPositionX);
    _saveCursorPositionX = tlr.cursorToX(_cursorPositionX, TextLineRef::Leading);
}

bool operator<(const File::Position &a, const File::Position &b) {
    return std::tie(a.y, a.x) < std::tie(b.y, b.x);
}

bool operator>(const File::Position &a, const File::Position &b) {
    return std::tie(a.y, a.x) > std::tie(b.y, b.x);
}

bool operator==(const File::Position &a, const File::Position &b) {
    return std::tie(a.x, a.y) < std::tie(b.x, b.y);
}
