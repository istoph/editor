// SPDX-License-Identifier: BSL-1.0

#include "document.h"

#include <Tui/Misc/SurrogateEscape.h>

#include "file.h"


Document::Document(QObject *parent) : QObject (parent) {
    _lines.append(QString());
    initalUndoStep(nullptr);
}

void Document::reset() {
    _lines.clear();
    _lines.append(QString());
    initalUndoStep(nullptr);
}

void Document::writeTo(QIODevice *file, bool crLfMode) {
    for (int i = 0; i < lineCount(); i++) {
        file->write(Tui::Misc::SurrogateEscape::encode(_lines[i]));
        if (i + 1 == lineCount() && _nonewline) {
            // omit newline
        } else {
            if (crLfMode) {
                file->write("\r\n", 2);
            } else {
                file->write("\n", 1);
            }
        }
    }
}

bool Document::readFrom(QIODevice *file) {
    _lines.clear();
    QByteArray lineBuf;
    lineBuf.resize(16384);
    while (!file->atEnd()) { // each line
        int lineBytes = 0;
        _nonewline = true;
        while (!file->atEnd()) { // chunks of the line
            int res = file->readLine(lineBuf.data() + lineBytes, lineBuf.size() - 1 - lineBytes);
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

        QString text = Tui::Misc::SurrogateEscape::decode(lineBuf.constData(), lineBytes);
        _lines.append(text);
    }

    if (_lines.isEmpty()) {
        _lines.append("");
        _nonewline = true;
    }

    bool allLinesCrLf = false;

    for (int l = 0; l < lineCount(); l++) {
        if (_lines[l].size() >= 1 && _lines[l].at(_lines[l].size() - 1) == QChar('\r')) {
            allLinesCrLf = true;
        } else {
            allLinesCrLf = false;
            break;
        }
    }
    if (allLinesCrLf) {
        for (int i = 0; i < lineCount(); i++) {
            _lines[i].remove(_lines[i].size() - 1, 1);
        }
    }

    return allLinesCrLf;
}

int Document::lineCount() const {
    return _lines.size();
}

QString Document::line(int line) const {
    return _lines[line];
}

int Document::lineCodeUnits(int line) const {
    return _lines[line].size();
}

QVector<QString> Document::getLines() const {
    return _lines;
}

bool Document::isModified() const {
    return _currentUndoStep != _savedUndoStep;
}

void Document::setNoNewline(bool value) {
    _nonewline = value;
}

bool Document::noNewLine() const {
    return _nonewline;
}

QString Document::filename() const {
    return _filename;
}

void Document::setFilename(const QString &filename) {
    _filename = filename;
}

void Document::clearCollapseUndoStep() {
    _collapseUndoStep = false;
}

void Document::tmp_sortLines(int first, int last, TextCursor *cursorForUndoStep) {
    std::sort(_lines.begin() + first, _lines.begin() + last);
    saveUndoStep(cursorForUndoStep);
}

void Document::tmp_moveLine(int from, int to, TextCursor *cursorForUndoStep) {
    _lines.insert(to, _lines[from]);
    if (from < to) {
        _lines.remove(from);
    } else {
        _lines.remove(from + 1);
    }
    saveUndoStep(cursorForUndoStep);
}

void Document::undo(TextCursor *cursor) {
    if(_undoSteps.isEmpty()) {
        return;
    }

    if (_currentUndoStep == 0) {
        return;
    }

    --_currentUndoStep;

    _lines = _undoSteps[_currentUndoStep].text;
    cursor->setPosition({_undoSteps[_currentUndoStep].cursorPositionX, _undoSteps[_currentUndoStep].cursorPositionY});
    emitModifedSignals();
}

void Document::redo(TextCursor *cursor) {
    if(_undoSteps.isEmpty()) {
        return;
    }

    if (_currentUndoStep + 1 >= _undoSteps.size()) {
        return;
    }

    ++_currentUndoStep;

    _lines = _undoSteps[_currentUndoStep].text;
    cursor->setPosition({_undoSteps[_currentUndoStep].cursorPositionX, _undoSteps[_currentUndoStep].cursorPositionY});
    emitModifedSignals();
}

Document::UndoGroup Document::startUndoGroup(TextCursor *cursor) {
    _groupUndo++;
    return UndoGroup{this, cursor};
}

void Document::closeUndoGroup(TextCursor *cursor) {
    if(_groupUndo <= 1) {
        _groupUndo = 0;
        if (_undoStepCreationDeferred) {
            saveUndoStep(cursor);
            _undoStepCreationDeferred = false;
        }
    } else {
        _groupUndo--;
    }
}

Document::UndoGroup::~UndoGroup() {
    if (!_closed) {
        closeGroup();
    }
}

void Document::UndoGroup::closeGroup() {
    _doc->closeUndoGroup(_cursor);
    _closed = true;
}

Document::UndoGroup::UndoGroup(Document *doc, TextCursor *cursor)
    : _doc(doc), _cursor(cursor)
{
}

void Document::initalUndoStep(TextCursor *cursor) {
    _collapseUndoStep = false;
    _groupUndo = 0;
    _undoStepCreationDeferred = false;
    _undoSteps.clear();
    if (cursor) {
        const auto [endCodeUnit, endLine] = cursor->position();
        _undoSteps.append({ _lines, endCodeUnit, endLine});
    } else {
        _undoSteps.append({ _lines, 0, 0});
    }
    _currentUndoStep = 0;
    _savedUndoStep = _currentUndoStep;
    emitModifedSignals();
}

void Document::saveUndoStep(TextCursor *cursor, bool collapsable) {
    if (_groupUndo == 0) {
        if (_currentUndoStep + 1 != _undoSteps.size()) {
            _undoSteps.resize(_currentUndoStep + 1);
        } else if (_collapseUndoStep && collapsable && _currentUndoStep != _savedUndoStep) {
            _undoSteps.removeLast();
        }
        const auto [endCodeUnit, endLine] = cursor->position();
        _undoSteps.append({ _lines, endCodeUnit, endLine});
        _currentUndoStep = _undoSteps.size() - 1;
        _collapseUndoStep = collapsable;
        emitModifedSignals();
    } else {
        _undoStepCreationDeferred = true;
    }
}

void Document::removeFromLine(int line, int codeUnitStart, int codeUnits) {
    _lines[line].remove(codeUnitStart, codeUnits);
}

void Document::insertIntoLine(int line, int codeUnitStart, const QString &data) {
    _lines[line].insert(codeUnitStart, data);
}

void Document::appendToLine(int line, const QString &data) {
    _lines[line].append(data);
}

void Document::removeLines(int start, int count) {
    _lines.remove(start, count);
}

void Document::insertLine(int before, const QString &data) {
    _lines.insert(before, data);
}

void Document::splitLine(TextCursor::Position pos) {
    _lines.insert(pos.line + 1, _lines[pos.line].mid(pos.codeUnit));
    _lines[pos.line].resize(pos.codeUnit);
}

void Document::emitModifedSignals() {
    // TODO: Ideally emit these only when changed
    undoAvailable(_undoSteps.size() && _currentUndoStep != 0);
    redoAvailable(_undoSteps.size() && _currentUndoStep + 1 < _undoSteps.size());
    modificationChanged(isModified());
}

bool operator<(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.line, a.codeUnit) < std::tie(b.line, b.codeUnit);
}

bool operator>(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.line, a.codeUnit) > std::tie(b.line, b.codeUnit);
}

bool operator==(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.codeUnit, a.line) == std::tie(b.codeUnit, b.line);
}


TextCursor::TextCursor(Document *doc, Tui::ZWidget *widget, std::function<Tui::ZTextLayout(int line, bool wrappingAllowed)> createTextLayout)
    : _doc(doc), _widget(widget), _createTextLayout(createTextLayout) {
}

void TextCursor::insertText(const QString &text) {
    auto lines = text.split("\n");

    removeSelectedText();
    if (_doc->_nonewline && atEnd() && lines.size() > 1 && lines.last().size() == 0) {
        lines.removeLast();
        _doc->_nonewline = false;
    }
    _doc->insertIntoLine(_cursorPositionY, _cursorPositionX, lines.front());
    _cursorPositionX += lines.front().size();
    for (int i = 1; i < lines.size(); i++) {
        _doc->splitLine({_cursorPositionX, _cursorPositionY});
        _cursorPositionY++;
        _cursorPositionX = 0;
        _doc->insertIntoLine(_cursorPositionY, _cursorPositionX, lines.at(i));
        _cursorPositionX = lines.at(i).size();
    }
    _anchorPositionX = _cursorPositionX;
    _anchorPositionY = _cursorPositionY;

    Tui::ZTextLayout lay = _createTextLayout(_cursorPositionY, true);
    updateVerticalMovementColumn(lay);
    _doc->saveUndoStep(this);
}

void TextCursor::removeSelectedText() {
    if (!hasSelection()) {
        return;
    }

    const Position start = selectionStartPos();
    const Position end = selectionEndPos();

    if (start.line == end.line) {
        // selection only on one line
        _doc->removeFromLine(start.line, start.codeUnit, end.codeUnit - start.codeUnit);
    } else {
        _doc->removeFromLine(start.line, start.codeUnit, _doc->_lines[start.line].size() - start.codeUnit);
        const auto orignalTextLines = _doc->_lines.size();
        if (start.line + 1 < end.line) {
            _doc->removeLines(start.line + 1, end.line - start.line - 1);
        }
        if (end.line == orignalTextLines) {
            // selected until the end of buffer, no last selection line to edit
        } else {
            _doc->appendToLine(start.line, _doc->_lines[start.line + 1].mid(end.codeUnit));
            if (start.line + 1 < _doc->_lines.size()) {
                _doc->removeLines(start.line + 1, 1);
            } else {
                _doc->removeFromLine(start.line + 1, 0, _doc->_lines[start.line + 1].size());
            }
        }
    }
    clearSelection();
    setPosition(start);

    _doc->saveUndoStep(this);
}

void TextCursor::clearSelection() {
    _anchorPositionX = _cursorPositionX;
    _anchorPositionY = _cursorPositionY;
}

QString TextCursor::selectedText() const {
    if (!hasSelection()) {
        return "";
    }

    const Position start = selectionStartPos();
    const Position end = selectionEndPos();

    if (start.line == end.line) {
        // selection only on one line
        return _doc->_lines[start.line].mid(start.codeUnit, end.codeUnit - start.codeUnit);
    } else {
        QString res = _doc->_lines[start.line].mid(start.codeUnit);
        for (int line = start.line + 1; line < end.line; line++) {
            res += "\n";
            res += _doc->_lines[line];
        }
        res += "\n";
        res += _doc->_lines[end.line].mid(0, end.codeUnit);
        return res;
    }
}

void TextCursor::deleteCharacter() {
    if (!hasSelection()) {
        moveCharacterRight(true);
    }

    removeSelectedText();
}

void TextCursor::deletePreviousCharacter() {
    if (!hasSelection()) {
        moveCharacterLeft(true);
    }

    removeSelectedText();
}

void TextCursor::deleteWord() {
    if (!hasSelection()) {
        moveWordRight(true);
    }

    removeSelectedText();
}

void TextCursor::deletePreviousWord() {
    if (!hasSelection()) {
        moveWordLeft(true);
    }

    removeSelectedText();
}

void TextCursor::deleteLine() {
    clearSelection();
    moveToStartOfLine();
    moveToEndOfLine(true);
    removeSelectedText();
    if (atEnd()) {
        deletePreviousCharacter();
    } else {
        deleteCharacter();
    }
}

void TextCursor::moveCharacterLeft(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit) {
        Tui::ZTextLayout lay = _createTextLayout(_cursorPositionY, false);
        setPosition({lay.previousCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipCharacters), currentLine},
                    extendSelection);
    } else if (currentLine > 0) {
        setPosition({_doc->_lines[currentLine - 1].size(), currentLine - 1}, extendSelection);
    } else {
        // here we update the selection and the vertical movemend position
        setPosition({currentCodeUnit, currentLine}, extendSelection);
    }
}

void TextCursor::moveCharacterRight(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit < _doc->_lines[currentLine].size()) {
        Tui::ZTextLayout lay = _createTextLayout(_cursorPositionY, false);
        setPosition({lay.nextCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipCharacters), currentLine},
                    extendSelection);
    } else if (currentLine + 1 < _doc->_lines.size()) {
        setPosition({0, currentLine + 1}, extendSelection);
    } else {
        // here we update the selection and the vertical movemend position
        setPosition({currentCodeUnit, currentLine}, extendSelection);
    }
}

void TextCursor::moveWordLeft(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit) {
        Tui::ZTextLayout lay = _createTextLayout(_cursorPositionY, false);
        setPosition({lay.previousCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipWords), currentLine},
                    extendSelection);
    } else if (currentLine > 0) {
        setPosition({_doc->_lines[currentLine - 1].size(), currentLine - 1}, extendSelection);
    }
}

void TextCursor::moveWordRight(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit < _doc->_lines[currentLine].size()) {
        Tui::ZTextLayout lay = _createTextLayout(_cursorPositionY, false);
        setPosition({lay.nextCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipWords), currentLine},
                    extendSelection);
    } else if (currentLine + 1 < _doc->_lines.size()) {
        setPosition({0, currentLine + 1}, extendSelection);
    }
}

void TextCursor::moveUp(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();

    Tui::ZTextLayout layStarting = _createTextLayout(currentLine, true);
    Tui::ZTextLineRef lineStarting = layStarting.lineForTextPosition(currentCodeUnit);

    if (lineStarting.lineNumber() > 0) {
        int fineMoveCodeUnit = layStarting.lineAt(lineStarting.lineNumber() - 1).xToCursor(_saveCursorPositionX);
        if (layStarting.lineForTextPosition(fineMoveCodeUnit).lineNumber() != lineStarting.lineNumber() - 1) {
            // When the line is shorter than _saveCursorPositionX the cursor ends up in the next line,
            // which is not intended, move once to the left in that case
            fineMoveCodeUnit = layStarting.previousCursorPosition(fineMoveCodeUnit, Tui::ZTextLayout::SkipCharacters);
        }

        setPositionPreservingVerticalMovementColumn(
                {fineMoveCodeUnit, currentLine},
                extendSelection);
        return;
    }

    if (currentLine > 0) {
        Tui::ZTextLayout lay = _createTextLayout(currentLine - 1, true);
        Tui::ZTextLineRef la = lay.lineAt(lay.lineCount() - 1);
        setPositionPreservingVerticalMovementColumn({la.xToCursor(_saveCursorPositionX), currentLine - 1}, extendSelection);
    }
}

void TextCursor::moveDown(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();

    Tui::ZTextLayout layStarting = _createTextLayout(currentLine, true);
    Tui::ZTextLineRef lineStarting = layStarting.lineForTextPosition(currentCodeUnit);

    if (lineStarting.lineNumber() + 1 < layStarting.lineCount()) {
        int fineMoveCodeUnit = layStarting.lineAt(lineStarting.lineNumber() + 1).xToCursor(_saveCursorPositionX);
        if (layStarting.lineForTextPosition(fineMoveCodeUnit).lineNumber() != lineStarting.lineNumber() + 1) {
            // When the line is shorter than _saveCursorPositionX the cursor ends up in the next line,
            // which is not intended, move once to the left in that case
            fineMoveCodeUnit = layStarting.previousCursorPosition(fineMoveCodeUnit, Tui::ZTextLayout::SkipCharacters);
        }

        setPositionPreservingVerticalMovementColumn(
                {fineMoveCodeUnit,
                currentLine}, extendSelection);
        return;
    }

    if (currentLine < _doc->_lines.size() - 1) {
        Tui::ZTextLayout lay = _createTextLayout(currentLine + 1, true);
        Tui::ZTextLineRef la = lay.lineAt(0);
        setPositionPreservingVerticalMovementColumn({la.xToCursor(_saveCursorPositionX), currentLine + 1}, extendSelection);
    }
}

void TextCursor::moveToStartOfLine(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    setPosition({0, currentLine}, extendSelection);
}

void TextCursor::moveToStartIndentedText(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();

    int i = 0;
    for (; i < _doc->_lines[currentLine].size(); i++) {
        if (_doc->_lines[currentLine][i] != ' ' && _doc->_lines[currentLine][i] != '\t') {
            break;
        }
    }

    setPosition({i, currentLine}, extendSelection);
}

void TextCursor::moveToEndOfLine(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    setPosition({_doc->_lines[currentLine].size(), currentLine}, extendSelection);
}

void TextCursor::moveToStartOfDocument(bool extendSelection) {
    setPosition({0, 0}, extendSelection);
}

void TextCursor::moveToEndOfDocument(bool extendSelection) {
    setPosition({_doc->_lines.last().size(), _doc->_lines.size() - 1}, extendSelection);
}

TextCursor::Position TextCursor::position() {
    return Position{_cursorPositionX, _cursorPositionY};
}

void TextCursor::setPosition(TextCursor::Position pos, bool extendSelection) {
    setPositionPreservingVerticalMovementColumn(pos, extendSelection);
    if (!_widget || _widget->terminal()) { // TODO rethink this hack to allow using setPosition before terminal is connected
        Tui::ZTextLayout lay = _createTextLayout(_cursorPositionY, true);
        updateVerticalMovementColumn(lay);
    }
}

void TextCursor::setPositionPreservingVerticalMovementColumn(TextCursor::Position pos, bool extendSelection) {
    _cursorPositionY = std::max(std::min(pos.line, _doc->_lines.size() - 1), 0);
    _cursorPositionX = std::max(std::min(pos.codeUnit, _doc->_lines[_cursorPositionY].size()), 0);

    // We are not allowed to jump between characters. Therefore, we go once to the left and again to the right.
    if (_cursorPositionX > 0) {
        if (!_widget || _widget->terminal()) { // TODO rethink this hack to allow using setPosition before terminal is connected
            Tui::ZTextLayout lay = _createTextLayout(_cursorPositionY, false);
            _cursorPositionX = lay.previousCursorPosition(_cursorPositionX, Tui::ZTextLayout::SkipCharacters);
            _cursorPositionX = lay.nextCursorPosition(_cursorPositionX, Tui::ZTextLayout::SkipCharacters);
        }
    }

    if (!extendSelection) {
        _anchorPositionX = _cursorPositionX;
        _anchorPositionY = _cursorPositionY;
    }
}

TextCursor::Position TextCursor::anchor() {
    return Position{_anchorPositionX, _anchorPositionY};
}

void TextCursor::setAnchorPosition(TextCursor::Position pos) {
    clearSelection();

    _anchorPositionY = std::max(std::min(pos.line, _doc->_lines.size() - 1), 0);
    _anchorPositionX = std::max(std::min(pos.codeUnit, _doc->_lines[_anchorPositionY].size()), 0);

    // We are not allowed to jump between characters. Therefore, we go once to the left and again to the right.
    if (_anchorPositionX > 0) {
        Tui::ZTextLayout lay = _createTextLayout(_anchorPositionY, false);
        _anchorPositionX = lay.previousCursorPosition(_anchorPositionX, Tui::ZTextLayout::SkipCharacters);
        _anchorPositionX = lay.nextCursorPosition(_anchorPositionX, Tui::ZTextLayout::SkipCharacters);
    }
}

int TextCursor::verticalMovementColumn() {
    return _saveCursorPositionX;
}

void TextCursor::setVerticalMovementColumn(int column) {
    _saveCursorPositionX = std::max(0, column);
}

TextCursor::Position TextCursor::selectionStartPos() const {
    return std::min(Position{_anchorPositionX, _anchorPositionY},
                    Position{_cursorPositionX, _cursorPositionY});
}

TextCursor::Position TextCursor::selectionEndPos() const {
    return std::max(Position{_anchorPositionX, _anchorPositionY},
                    Position{_cursorPositionX, _cursorPositionY});
}

void TextCursor::selectAll() {
    moveToStartOfDocument(false);
    moveToEndOfDocument(true);
}

bool TextCursor::hasSelection() const {
    return _cursorPositionX != _anchorPositionX
            || _cursorPositionY != _anchorPositionY;
}

bool TextCursor::atStart() const {
    return _cursorPositionY == 0 && _cursorPositionX == 0;
}

bool TextCursor::atEnd() const {
    return _cursorPositionY == _doc->_lines.size() - 1
            && _cursorPositionX == _doc->_lines[_cursorPositionY].size();
}

bool TextCursor::atLineStart() const {
    return _cursorPositionX == 0;
}

bool TextCursor::atLineEnd() const {
    return _cursorPositionX == _doc->_lines[_cursorPositionY].size();
}

void TextCursor::updateVerticalMovementColumn(const Tui::ZTextLayout &layoutForCursorLine) {
    Tui::ZTextLineRef tlr = layoutForCursorLine.lineForTextPosition(_cursorPositionX);
    _saveCursorPositionX = tlr.cursorToX(_cursorPositionX, Tui::ZTextLayout::Leading);
}

void TextCursor::tmp_ensureInRange() {
    // FIXME: Remove when everything uses TextCursor and TextCursor can ensure it does not point outside of the line.
    if (_doc->_lines[_cursorPositionY].size() < _cursorPositionX) {
        _cursorPositionX = _doc->_lines[_cursorPositionY].size();
    }
}
