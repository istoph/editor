#include "file.h"

#include <Tui/ZCommandNotifier.h>

File::File(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setCursorStyle(Tui::CursorStyle::Bar);
    setCursorColor(255, 255, 255);
    setCursorStyle(Tui::CursorStyle::Underline);
    initText();
    _cmdCopy = new Tui::ZCommandNotifier("Copy", this, Qt::WindowShortcut);
    QObject::connect(_cmdCopy, &Tui::ZCommandNotifier::activated, this, [this]{copy();});
    _cmdCopy->setEnabled(false);
    _cmdCut = new Tui::ZCommandNotifier("Cut", this, Qt::WindowShortcut);
    QObject::connect(_cmdCut, &Tui::ZCommandNotifier::activated, this, [this]{cut();});
    _cmdCut->setEnabled(false);
    _cmdPaste = new Tui::ZCommandNotifier("Paste", this, Qt::WindowShortcut);
    QObject::connect(_cmdPaste, &Tui::ZCommandNotifier::activated, this, [this]{paste();});
    _cmdPaste->setEnabled(false);
    _cmdUndo = new Tui::ZCommandNotifier("Undo", this, Qt::WindowShortcut);
    QObject::connect(_cmdUndo, &Tui::ZCommandNotifier::activated, this, [this]{_doc.undo(this);});
    _cmdUndo->setEnabled(false);
    _cmdRedo = new Tui::ZCommandNotifier("Redo", this, Qt::WindowShortcut);
    QObject::connect(_cmdRedo, &Tui::ZCommandNotifier::activated, this, [this]{_doc.redo(this);});
    _cmdRedo->setEnabled(false);

    _cmdSearchNext = new Tui::ZCommandNotifier("Search Next", this, Qt::WindowShortcut);
    QObject::connect(_cmdSearchNext, &Tui::ZCommandNotifier::activated, this, [this]{runSearch(false);});
    _cmdSearchNext->setEnabled(false);
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, 0), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
          [this] {
            runSearch(false);
          });

    _cmdSearchPrevious = new Tui::ZCommandNotifier("Search Previous", this, Qt::WindowShortcut);
    QObject::connect(_cmdSearchPrevious, &Tui::ZCommandNotifier::activated, this, [this]{runSearch(true);});
    _cmdSearchPrevious->setEnabled(false);
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, Qt::ShiftModifier), this, Qt::ApplicationShortcut), &Tui::ZShortcut::activated,
          [this] {
            runSearch(true);
          });
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
        QJsonObject data = _jo.value(getFilename()).toObject();
        if(_doc._text.size() > data.value("cursorPositionY").toInt() && _doc._text[data.value("cursorPositionY").toInt()].size() +1 > data.value("cursorPositionX").toInt()) {
            setCursorPosition({data.value("cursorPositionX").toInt(), data.value("cursorPositionY").toInt()});
            _saveCursorPositionX = _cursorPositionX;
            _scrollPositionX = data.value("scrollPositionX").toInt();
            _scrollPositionY = data.value("scrollPositionY").toInt();
            adjustScrollPosition();
        }
    }
}

bool File::writeAttributes() {
    QFileInfo filenameInfo(getFilename());
    if(!filenameInfo.exists() || _attributesfile.isEmpty()) {
        return false;
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
    QDir d = QFileInfo(_attributesfile).absoluteDir();
    QString absolute=d.absolutePath();
    if(!QDir(absolute).exists()) {
        if(QDir().mkdir(absolute)) {
            qWarning("%s%s", "can not create directory: ", absolute.toUtf8().data());
            return false;
        }
    }

    QSaveFile file(_attributesfile);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("%s%s", "can not save file: ", _attributesfile.toUtf8().data());
        return false;
    }
    file.write(jsonDoc.toJson());
    file.commit();
    return true;
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

QString File::getAttributesfile() {
    return _attributesfile;
}

bool File::getMsDosMode() {
    return _msdos;
}

void File::setMsDosMode(bool msdos) {
    _msdos = msdos;
    msdosMode(_msdos);
}

int File::tabToSpace() {
    int count = 0;

    ZTextOption option = getTextOption(false);
    option.setWrapMode(ZTextOption::NoWrap);

    TextLayout lay = getTextLayoutForLine(option, _cursorPositionY);
    int cursorPosition = lay.lineAt(0).cursorToX(_cursorPositionX, TextLineRef::Leading);

    for (int line = 0, found = -1; line < _doc._text.size(); line++) {
        found = _doc._text[line].lastIndexOf("\t");
        if(found != -1) {
            TextLayout lay = getTextLayoutForLine(option, line);
            while (found != -1) {
                int columnStart = lay.lineAt(0).cursorToX(found, TextLineRef::Leading);
                int columnEnd = lay.lineAt(0).cursorToX(found, TextLineRef::Trailing);
                _doc._text[line].remove(found, 1);
                _doc._text[line].insert(found, QString(" ").repeated(columnEnd-columnStart));
                count++;
                found = _doc._text[line].lastIndexOf("\t", found);
            }
        }
    }

    lay = getTextLayoutForLine(option, _cursorPositionY);
    int newCursorPosition = lay.lineAt(0).xToCursor(cursorPosition);
    if(newCursorPosition -1 >= 0) {
        _cursorPositionX = newCursorPosition;
    }

    if (count > 0) {
        _doc.saveUndoStep(this);
    }
    return count;
}

QPoint File::getCursorPosition() {
    return {_cursorPositionX, _cursorPositionY};
}

void File::setCursorPosition(QPoint position) {
    _cursorPositionY = std::max(std::min(position.y(), _doc._text.size() -1), 0);
    _cursorPositionX = std::max(std::min(position.x(), _doc._text[_cursorPositionY].size()), 0);

    //We are not allowed to jump between characters. Therefore, we go once to the left and again to the right.
    if (_cursorPositionX > 0) {
        TextLayout lay(terminal()->textMetrics(), _doc._text[_cursorPositionY]);
        lay.doLayout(65000);
        auto mode = TextLayout::SkipCharacters;
        _cursorPositionX = lay.previousCursorPosition(_cursorPositionX, mode);
        _cursorPositionX = lay.nextCursorPosition(_cursorPositionX, mode);
    }

    adjustScrollPosition();
}

void File::setSaveAs(bool state) {
    _saveAs = state;
}
bool File::isSaveAs() {
    return _saveAs;
}


bool File::setFilename(QString filename) {
    QFileInfo filenameInfo(filename);
    _doc._filename = filenameInfo.absoluteFilePath();
    return true;
}

QString File::getFilename() {
    return _doc._filename;
}

bool File::newText(QString filename = "") {
    initText();
    if (filename == "") {
        _doc._filename = "NEWFILE";
        setSaveAs(true);
    } else {
        setFilename(filename);
        setSaveAs(false);
    }
    modifiedChanged(false);
    return true;
}

bool File::stdinText() {
    _doc._filename = "STDIN";
    initText();
    _doc.saveUndoStep(this, false);
    modifiedChanged(true);
    setSaveAs(true);
    return true;
}

bool File::initText() {
    _doc._text.clear();
    _doc._text.append(QString());
    _scrollPositionX = 0;
    _scrollPositionY = 0;
    _cursorPositionX = 0;
    _cursorPositionY = 0;
    _saveCursorPositionX = 0;
    _doc.initalUndoStep(this);
    cursorPositionChanged(0, 0, 0);
    scrollPositionChanged(0, 0);
    setMsDosMode(false);
    update();
    return true;
}

bool File::saveText() {
    // QSaveFile does not take over the user and group. Therefore this should only be used if
    // user and group are the same and the editor also runs under this user.
    QFile file(getFilename());
    //QSaveFile file(getFilename());
    if (file.open(QIODevice::WriteOnly)) {
        for (int i=0; i<_doc._text.size(); i++) {
            file.write(surrogate_escape::encode(_doc._text[i]));
            if(i+1 == _doc._text.size() && _doc._nonewline) {
                // omit newline
            } else {
                if(getMsDosMode()) {
                    file.write("\r\n", 2);
                } else {
                    file.write("\n", 1);
                }
            }
        }

        //file.commit();
        file.close();
        _doc.initalUndoStep(this);
        modifiedChanged(false);
        setSaveAs(false);
        checkWritable();
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
    QFileInfo file(getFilename());
    if(file.isWritable()) {
        return true;
    }
    QFileInfo path(file.path());
    if(!file.exists() && path.isWritable()) {
        return true;
    }
    return false;
}

void File::setHighlightBracket(bool hb) {
    _bracket = hb;
}

bool File::getHighlightBracket() {
    return _bracket;
}

bool File::openText(QString filename) {
    setFilename(filename);
    QFile file(getFilename());
    if (file.open(QIODevice::ReadOnly)) {
        initText();
        _doc._text.clear();
        QByteArray lineBuf;
        lineBuf.resize(16384);
        while (!file.atEnd()) { // each line
            int lineBytes = 0;
            _doc._nonewline = true;
            while (!file.atEnd()) { // chunks of the line
                int res = file.readLine(lineBuf.data() + lineBytes, lineBuf.size() - 1 - lineBytes);
                if (res < 0) {
                    // Some kind of error
                    return false;
                } else if (res > 0) {
                    lineBytes += res;
                    if (lineBuf[lineBytes - 1] == '\n') {
                        --lineBytes; // remove \n
                        _doc._nonewline = false;
                        break;
                    } else if (lineBytes == lineBuf.size() - 2) {
                        lineBuf.resize(lineBuf.size() * 2);
                    } else {
                        break;
                    }
                }
            }

            QString text = surrogate_escape::decode(lineBuf.constData(), lineBytes);
            _doc._text.append(text);
        }
        file.close();

        if (getWritable()) {
            setSaveAs(false);
        } else {
            setSaveAs(true);
        }

        if (_doc._text.isEmpty()) {
            _doc._text.append("");
            _doc._nonewline = true;
        }

        checkWritable();
        getAttributes();
        _doc.initalUndoStep(this);
        modifiedChanged(false);

        for (int l = 0; l < _doc._text.size(); l++) {
            if(_doc._text[l].size() >= 1 && _doc._text[l].at(_doc._text[l].size() -1) == QChar('\r')) {
                _msdos = true;
            } else {
                _msdos = false;
                break;
            }
        }
        if(_msdos) {
            msdosMode(true);
            for (int i = 0; i < _doc._text.size(); i++) {
                _doc._text[i].remove(_doc._text[i].size() -1, 1);
            }
        }
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
    select(_doc._text[_cursorPositionY].size(),_cursorPositionY);
    //select(0,_cursorPositionY +1);
    cut();
    _doc.saveUndoStep(this);
}

void File::copy() {
    if(isSelect()) {
        Clipboard *clipboard = findFacet<Clipboard>();
        QVector<QString> _clipboard;
        _clipboard.append("");
        for(int y = 0; y < _doc._text.size();y++) {
            for (int x = 0; x < _doc._text[y].size(); x++) {
                if(isSelect(x,y)) {
                    _clipboard.back() += _doc._text[y].mid(x,1);
                }
            }
            if(isSelect(_doc._text[y].size(),y) || (_blockSelect && y >= std::min(_startSelectY, _endSelectY) && y < std::max(_startSelectY, _endSelectY)) ) {
                _clipboard.append("");
            }
        }
        clipboard->setClipboard(_clipboard);
        _cmdPaste->setEnabled(true);
    }
}

void File::paste() {
    Clipboard *clipboard = findFacet<Clipboard>();
    if(!clipboard->getClipboard().empty()) {
        insertAtCursorPosition(clipboard->getClipboard());

        safeCursorPosition();
        adjustScrollPosition();
        _doc.saveUndoStep(this);
    }
}

bool File::isInsertable() {
    Clipboard *clipboard = findFacet<Clipboard>();
    QVector<QString> _clipboard = clipboard->getClipboard();
    return !_clipboard.isEmpty();
}

void File::insertLinebreak() {
    setCursorPosition(insertLinebreakAtPosition({_cursorPositionX, _cursorPositionY}));
}

QPoint File::insertLinebreakAtPosition(QPoint cursor) {
    if(_doc._nonewline && cursor.y() == _doc._text.size() -1 && _doc._text[cursor.y()].size() == cursor.x()) {
        _doc._nonewline = false;
    } else {
        _doc._text.insert(cursor.y() + 1,QString());
        if (_doc._text[cursor.y()].size() > cursor.x()) {
            _doc._text[cursor.y() + 1] = _doc._text[cursor.y()].mid(cursor.x());
            _doc._text[cursor.y()].resize(cursor.x());
        }
        cursor.setX(0);
        cursor.setY(cursor.y() +1);
    }
    return cursor;
}

void File::gotoline(QString pos) {
    int lineNumber = -1, lineChar = 0;
    if (pos.mid(0,1) == "+") {
        pos = pos.mid(1);
    }
    QStringList list1 = pos.split(',');
    if (list1.count() > 0) {
        lineNumber = list1[0].toInt() -1;
    }
    if (list1.count() > 1) {
        lineChar = list1[1].toInt() -1;
    }
    setCursorPosition({lineChar, lineNumber});
    safeCursorPosition();
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

bool File::formattingCharacters() {
    return _formattingCharacters;
}
void File::setFormattingCharacters(bool formattingCharacters) {
    _formattingCharacters = formattingCharacters;
}

bool File::colorTabs() {
    return _colorTabs;
}
void File::setColorTabs(bool colorTabs) {
    _colorTabs = colorTabs;
}

bool File::colorSpaceEnd() {
    return _colorSpaceEnd;
}
void File::setColorSpaceEnd(bool colorSpaceEnd) {
    _colorSpaceEnd = colorSpaceEnd;
}

void File::setEatSpaceBeforeTabs(bool eat) {
    _eatSpaceBeforeTabs = eat;
}

bool File::eatSpaceBeforeTabs() {
    return _eatSpaceBeforeTabs;
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
    if(_startSelectX == -1) {
        _startSelectX = x;
        _startSelectY = y;
    }
    _endSelectX = x;
    _endSelectY = y;

    _cmdCopy->setEnabled(true);
    _cmdCut->setEnabled(true);
}

void File::blockSelect(int x, int y) {
    _blockSelect = true;
    select(x, y);
}
bool File::blockSelectEdit(int x) {
    bool del = false;
    int startY = std::min(_startSelectY, _endSelectY);
    int stopY = std::max(_endSelectY, _startSelectY);

    if(std::max(_startSelectX, _endSelectX) - std::min(_startSelectX, _endSelectX) > 0) {
        if(_cursorPositionX >= std::max(_startSelectX, _endSelectX)) {
            x = std::max(0, x - (std::max(_startSelectX, _endSelectX) - std::min(_startSelectX, _endSelectX)));
        }
        delSelect();
        del = true;
    }

    _blockSelect = true;
    _startSelectX = x;
    _startSelectY = startY;
    _endSelectX = x;
    _endSelectY = stopY;
    setCursorPosition({x,stopY});
    return del;
}

QPair<int,int> File::getSelectLinesSort() {
    auto lines = getSelectLines();
    return {std::min(lines.first, lines.second), std::max(lines.first, lines.second)};
}
QPair<int,int> File::getSelectLines() {
/*
 * However other editors take that the chois
 * when the mouse cursor is at the beginning of a line
 * this line is not add to the selection.
*/
    int startY = _startSelectY;
    int endeY = _endSelectY;

    if(_startSelectY > _endSelectY) {
        if (select_cursor_position_x0 && _startSelectX == 0) {
            startY--;
        }
    } else {
        if (select_cursor_position_x0 && _endSelectX == 0) {
            endeY--;
        }
    }

    return {std::max(0, startY), std::max(0, endeY)};
}
void File::selectLines(int startY, int endY) {
    resetSelect();
    if(startY > endY) {
        select(_doc._text[startY].size(), startY);
        select(0, endY);
        setCursorPosition({0,endY});
    } else {
        select(0, startY);
        select(_doc._text[endY].size(), endY);
        setCursorPosition({_doc._text[endY].size(), endY});
    }
}

void File::resetSelect() {
    _startSelectX = _startSelectY = _endSelectX = _endSelectY = -1;
    _blockSelect = false;
    setSelectMode(false);

    _cmdCopy->setEnabled(false);
    _cmdCut->setEnabled(false);
}

QString File::getSelectText() {
    QString selectText = "";
    if(isSelect()) {
        for(int y = 0; y < _doc._text.size();y++) {
            for (int x = 0; x < _doc._text[y].size(); x++) {
                if(isSelect(x,y)) {
                    selectText += _doc._text[y].mid(x,1);
                }
            }
            if(isSelect(_doc._text[y].size(),y)) {
                selectText += "\n";
            }
        }
    }
    return selectText;
}

bool File::isSelect(int x, int y) {
    if(_blockSelect) {
        if(y >= std::min(_startSelectY, _endSelectY)  && y <= std::max(_startSelectY, _endSelectY) &&
                x >= std::min(_startSelectX, _endSelectX) && x < std::max(_startSelectX, _endSelectX)) {
            return true;
        }
    } else {
        auto startSelect = std::make_pair(_startSelectY,_startSelectX);
        auto endSelect = std::make_pair(_endSelectY,_endSelectX);

        if(startSelect > endSelect) {
            std::swap(startSelect,endSelect);
        }
        auto current = std::make_pair(y,x);
        if(startSelect <= current && current < endSelect) {
            return true;
        }
    }
    return false;
}

bool File::isSelect() {
    if (_startSelectX != -1) {
        return true;
    }
    return false;
}

void File::selectAll() {
    resetSelect();
    select(0,0);
    select(_doc._text[_doc._text.size()-1].size(),_doc._text.size());
    adjustScrollPosition();
}

bool File::delSelect() {
    if(!isSelect()) {
        return false;
    }

    auto startSelect = Position(_startSelectX, _startSelectY);
    auto endSelect = Position(_endSelectX, _endSelectY);
    if (startSelect > endSelect) {
        std::swap(startSelect, endSelect);
    }

    if(_blockSelect) {
        for(int line: getBlockSelectedLines()) {
            _doc._text[line].remove(std::min(startSelect.x, endSelect.x),
                               std::max(endSelect.x, startSelect.x) - std::min(startSelect.x, endSelect.x));
        }
        const int pos = std::min(startSelect.x, endSelect.x);
        setCursorPosition({pos, startSelect.y});
        _endSelectX = pos;
        _startSelectX = pos;
    } else {
        if (startSelect.y == endSelect.y) {
            // selection only on one line
            _doc._text[startSelect.y].remove(startSelect.x, endSelect.x - startSelect.x);
        } else {
            _doc._text[startSelect.y].remove(startSelect.x, _doc._text[startSelect.y].size() - startSelect.x);
            const auto orignalTextLines = _doc._text.size();
            if (startSelect.y + 1 < endSelect.y) {
                _doc._text.remove(startSelect.y + 1, endSelect.y - startSelect.y - 1);
            }
            if (endSelect.y == orignalTextLines) {
                // selected until the end of buffer, no last selection line to edit
            } else {
                _doc._text[startSelect.y].append(_doc._text[startSelect.y + 1].midRef(endSelect.x));
                if (startSelect.y + 1 < _doc._text.size()) {
                    _doc._text.removeAt(startSelect.y + 1);
                } else {
                    _doc._text[startSelect.y + 1].clear();
                }
            }
        }
        setCursorPosition({startSelect.x, startSelect.y});
        safeCursorPosition();
        resetSelect();
    }
    _doc.saveUndoStep(this);
    return true;
}

void File::toggleOverwrite() {
   if(isOverwrite()) {
       _overwrite = false;
       setCursorStyle(Tui::CursorStyle::Underline);
    } else {
       _overwrite = true;
       setCursorStyle(Tui::CursorStyle::Block);
    }
    emitOverwrite(_overwrite);
}

bool File::isOverwrite() {
    return _overwrite;
}

void File::checkUndo() {
    if(_cmdUndo != nullptr) {
        if(_doc._currentUndoStep != _doc._savedUndoStep) {
            _cmdUndo->setEnabled(true);
        } else {
            _cmdUndo->setEnabled(false);
        }
        if(_doc._undoSteps.size() > _doc._currentUndoStep +1) {
            _cmdRedo->setEnabled(true);
        } else {
            _cmdRedo->setEnabled(false);
        }
    }
}

void File::setSearchText(QString searchText) {
    int gen = ++(*searchGeneration);
    _searchText = searchText.replace("\\t","\t");
    emitSearchText(_searchText);

    if(searchText == "") {
        _cmdSearchNext->setEnabled(false);
        _cmdSearchPrevious->setEnabled(false);
        return;
    } else {
        _cmdSearchNext->setEnabled(true);
        _cmdSearchPrevious->setEnabled(true);
    }

    QtConcurrent::run([this](QVector<QString> text, QString searchText, Qt::CaseSensitivity caseSensitivity, int gen, std::shared_ptr<std::atomic<int>> searchGen) {
        SearchCount sc;
        QObject::connect(&sc, &SearchCount::searchCount, this, &File::emitSearchCount);
        sc.run(text, searchText, caseSensitivity, gen, searchGen);
    },_doc._text, _searchText, searchCaseSensitivity, gen, searchGeneration);
}

void File::setReplaceText(QString replaceText) {
   _replaceText = replaceText.replace("\\t","\t");
}

void File::setRegex(bool reg) {
    _searchReg = reg;
}
void File::setSearchWrap(bool wrap) {
    _searchWrap = wrap;
}

bool File::getSearchWrap() {
    return _searchWrap;
}

void File::setSearchDirection(bool searchDirection) {
    _searchDirection = searchDirection;
}

void File::setLineNumber(bool linenumber) {
    _linenumber = linenumber;
}

bool File::getLineNumber() {
    return _linenumber;
}

void File::toggleLineNumber() {
    _linenumber = !_linenumber;
}

void File::followStandardInput(bool follow) {
    _follow = follow;
}

void File::setReplaceSelected() {
    if(isSelect()){
        _doc.setGroupUndo(this, true);
        delSelect();
        bool esc=false;
        for (int i=0; i<_replaceText.size(); i++) {
            if(esc) {
                switch (_replaceText[i].unicode()) {
                    case 'n':
                        insertLinebreak();
                        break;
                    case 't':
                        break;
                }
                esc = false;
            } else {
                if(_replaceText[i] == '\\') {
                    esc = true;
                } else {
                    _doc._text[_cursorPositionY].insert(_cursorPositionX, _replaceText[i]);
                    _cursorPositionX++;
                }
            }
        }
        safeCursorPosition();
        adjustScrollPosition();
        _doc.setGroupUndo(this, false);
    }
}

struct SearchLine {
    int line;
    int found;
    int length;
};

//This struct is for fucking QtConcurrent::run which only has five parameters
struct SearchParameter {
    QString searchText;
    bool searchWrap;
    Qt::CaseSensitivity caseSensitivity;
    int startAtLine;
    int startPosition;
    bool reg;
};

static SearchLine conSearchNext(QVector<QString> text, SearchParameter search, int gen, std::shared_ptr<std::atomic<int>> nextGeneration) {
    int start = search.startAtLine;
    int line = search.startAtLine;
    int found = search.startPosition -1;
    bool reg = search.reg;
    int end = text.size();
    QRegularExpression rx(search.searchText);
    int length = 0;

    while (true) {
        for(;line < end; line++) {
            if(reg) {
                QRegularExpressionMatchIterator remi = rx.globalMatch(text[line]);
                while (remi.hasNext()) {
                    QRegularExpressionMatch match = remi.next();
                    if(nextGeneration != nullptr && gen != *nextGeneration) return {-1, -1, -1};
                    if(match.capturedLength() <= 0) continue;
                    if(match.capturedStart() < found +1) continue;
                    found = match.capturedStart();
                    length = match.capturedLength();
                    return {line, found, length};
                }
                found = -1;
            } else {
                found = text[line].indexOf(search.searchText, found + 1, search.caseSensitivity);
                length = search.searchText.size();
            }
            if(nextGeneration != nullptr && gen != *nextGeneration) return {-1, -1, -1};
            if(found != -1) return {line, found, length};
        }
        if(!search.searchWrap || start == 0) {
            return {-1, -1, -1};
        } else {
            end = std::min(start+1, text.size());
            start = line = 0;
        }
    }
    return {-1, -1, -1};
}

SearchLine File::searchNext(QVector<QString> text, SearchParameter search) {
    return conSearchNext(text, search, 0, nullptr);
}

static SearchLine conSearchPrevious(QVector<QString> text, SearchParameter search, int gen, std::shared_ptr<std::atomic<int>> nextGeneration) {
    int start = search.startAtLine;
    int line = search.startAtLine;
    bool reg = search.reg;
    int found = search.startPosition -1;
    if(found <= 0 && start > 0) {
        --line;
        found = text[line].size();
    }
    int end = 0;
    QRegularExpression rx(search.searchText);
    int length = 0;
    while (true) {
        for (; line >= end;) {
            if(reg) {
                SearchLine t = {-1,-1,-1};
                QRegularExpressionMatchIterator remi = rx.globalMatch(text[line]);
                while (remi.hasNext()) {
                    QRegularExpressionMatch match = remi.next();
                    if(gen != *nextGeneration) return {-1, -1, -1};
                    if(match.capturedLength() <= 0) continue;
                    if(match.capturedStart() <= found - match.capturedLength()) {
                        t = {line, match.capturedStart(), match.capturedLength()};
                        continue;
                    }
                    break;
                }
                if(t.length > 0)
                    return t;
                found = -1;
            } else {
                found = text[line].lastIndexOf(search.searchText, found -1, search.caseSensitivity);
                length = search.searchText.size();
            }
            if(gen != *nextGeneration) return {-1, -1, -1};
            if(found != -1) return {line, found, length};
            if(--line >= 0) found = text[line].size();
        }
        if(!search.searchWrap || start == text.size()) {
            return {-1, -1, -1};
        } else {
            end = start;
            start = line = text.size() -1;
            found = text[text.size() -1].size();
        }
    }
    return {-1, -1, -1};
}

void File::runSearch(bool direction) {
    if(_searchText != "") {
        const int gen = ++(*searchNextGeneration);
        const bool efectivDirection = direction ^ _searchDirection;

        auto watcher = new QFutureWatcher<SearchLine>();
        QObject::connect(watcher, &QFutureWatcher<SearchLine>::finished, this, [this, watcher, gen, efectivDirection]{
            auto n = watcher->future().result();
            if(gen == *searchNextGeneration && n.line > -1) {
               searchSelect(n.line, n.found, n.length, efectivDirection);
            }
            watcher->deleteLater();
        });

        SearchParameter search = { _searchText, _searchWrap, searchCaseSensitivity, _cursorPositionY, _cursorPositionX, _searchReg};
        QFuture<SearchLine> future;
        if(efectivDirection) {
            future = QtConcurrent::run(conSearchNext, _doc._text, search, gen, searchNextGeneration);
        } else {
            future = QtConcurrent::run(conSearchPrevious, _doc._text, search, gen, searchNextGeneration);
        }
        watcher->setFuture(future);
    }
}

void File::searchSelect(int line, int found, int length, bool direction) {
    if (found != -1) {
        _cursorPositionY = line;
        _cursorPositionX = found;
        adjustScrollPosition();
        resetSelect();
        select(_cursorPositionX, _cursorPositionY);
        select(_cursorPositionX + length, _cursorPositionY);
        if(direction) {
            setCursorPosition({_cursorPositionX + length -1, _cursorPositionY});
        }
        if(_cursorPositionY - 1 > 0) {
            _scrollPositionY = _cursorPositionY -1;
        }
        safeCursorPosition();
        adjustScrollPosition();
    }
}

int File::replaceAll(QString searchText, QString replaceText) {
    int counter = 0;
    setSearchText(searchText);
    setReplaceText(replaceText);
    _doc.setGroupUndo(this, true);

    _cursorPositionY = 0;
    _cursorPositionX = 0;
    while(true) {
        SearchParameter search = { _searchText, false, searchCaseSensitivity, _cursorPositionY, _cursorPositionX, _searchReg};
        SearchLine sl = searchNext(_doc._text, search);
        if(sl.length == -1) {
            break;
        }
        searchSelect(sl.line,sl.found, sl.length, true);
        setReplaceSelected();
        counter++;
    }
    _doc.setGroupUndo(this, false);

    return counter;
}

Range File::getBlockSelectedLines() {
    return Range {std::min(_startSelectY, _endSelectY), std::max(_startSelectY, _endSelectY) + 1};
}

ZTextOption File::getTextOption(bool lineWithCursor) {
    ZTextOption option;
    option.setWrapMode(_wrapOption ? ZTextOption::WrapAnywhere : ZTextOption::NoWrap);
    option.setTabStopDistance(_tabsize);

    ZTextOption::Flags flags;
    if (formattingCharacters()) {
        flags |= ZTextOption::ShowTabsAndSpaces;
    }
    if(colorTabs()) {
        flags |= ZTextOption::ShowTabsAndSpacesWithColors;
        if (getTabOption()) {
            option.setTabColor([] (int pos, int size, int hidden, const Tui::ZTextStyle &base, const Tui::ZTextStyle &formating, const FormatRange* range) -> Tui::ZTextStyle {
                (void)formating;
                if (range) {
                    if (pos == hidden) {
                        return { range->format.foregroundColor(), {0xff, 0x80, 0xff} };
                    }
                    return { range->format.foregroundColor(), {0xff, 0xb0, 0xff} };
                }
                if (pos == hidden) {
                    return { base.foregroundColor(), {base.backgroundColor().red() + 0x60,
                                    base.backgroundColor().green(),
                                    base.backgroundColor().blue()} };

                }
                return { base.foregroundColor(), {base.backgroundColor().red() + 0x40,
                                base.backgroundColor().green(),
                                base.backgroundColor().blue()} };
            });
        } else {
            option.setTabColor([] (int pos, int size, int hidden, const Tui::ZTextStyle &base, const Tui::ZTextStyle &formating, const FormatRange* range) -> Tui::ZTextStyle {
                (void)formating;
                if (range) {
                    if (pos == hidden) {
                        return { range->format.foregroundColor(), {0x80, 0xff, 0xff} };
                    }
                    return { range->format.foregroundColor(), {0xb0, 0xff, 0xff} };
                }
                if (pos == hidden) {
                    return { base.foregroundColor(), {base.backgroundColor().red(),
                                    base.backgroundColor().green() + 0x60,
                                    base.backgroundColor().blue()} };

                }
                return { base.foregroundColor(), {base.backgroundColor().red(),
                                base.backgroundColor().green() + 0x40,
                                base.backgroundColor().blue()} };
            });
        }
    }
    if (colorSpaceEnd()) {
        flags |= ZTextOption::ShowTabsAndSpacesWithColors;
        if (!lineWithCursor) {
            option.setTrailingWhitespaceColor([] (const Tui::ZTextStyle &base, const Tui::ZTextStyle &formating, const FormatRange* range) -> Tui::ZTextStyle {
                (void)formating;
                if (range) {
                    return { range->format.foregroundColor(), {0xff, 0x80, 0x80} };
                }
                return { base.foregroundColor(), {0x80, 0, 0} };
            });
        }
    }
    option.setFlags(flags);
    return option;
}

TextLayout File::getTextLayoutForLine(const ZTextOption &option, int line) {
    TextLayout lay(terminal()->textMetrics(), _doc._text[line]);
    lay.setTextOption(option);
    if (_wrapOption) {
        lay.doLayout(std::max(rect().width() - shiftLinenumber(), 0));
    } else {
        lay.doLayout(std::numeric_limits<unsigned short>::max() - 1);
    }
    return lay;
}

bool File::highlightBracket() {
    QString openBracket = "{[(<";
    QString closeBracket = "}])>";

    if(getHighlightBracket()) {
        for(int i=0; i<openBracket.size(); i++) {
            if (_doc._text[_cursorPositionY][_cursorPositionX] == openBracket[i]) {
                int y = 0;
                int counter = 0;
                int startX = _cursorPositionX+1;
                for (int line = _cursorPositionY; y++ < rect().height() && line < _doc._text.size(); line++) {
                    for(; startX < _doc._text[line].size(); startX++) {
                        if (_doc._text[line][startX] == openBracket[i]) {
                            counter++;
                        } else if (_doc._text[line][startX] == closeBracket[i]) {
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

            if (_doc._text[_cursorPositionY][_cursorPositionX] == closeBracket[i]) {
                int counter = 0;
                int startX = _cursorPositionX-1;
                for (int line = _cursorPositionY; line >= 0;) {
                    for(; startX >= 0; startX--) {
                        if (_doc._text[line][startX] == closeBracket[i]) {
                            counter++;
                        } else if (_doc._text[line][startX] == openBracket[i]) {
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
                        startX=_doc._text[line].size();
                    }
                }
            }
        }
    }
    _bracketX=-1;
    _bracketY=-1;
    return false;
}

int File::shiftLinenumber() {
    if(_linenumber) {
        return QString::number(_doc._text.size()).size() + 1;
    }
    return 0;
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

    ZTextOption option = getTextOption(false);

    auto startSelect = std::make_pair(_startSelectY,_startSelectX);
    auto endSelect = std::make_pair(_endSelectY,_endSelectX);
    if(startSelect > endSelect) {
        std::swap(startSelect,endSelect);
    }
    QVector<TextLayout::FormatRange> highlights;
    const Tui::ZTextStyle base{fg, bg};
    const Tui::ZTextStyle formatingChar{Tui::Colors::darkGray, bg};
    const Tui::ZTextStyle selected{Tui::Colors::darkGray,fg,Tui::ZPainter::Attribute::Bold};
    const Tui::ZTextStyle blockSelected{fg,Tui::Colors::lightGray,Tui::ZPainter::Attribute::Blink | Tui::ZPainter::Attribute::Italic};
    const Tui::ZTextStyle blockSelectedFormatingChar{Tui::Colors::darkGray, Tui::Colors::lightGray, Tui::ZPainter::Attribute::Blink};
    const Tui::ZTextStyle selectedFormatingChar{Tui::Colors::darkGray, fg};

    QString strlinenumber;
    int y = 0;
    int tmpLastLineWidth = 0;
    for (int line = _scrollPositionY; y < rect().height() && line < _doc._text.size(); line++) {
        TextLayout lay = [&] {
                if (line == _cursorPositionY && _doc._text[_cursorPositionY].size() == _cursorPositionX) {
                    return getTextLayoutForLine(getTextOption(true), line);
                }
                return getTextLayoutForLine(option, line);
            }();

        // highlights
        highlights.clear();
        // search matches
        if(_searchText != "") {
            int found = -1;
            if(_searchReg) {
                QRegularExpression rx(_searchText);
                QRegularExpressionMatchIterator i = rx.globalMatch(_doc._text[line]);
                while (i.hasNext()) {
                    QRegularExpressionMatch match = i.next();
                    if(match.capturedLength() > 0) {
                        highlights.append(TextLayout::FormatRange{match.capturedStart(), match.capturedLength(), {Tui::Colors::darkGray,{0xff,0xdd,0},Tui::ZPainter::Attribute::Bold}, selectedFormatingChar});
                    }
                }
            } else {
                while ((found = _doc._text[line].indexOf(_searchText, found + 1, searchCaseSensitivity)) != -1) {
                    highlights.append(TextLayout::FormatRange{found, _searchText.size(), {Tui::Colors::darkGray,{0xff,0xdd,0},Tui::ZPainter::Attribute::Bold}, selectedFormatingChar});
                }
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
        if(_blockSelect) {
            if (line >= std::min(startSelect.first, endSelect.first) && line <= std::max(endSelect.first, startSelect.first)) {
                if(startSelect.second == endSelect.second) {
                    highlights.append(TextLayout::FormatRange{startSelect.second, 1, blockSelected, blockSelectedFormatingChar});
                } else {
                    highlights.append(TextLayout::FormatRange{std::min(startSelect.second, endSelect.second), std::max(startSelect.second, endSelect.second) - std::min(startSelect.second, endSelect.second), selected, selectedFormatingChar});
                }
            }
        } else {
            if (line > startSelect.first && line < endSelect.first) {
                // whole line
                highlights.append(TextLayout::FormatRange{0, _doc._text[line].size(), selected, selectedFormatingChar});
            } else if (line > startSelect.first && line == endSelect.first) {
                // selection ends on this line
                highlights.append(TextLayout::FormatRange{0, endSelect.second, selected, selectedFormatingChar});
            } else if (line == startSelect.first && line < endSelect.first) {
                // selection starts on this line
                highlights.append(TextLayout::FormatRange{startSelect.second, _doc._text[line].size() - startSelect.second, selected, selectedFormatingChar});
            } else if (line == startSelect.first && line == endSelect.first) {
                // selection is contained in this line
                highlights.append(TextLayout::FormatRange{startSelect.second, endSelect.second - startSelect.second, selected, selectedFormatingChar});
            }
        }
        lay.draw(*painter, {-_scrollPositionX + shiftLinenumber(), y}, base, &formatingChar, highlights);
        if (formattingCharacters()) {
            TextLineRef lastLine = lay.lineAt(lay.lineCount()-1);
            tmpLastLineWidth = lastLine.width();
            if (isSelect(_doc._text[line].size(), line)) {
                painter->writeWithAttributes(-_scrollPositionX + lastLine.width() + shiftLinenumber(), y + lastLine.y(), QStringLiteral("¶"),
                                             selectedFormatingChar.foregroundColor(), selectedFormatingChar.backgroundColor(), selectedFormatingChar.attributes());
            } else {
                painter->writeWithAttributes(-_scrollPositionX + lastLine.width() + shiftLinenumber(), y + lastLine.y(), QStringLiteral("¶"),
                                             formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
            }
        }
        if (_cursorPositionY == line) {
            if (focus()) {
                lay.showCursor(*painter, {-_scrollPositionX + shiftLinenumber(), y}, _cursorPositionX);
            }
        }
        //linenumber
        if(_linenumber) {
            strlinenumber = QString::number(line + 1) + QString(" ").repeated(shiftLinenumber() - QString::number(line + 1).size());
            painter->writeWithColors(0, y, strlinenumber, getColor("chr.linenumberFg"), getColor("chr.linenumberBg"));
            for(int i = lay.lineCount() -1; i > 0; i--) {
                painter->writeWithColors(0, y + i, QString(" ").repeated(shiftLinenumber()), getColor("chr.linenumberFg"), getColor("chr.linenumberBg"));
            }
        }
        y += lay.lineCount();
    }
    if (_doc._nonewline) {
        if (formattingCharacters() && y < rect().height() && _scrollPositionX == 0) {
            painter->writeWithAttributes(-_scrollPositionX + tmpLastLineWidth + shiftLinenumber(), y-1, "♦", formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
        }
        painter->writeWithAttributes(0 + shiftLinenumber(), y, "\\ No newline at end of file", formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
    } else {
        if (formattingCharacters() && y < rect().height() && _scrollPositionX == 0) {
            painter->writeWithAttributes(0 + shiftLinenumber(), y, "♦", formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
        }
    }
}

void File::deletePreviousCharacterOrWord(TextLayout::CursorMode mode) {
    QPoint t;
    if(_blockSelect) {
        if(!blockSelectEdit(_cursorPositionX)) {
            for(int line: getBlockSelectedLines()) {
               t = deletePreviousCharacterOrWordAt(mode, _cursorPositionX, line);
            }
            blockSelectEdit(std::max(0,_cursorPositionX -1));
        }
    } else {
        t = deletePreviousCharacterOrWordAt(mode, _cursorPositionX, _cursorPositionY);
        setCursorPosition(t);
    }
    safeCursorPosition();
    _doc.saveUndoStep(this);
}

QPoint File::deletePreviousCharacterOrWordAt(TextLayout::CursorMode mode, int x, int y) {
    if (x > 0) {
        TextLayout lay(terminal()->textMetrics(), _doc._text[y]);
        lay.doLayout(rect().width());
        int leftBoundary = lay.previousCursorPosition(x, mode);
        _doc._text[y].remove(leftBoundary, x - leftBoundary);
        x = leftBoundary;
    } else if (y > 0 && !_blockSelect) {
        x = _doc._text[y -1].size();
        _doc._text[y -1] += _doc._text[y];
        _doc._text.removeAt(y);
        --y;
    }
    return {x, y};
}

void File::deleteNextCharacterOrWord(TextLayout::CursorMode mode) {
    QPoint t;
    if(_blockSelect) {
        /*
        for(int line: getBlockSelectedLines()) {
           if(_doc._text[line].size() <= _cursorPositionX) {
               return;
           }
        }*/
        if(!blockSelectEdit(_cursorPositionX)) {
            for(int line: getBlockSelectedLines()) {
                if(_doc._text[line].size() > _cursorPositionX) {
                    t = deleteNextCharacterOrWordAt(mode, _cursorPositionX, line);
                }
            }
        }
    } else {
        t = deleteNextCharacterOrWordAt(mode, _cursorPositionX, _cursorPositionY);
        setCursorPosition(t);
    }
}

QPoint File::deleteNextCharacterOrWordAt(TextLayout::CursorMode mode, int x, int y) {
    if(y == _doc._text.size() -1 && _doc._text[y].size() == x && !_blockSelect) {
        _doc._nonewline = true;
    } else if(_doc._text[y].size() > x) {
        TextLayout lay(terminal()->textMetrics(), _doc._text[y]);
        lay.doLayout(rect().width());
        int rightBoundary = lay.nextCursorPosition(x, mode);
        _doc._text[y].remove(x, rightBoundary - x);
        _doc.saveUndoStep(this);
    } else if(_doc._text.count() > y +1 && !_blockSelect) {
        if(_doc._text[y].size() < x) {
            _doc._text[y].resize(x,' ');
        }
        _doc._text[y] += _doc._text[y + 1];
        _doc._text.removeAt(y +1);
        _doc.saveUndoStep(this);
    }
    return {x,y};
}

QPoint File::addTabAt(QPoint cursor) {
    if(getTabOption()) {
        for (int i=0; i<getTabsize(); i++) {
            if(cursor.x() % getTabsize() == 0 && i != 0)
                break;
            _doc._text[cursor.y()].insert(cursor.x(), ' ');
            cursor.setX(cursor.x() +1);
        }
    } else {
        _doc._text[cursor.y()].insert(cursor.x(), '\t');
        cursor.setX(cursor.x() +1);
    }
    return {cursor.x(), cursor.y()};
}

int File::getVisibleLines() {
    if(getWrapOption()) {
        return geometry().height() - 2;
    }
    return geometry().height() - 1;
}

void File::appendLine(const QString &line) {
    if(_doc._text.size() == 1 && _doc._text[0] == "") {
        _doc._text.clear();
    }
    _doc._text.append(line);
    if(_follow) {
        _cursorPositionY = _doc._text.size() -1;
    }
    _doc.saveUndoStep(this);
    adjustScrollPosition();
}

void File::insertAtCursorPosition(QVector<QString> str) {
    QPoint t;
    if(_blockSelect) {
        blockSelectEdit(_cursorPositionX);
        if(str.size() -1 == (std::max(_startSelectY, _endSelectY) - std::min(_startSelectY, _endSelectY))) {
            QVector<QString> qst;
            for(int line: getBlockSelectedLines()) {
                qst.append(str.takeAt(0));
                t = insertAtPosition(qst, {_cursorPositionX, line});
                qst.clear();
            }
            blockSelectEdit(t.x());
        } else {
            int offset = 0;
            for(int line: getBlockSelectedLines()) {
                t = insertAtPosition(str, {_cursorPositionX, line + offset});
                offset += str.size() -1;
            }
            if(offset > 0) {
                resetSelect();
            } else {
                blockSelectEdit(t.x());
            }
        }
    } else {
        delSelect();
        t = insertAtPosition(str, {_cursorPositionX, _cursorPositionY});
        setCursorPosition(t);
    }
    safeCursorPosition();
    adjustScrollPosition();
    _doc.saveUndoStep(this);
}

QPoint File::insertAtPosition(QVector<QString> str, QPoint cursor) {
    for(int i = 0; i < str.size() ;i++) {
        _doc._text[cursor.y()].insert(cursor.x(), str[i]);
        cursor.setX(cursor.x() + str[i].size());
        if(i+1 < str.size()) {
            cursor = insertLinebreakAtPosition(cursor);
        }
    }
    return cursor;
}

void File::sortSelecedLines() {
    if(isSelect()) {
        auto lines = getSelectLinesSort();
        std::sort(_doc._text.begin() + lines.first, _doc._text.begin() + lines.second);
    }
}

void File::pasteEvent(Tui::ZPasteEvent *event) {
    QString text = event->text();
    if(_formatting_characters) {
        text.replace(QString("·"), QString(" "));
        text.replace(QString("→"), QString(" "));
        text.replace(QString("¶"), QString(""));
    }
    text.replace(QString("\r\n"), QString('\n'));
    text.replace(QString('\r'), QString('\n'));

    insertAtCursorPosition(text.split('\n').toVector());
}

void File::resizeEvent(Tui::ZResizeEvent *event) {
    if (event->size().height() > 0 && event->size().width() > 0) {
        adjustScrollPosition();
    }
}

void File::setSelectMode(bool event) {
    _selectMode = event;
    modifiedSelectMode(_selectMode);
}

bool File::getSelectMode() {
    return _selectMode;
}

void File::toggleSelectMode() {
    setSelectMode(!_selectMode);
}

bool File::isNewFile() {
    if (_doc._filename == "NEWFILE") {
        return true;
    }
    return false;
}

void File::keyEvent(Tui::ZKeyEvent *event) {
    QString text = event->text();
    bool extendSelect = event->modifiers() == Qt::ShiftModifier || getSelectMode();
    bool extendBlockSelect = event->modifiers() == (Qt::AltModifier | Qt::ShiftModifier);
    //bool extendBlockSelect = getSelectMode();

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
            event->key() != Qt::Key_F1 &&
            event->key() != Qt::Key_F2 &&
            event->key() != Qt::Key_F3 &&
            event->key() != Qt::Key_F4 &&
            event->key() != Qt::Key_F5 &&
            event->key() != Qt::Key_F6 &&
            event->key() != Qt::Key_F7 &&
            event->key() != Qt::Key_F8 &&
            event->key() != Qt::Key_F9 &&
            event->key() != Qt::Key_F10 &&
            event->key() != Qt::Key_F11 &&
            event->key() != Qt::Key_F12 &&
            event->key() != Qt::Key_Tab &&
            event->modifiers() != Qt::ControlModifier &&
            !(event->text() == "S" && (event->modifiers() == Qt::AltModifier || event->modifiers() == Qt::AltModifier | Qt::ShiftModifier)) &&
            !_blockSelect
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
            event->key() != Qt::Key_F1 &&
            event->key() != Qt::Key_F2 &&
            event->key() != Qt::Key_F3 &&
            event->key() != Qt::Key_F5 &&
            event->key() != Qt::Key_F6 &&
            event->key() != Qt::Key_F7 &&
            event->key() != Qt::Key_F8 &&
            event->key() != Qt::Key_F9 &&
            event->key() != Qt::Key_F10 &&
            event->key() != Qt::Key_F11 &&
            event->key() != Qt::Key_F12 &&
        (

            !extendSelect &&
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
        ) && event->key() != Qt::Key_F4 && !_blockSelect
        && !(event->text() == "S" && (event->modifiers() == Qt::AltModifier || event->modifiers() == Qt::AltModifier | Qt::ShiftModifier))) {
        resetSelect();
        adjustScrollPosition();
    }

    if (_formatting_characters && (event->text() == "·" || event->text() == "→") && event->modifiers() == 0) {
        _doc._text[_cursorPositionY].insert(_cursorPositionX, ' ');
        ++_cursorPositionX;
        safeCursorPosition();
    } else if (_formatting_characters && event->text() == "¶" && event->modifiers() == 0) {
        //do not add the character
    } else if(text.size() && event->modifiers() == 0) {
        if(_blockSelect) {
            blockSelectEdit(_cursorPositionX);
            for(int line: getBlockSelectedLines()) {
                _doc._text[line].insert(_cursorPositionX, text);
            }
            blockSelectEdit(_cursorPositionX + text.size());
            if (isOverwrite()) {
                deleteNextCharacterOrWord(TextLayout::SkipCharacters);
            }
        } else {
            if(_doc._text[_cursorPositionY].size() < _cursorPositionX) {
                _doc._text[_cursorPositionY].resize(_cursorPositionX, ' ');
            }
            if (isOverwrite() && _doc._text[_cursorPositionY].size() > _cursorPositionX) {
                deleteNextCharacterOrWord(TextLayout::SkipCharacters);
            }
            _doc._text[_cursorPositionY].insert(_cursorPositionX, text);
            _cursorPositionX += text.size();
        }
        safeCursorPosition();
        adjustScrollPosition();
        _doc.saveUndoStep(this, text != " ");
    } else if (event->text() == "S" && (event->modifiers() == Qt::AltModifier || event->modifiers() == Qt::AltModifier | Qt::ShiftModifier)  && isSelect()) {
        // Alt + Shift + s sort selected lines
        sortSelecedLines();
        update();
    } else if(event->key() == Qt::Key_Left && (event->modifiers() == 0 || event->modifiers() == Qt::ControlModifier || extendSelect || extendBlockSelect)) {
        selectCursorPosition(event->modifiers());
        if (_cursorPositionX > 0) {
            TextLayout lay(terminal()->textMetrics(), _doc._text[_cursorPositionY]);
            lay.doLayout(rect().width());
            auto mode = event->modifiers() & Qt::ControlModifier ? TextLayout::SkipWords : TextLayout::SkipCharacters;
            _cursorPositionX = lay.previousCursorPosition(_cursorPositionX, mode);
        } else if (_cursorPositionY > 0) {
            _cursorPositionY -= 1;
            _cursorPositionX = _doc._text[_cursorPositionY].size();
        }
        safeCursorPosition();
        adjustScrollPosition();
        selectCursorPosition(event->modifiers());
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Right && (event->modifiers() == 0 || event->modifiers() == Qt::ControlModifier || extendSelect || extendBlockSelect)) {
        selectCursorPosition(event->modifiers());
        if (_cursorPositionX < _doc._text[_cursorPositionY].size()) {
            TextLayout lay(terminal()->textMetrics(), _doc._text[_cursorPositionY]);
            lay.doLayout(rect().width());
            auto mode = event->modifiers() & Qt::ControlModifier ? TextLayout::SkipWords : TextLayout::SkipCharacters;
            _cursorPositionX = lay.nextCursorPosition(_cursorPositionX, mode);
        } else if (_cursorPositionY + 1 < _doc._text.size()) {
            ++_cursorPositionY;
            _cursorPositionX = 0;
        }
        safeCursorPosition();
        adjustScrollPosition();
        selectCursorPosition(event->modifiers());
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Down && (event->modifiers() == 0 || extendSelect || extendBlockSelect)) {
        selectCursorPosition(event->modifiers());
        if (_doc._text.size() -1 > _cursorPositionY) {
            ++_cursorPositionY;
            TextLayout lay = getTextLayoutForLine(getTextOption(false), _cursorPositionY);
            TextLineRef la = lay.lineAt(0);
            _cursorPositionX = la.xToCursor(_saveCursorPositionX);
        }
        adjustScrollPosition();
        selectCursorPosition(event->modifiers());
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Up && (event->modifiers() == 0 || extendSelect || extendBlockSelect)) {
        selectCursorPosition(event->modifiers());
        if (_cursorPositionY > 0) {
            --_cursorPositionY;
            TextLayout lay = getTextLayoutForLine(getTextOption(false), _cursorPositionY);
            TextLineRef la = lay.lineAt(0);
            _cursorPositionX = la.xToCursor(_saveCursorPositionX);
        }
        adjustScrollPosition();
        selectCursorPosition(event->modifiers());
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Home && (event->modifiers() == 0 || extendSelect || extendBlockSelect)) {
        selectCursorPosition(event->modifiers());
        if(_cursorPositionX == 0) {
            for (int i = 0; i <= _doc._text[_cursorPositionY].size()-1; i++) {
                if(_doc._text[_cursorPositionY][i] != ' ' && _doc._text[_cursorPositionY][i] != '\t') {
                    _cursorPositionX = i;
                    break;
                }
            }
        } else {
            _cursorPositionX = 0;
        }
        selectCursorPosition(event->modifiers());
        safeCursorPosition();
        adjustScrollPosition();
        _doc._collapseUndoStep = false;
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
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_End && (event->modifiers() == 0 || extendSelect)) {
        selectCursorPosition(event->modifiers());
        _cursorPositionX = _doc._text[_cursorPositionY].size();
        selectCursorPosition(event->modifiers());
        safeCursorPosition();
        adjustScrollPosition();
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_End && (event->modifiers() == Qt::ControlModifier || (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)))) {
        selectCursorPosition(event->modifiers());
        _cursorPositionY = _doc._text.size() -1;
        _cursorPositionX = _doc._text[_cursorPositionY].size();

        selectCursorPosition(event->modifiers());
        safeCursorPosition();
        adjustScrollPosition();
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_PageDown && (event->modifiers() == 0 || extendSelect)) {
        if(extendSelect) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_doc._text.size() > _cursorPositionY + getVisibleLines()) {
            _cursorPositionY += getVisibleLines();
        } else {
            _cursorPositionY = _doc._text.size() -1;
        }
        if(extendSelect) {
            _cursorPositionX = _doc._text[_cursorPositionY].size();
            select(_cursorPositionX, _cursorPositionY);
        }
        safeCursorPosition();
        adjustScrollPosition();
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_PageDown && ( event->modifiers() == Qt::ControlModifier ||
              (event->modifiers() == Qt::ControlModifier && event->modifiers() == Qt::ControlModifier) ) ) {
        if(extendSelect) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionX = 1;
        _cursorPositionY = 1;
        if(extendSelect) {
            _cursorPositionX = _doc._text[_cursorPositionY].size();
            select(_cursorPositionX, _cursorPositionY);
        }
        safeCursorPosition();
        adjustScrollPosition();
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_PageUp && (event->modifiers() == Qt::ControlModifier ||
              (event->modifiers() == Qt::ControlModifier || event->modifiers() == Qt::ControlModifier) ) ) {
        if(extendSelect) {
            select(_cursorPositionX, _cursorPositionY);
        }
        _cursorPositionX = _doc._text[_doc._text.size() -1].size();
        _cursorPositionY = _doc._text.size();
        if(extendSelect) {
          _cursorPositionX = 0;
          select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_PageUp && (event->modifiers() == 0 || extendSelect)) {
        if(extendSelect) {
            select(_cursorPositionX, _cursorPositionY);
        }
        if (_cursorPositionY > getVisibleLines()) {
            _cursorPositionY -= getVisibleLines();
        } else {
            _cursorPositionY = 0;
            _scrollPositionY = 0;
        }
        if(extendSelect) {
            _cursorPositionX = 0;
            select(_cursorPositionX, _cursorPositionY);
        }
        adjustScrollPosition();
        _doc._collapseUndoStep = false;
    } else if(event->key() == Qt::Key_Enter && (event->modifiers() & ~Qt::KeypadModifier) == 0) {
        insertLinebreak();
        safeCursorPosition();
        _doc.saveUndoStep(this);
        adjustScrollPosition();
    } else if(event->key() == Qt::Key_Tab && event->modifiers() == 0) {
        QPoint t;
        if (_blockSelect) {
            blockSelectEdit(_cursorPositionX);
            for(int line: getBlockSelectedLines()) {
                t = addTabAt({_cursorPositionX,line});
            }
            blockSelectEdit(t.x());
        } else if(isSelect()) {
            //Tabs an in Markierten zeilen hinzufügen
            for(int selectedLine = std::min(getSelectLines().first, getSelectLines().second); selectedLine <= std::max(getSelectLines().first, getSelectLines().second); selectedLine++) {
                if(_doc._text[selectedLine].size() > 0) {
                    if(getTabOption()) {
                        _doc._text[selectedLine].insert(0, QString(" ").repeated(getTabsize()));
                    } else {
                        _doc._text[selectedLine].insert(0, '\t');
                    }
                }
            }
            //Zeilen neu Markieren
            selectLines(getSelectLines().first, getSelectLines().second);
        } else {
            if(_eatSpaceBeforeTabs && _tabOption) {
                // If spaces in front of a tab
                int i = 0;
                for(; _doc._text[_cursorPositionY][i] == ' '; i++);
                if(i > _cursorPositionX) {
                    _cursorPositionX = i;
                    setCursorPosition({i, _cursorPositionY});
                }
            }
            // a normal tab
            t = addTabAt({_cursorPositionX, _cursorPositionY});
            setCursorPosition(t);
        }
        safeCursorPosition();
        _doc.saveUndoStep(this);

    } else if(event->key() == Qt::Key_Tab && event->modifiers() == Qt::ShiftModifier) {
        //Nich markierte Zeile verschiben
        int selectedLine = _cursorPositionY;
        int selectedEnd = _cursorPositionY;
        if(isSelect()) {
           selectedLine = std::min(getSelectLines().first, getSelectLines().second);
           selectedEnd = std::max(getSelectLines().first, getSelectLines().second);
        }

        for(; selectedLine <= selectedEnd; selectedLine++) {
            if(_doc._text[selectedLine][0] == '\t') {
               _doc._text[selectedLine].remove(0,1);
               if(selectedLine == _cursorPositionY && !isSelect()) {
                   setCursorPosition({_cursorPositionX - 1,_cursorPositionY});
               }
            } else if (_doc._text[selectedLine].mid(0,getTabsize()) == QString(" ").repeated(getTabsize())) {
                _doc._text[selectedLine].remove(0,getTabsize());
                if(selectedLine == _cursorPositionY && !isSelect()) {
                    setCursorPosition({_cursorPositionX - getTabsize(),_cursorPositionY});
                }
            } else {
                for(int i = 0; i < getTabsize(); i++) {
                    if (_doc._text[selectedLine].mid(0,1) == QString(" ")) {
                        _doc._text[selectedLine].remove(0,1);
                        if(selectedLine == _cursorPositionY && !isSelect()) {
                            setCursorPosition({_cursorPositionX - 1,_cursorPositionY});
                        }
                    } else {
                        break;
                    }
                }
            }
        }

        if(isSelect()) {
            selectLines(getSelectLines().first, getSelectLines().second);
        }

        safeCursorPosition();
        _doc.saveUndoStep(this);

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
        _doc.undo(this);
    } else if (event->text() == "y" && event->modifiers() == Qt::ControlModifier) {
        _doc.redo(this);
    } else if (event->text() == "a" && event->modifiers() == Qt::ControlModifier) {
        //STRG + a
        selectAll();
        _doc._collapseUndoStep = false;
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
        if (_doc._text.size() -1 > _scrollPositionY) {
            ++_scrollPositionY;
        }
        adjustScrollPosition();
    } else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier) && event->key() == Qt::Key_Up) {

        //Nich markierte Zeile verschiben
        bool _resetSelect = false;
        if(!isSelect()) {
            selectLines(_cursorPositionY, _cursorPositionY);
            _resetSelect = true;
        }
        if(std::min(getSelectLines().first, getSelectLines().second) > 0) {
            //Nach oben verschieben
            if(getSelectLines().first >= getSelectLines().second) {
                _doc._text.insert(getSelectLines().first +1, _doc._text[getSelectLines().second -1]);
                _doc._text.remove(getSelectLines().second -1);
            } else {
                _doc._text.insert(getSelectLines().second +1, _doc._text[getSelectLines().first -1]);
                _doc._text.remove(getSelectLines().first -1);
            }

            //Markirung Nachziehen
            if (_resetSelect) {
                setCursorPosition({0, std::min(_startSelectY -1, _endSelectY -1)});
                resetSelect();
            } else {
                selectLines(getSelectLines().first -1, getSelectLines().second -1);
            }
            _doc.saveUndoStep(this);
        } else if(_resetSelect) {
            resetSelect();
        }
    } else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier) && event->key() == Qt::Key_Down) {
        if(std::max(getSelectLines().first, getSelectLines().second) < _doc._text.size() -1) {
            //Nich markierte Zeile verschiben
            bool _resetSelect = false;
            if(!isSelect()) {
                selectLines(_cursorPositionY, _cursorPositionY);
                _resetSelect = true;
            }

            //Nach unten verschieben
            if(getSelectLines().first > getSelectLines().second) {
                _doc._text.insert(getSelectLines().second, _doc._text[getSelectLines().first +1]);
                _doc._text.remove(getSelectLines().first +2);
            } else {
                _doc._text.insert(getSelectLines().first, _doc._text[getSelectLines().second +1]);
                _doc._text.remove(getSelectLines().second +2);
            }

            //Markirung Nachziehen
            if (_resetSelect) {
                setCursorPosition({std::max(_startSelectX, _endSelectX), std::max(_startSelectY +1, _endSelectY +1)});
                resetSelect();
            } else {
                selectLines(getSelectLines().first +1, getSelectLines().second +1);
            }
            _doc.saveUndoStep(this);
        }
    } else if (event->key() == Qt::Key_Escape && event->modifiers() == 0) {
        setSearchText("");
        _blockSelect = false;
    } else if (event->key() == Qt::Key_Insert && event->modifiers() == 0) {
        toggleOverwrite();
    } else if (event->key() == Qt::Key_F4 && event->modifiers() == 0) {
        toggleSelectMode();
    } else {
        Tui::ZWidget::keyEvent(event);
    }
}

void File::selectCursorPosition(Qt::KeyboardModifiers modifiers) {
    if(modifiers == Qt::ShiftModifier || getSelectMode()) {
        select(_cursorPositionX, _cursorPositionY);
    } else if (modifiers == (Qt::AltModifier | Qt::ShiftModifier)) {
        blockSelect(_cursorPositionX, _cursorPositionY);
    } else {
        resetSelect();
    }
}

void File::adjustScrollPosition() {
    if(geometry().width() <= 0 && geometry().height() <= 0) {
        return;
    }
    ZTextOption option = getTextOption(false);
    option.setWrapMode(ZTextOption::NoWrap);
    TextLayout lay = getTextLayoutForLine(option, _cursorPositionY);
    int cursorColumn = lay.lineAt(0).cursorToX(_cursorPositionX, TextLineRef::Leading);

    int viewWidth = geometry().width() - shiftLinenumber();
    //x
    if (!_wrapOption) {
        if (cursorColumn - _scrollPositionX >= viewWidth) {
             _scrollPositionX = cursorColumn - viewWidth + 1;
        }
        if (cursorColumn > 0) {
            if (cursorColumn - _scrollPositionX < 1) {
                _scrollPositionX = cursorColumn - 1;
            }
        } else {
            _scrollPositionX = 0;
        }
    } else {
        _scrollPositionX = 0;
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

        if (_doc._text.size() - _scrollPositionY < geometry().height() -1) {
            _scrollPositionY = std::max(0,_doc._text.size() - geometry().height() +1);
        }
    } else {
        option = getTextOption(false);
        int y = 0;
        QVector<int> sizes;
        for (int line = _scrollPositionY; line <= _cursorPositionY && line < _doc._text.size(); line++) {
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


    if (_doc._text[_cursorPositionY].size() < _cursorPositionX) {
        _cursorPositionX = _doc._text[_cursorPositionY].size();
    }

    int _utf8PositionX = _doc._text[_cursorPositionY].left(_cursorPositionX).toUtf8().size();
    cursorPositionChanged(cursorColumn, _utf8PositionX, _cursorPositionY);
    scrollPositionChanged(_scrollPositionX, _scrollPositionY);

    int max=0;
    for (int i=_scrollPositionY; i < _doc._text.count() && i < _scrollPositionY + geometry().height(); i++) {
        if(max < _doc._text[i].count())
            max = _doc._text[i].count();
    }
    textMax(max - viewWidth, _doc._text.count() - geometry().height());

    update();
}

void File::safeCursorPosition() {
    TextLayout lay = getTextLayoutForLine(getTextOption(false), _cursorPositionY);
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

