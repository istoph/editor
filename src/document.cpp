// SPDX-License-Identifier: BSL-1.0

#include "document.h"

#include "file.h"


bool Document::isModified() const {
    return _currentUndoStep != _savedUndoStep;
}

void Document::undo(File *file) {
    if(_undoSteps.isEmpty()) {
        return;
    }

    if (_currentUndoStep == 0) {
        return;
    }

    --_currentUndoStep;

    _text = _undoSteps[_currentUndoStep].text;
    file->_cursorPositionX = _undoSteps[_currentUndoStep].cursorPositionX;
    file->_cursorPositionY = _undoSteps[_currentUndoStep].cursorPositionY;
    file->modifiedChanged(isModified());
    file->checkUndo();
    file->safeCursorPosition();
    file->adjustScrollPosition();
}

void Document::redo(File *file) {
    if(_undoSteps.isEmpty()) {
        return;
    }

    if (_currentUndoStep + 1 >= _undoSteps.size()) {
        return;
    }

    ++_currentUndoStep;

    _text = _undoSteps[_currentUndoStep].text;
    file->_cursorPositionX = _undoSteps[_currentUndoStep].cursorPositionX;
    file->_cursorPositionY = _undoSteps[_currentUndoStep].cursorPositionY;
    file->modifiedChanged(isModified());
    file->checkUndo();
    file->safeCursorPosition();
    file->adjustScrollPosition();
}

void Document::setGroupUndo(File *file, bool onoff) {
    if(onoff) {
        _groupUndo++;
    } else if(_groupUndo <= 1) {
        _groupUndo = 0;
        saveUndoStep(file);
    } else {
        _groupUndo--;
    }
}

int Document::getGroupUndo() {
    return _groupUndo;
}

void Document::initalUndoStep(File *file) {
    _undoSteps.clear();
    _collapseUndoStep = false;
    _groupUndo = 0;
    _currentUndoStep = -1;
    _savedUndoStep = -1;
    saveUndoStep(file);
    _savedUndoStep = _currentUndoStep;
}

void Document::saveUndoStep(File *file, bool collapsable) {
    if(getGroupUndo() == 0) {
        if (_currentUndoStep + 1 != _undoSteps.size()) {
            _undoSteps.resize(_currentUndoStep + 1);
        } else if (_collapseUndoStep && collapsable && _currentUndoStep != _savedUndoStep) {
            _undoSteps.removeLast();
        }
        _undoSteps.append({ _text, file->_cursorPositionX, file->_cursorPositionY});
        _currentUndoStep = _undoSteps.size() - 1;
        _collapseUndoStep = collapsable;
        file->modifiedChanged(isModified());
        file->checkUndo();
    }
}

void Document::removeFromLine(int line, int codeUnitStart, int codeUnits) {
    _text[line].remove(codeUnitStart, codeUnits);
}

void Document::insertIntoLine(int line, int codeUnitStart, const QString &data) {
    _text[line].insert(codeUnitStart, data);
}

void Document::appendToLine(int line, const QString &data) {
    _text[line].append(data);
}

void Document::removeLines(int start, int count) {
    _text.remove(start, count);
}

void Document::insertLine(int before, const QString &data) {
    _text.insert(before, data);
}

void Document::splitLine(TextCursor::Position pos) {
    _text.insert(pos.y + 1, _text[pos.y].mid(pos.x));
    _text[pos.y].resize(pos.x);
}

bool operator<(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.y, a.x) < std::tie(b.y, b.x);
}

bool operator>(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.y, a.x) > std::tie(b.y, b.x);
}

bool operator==(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.x, a.y) == std::tie(b.x, b.y);
}


TextCursor::TextCursor(Document *doc, File *file, std::function<Tui::ZTextLayout(int line, bool wrappingAllowed)> createTextLayout)
    : _doc(doc), _file(file), _createTextLayout(createTextLayout) {
}

void TextCursor::insertText(const QString &text) {
    auto lines = text.split("\n");

    removeSelectedText();
    if (_doc->_nonewline && atEnd() && lines.size() > 1 && lines.last().size() == 0) {
        lines.removeLast();
        _doc->_nonewline = false;
    }
    _doc->insertIntoLine(_file->_cursorPositionY, _file->_cursorPositionX, lines.front());
    _file->_cursorPositionX += lines.front().size();
    for (int i = 1; i < lines.size(); i++) {
        _doc->splitLine({_file->_cursorPositionX, _file->_cursorPositionY});
        _file->_cursorPositionY++;
        _file->_cursorPositionX = 0;
        _doc->insertIntoLine(_file->_cursorPositionY, _file->_cursorPositionX, lines.at(i));
        _file->_cursorPositionX = lines.at(i).size();
    }

    Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
    updateVerticalMovementColumn(lay);
    _doc->saveUndoStep(_file);
}

void TextCursor::removeSelectedText() {
    if (!hasSelection()) {
        return;
    }

    const Position start = selectionStartPos();
    const Position end = selectionEndPos();

    if (start.y == end.y) {
        // selection only on one line
        _doc->removeFromLine(start.y, start.x, end.x - start.x);
    } else {
        _doc->removeFromLine(start.y, start.x, _doc->_text[start.y].size() - start.x);
        const auto orignalTextLines = _doc->_text.size();
        if (start.y + 1 < end.y) {
            _doc->removeLines(start.y + 1, end.y - start.y - 1);
        }
        if (end.y == orignalTextLines) {
            // selected until the end of buffer, no last selection line to edit
        } else {
            _doc->appendToLine(start.y, _doc->_text[start.y + 1].mid(end.x));
            if (start.y + 1 < _doc->_text.size()) {
                _doc->removeLines(start.y + 1, 1);
            } else {
                _doc->removeFromLine(start.y + 1, 0, _doc->_text[start.y + 1].size());
            }
        }
    }
    clearSelection();
    setPosition(start);

    _doc->saveUndoStep(_file);
}

void TextCursor::clearSelection() {
    _file->_startSelectX = _file->_startSelectY = -1;
    _file->_endSelectX = _file->_endSelectY = -1;
}

QString TextCursor::selectedText() const {
    if (!hasSelection()) {
        return "";
    }

    const Position start = selectionStartPos();
    const Position end = selectionEndPos();

    if (start.y == end.y) {
        // selection only on one line
        return _doc->_text[start.y].mid(start.x, end.x - start.x);
    } else {
        QString res = _doc->_text[start.y].mid(start.x);
        for (int line = start.y + 1; line < end.y; line++) {
            res += "\n";
            res += _doc->_text[line];
        }
        res += "\n";
        res += _doc->_text[end.y].mid(0, end.x);
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

void TextCursor::moveCharacterLeft(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit) {
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        setPosition({lay.previousCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipCharacters), currentLine},
                    extendSelection);
    } else if (currentLine > 0) {
        setPosition({_doc->_text[currentLine - 1].size(), currentLine - 1}, extendSelection);
    }
}

void TextCursor::moveCharacterRight(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit < _doc->_text[currentLine].size()) {
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        setPosition({lay.nextCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipCharacters), currentLine},
                    extendSelection);
    } else if (currentLine + 1 < _doc->_text.size()) {
        setPosition({0, currentLine + 1}, extendSelection);
    }
}

void TextCursor::moveWordLeft(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit) {
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        setPosition({lay.previousCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipWords), currentLine},
                    extendSelection);
    } else if (currentLine > 0) {
        setPosition({_doc->_text[currentLine - 1].size(), currentLine - 1}, extendSelection);
    }
}

void TextCursor::moveWordRight(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit < _doc->_text[currentLine].size()) {
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        setPosition({lay.nextCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipWords), currentLine},
                    extendSelection);
    } else if (currentLine + 1 < _doc->_text.size()) {
        setPosition({0, currentLine + 1}, extendSelection);
    }
}

void TextCursor::moveUp(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentLine > 0) {
        Tui::ZTextLayout lay = _createTextLayout(currentLine - 1, false);
        Tui::ZTextLineRef la = lay.lineAt(0);
        setPositionPreservingVerticalMovementColumn({la.xToCursor(_file->_saveCursorPositionX), currentLine - 1}, extendSelection);
    }
}

void TextCursor::moveDown(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentLine < _doc->_text.size() - 1) {
        Tui::ZTextLayout lay = _createTextLayout(currentLine + 1, false);
        Tui::ZTextLineRef la = lay.lineAt(0);
        setPositionPreservingVerticalMovementColumn({la.xToCursor(_file->_saveCursorPositionX), currentLine + 1}, extendSelection);
    }
}

void TextCursor::moveToStartOfLine(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    setPosition({0, currentLine}, extendSelection);
}

void TextCursor::moveToStartIndentedText(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();

    int i = 0;
    for (; i < _doc->_text[currentLine].size(); i++) {
        if (_doc->_text[currentLine][i] != ' ' && _doc->_text[currentLine][i] != '\t') {
            break;
        }
    }

    setPosition({i, currentLine}, extendSelection);
}

void TextCursor::moveToEndOfLine(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    setPosition({_doc->_text[currentLine].size(), currentLine}, extendSelection);
}

void TextCursor::moveToStartOfDocument(bool extendSelection) {
    setPosition({0, 0}, extendSelection);
}

void TextCursor::moveToEndOfDocument(bool extendSelection) {
    setPosition({_doc->_text.last().size(), _doc->_text.size() - 1}, extendSelection);
}

TextCursor::Position TextCursor::position() {
    return Position{_file->_cursorPositionX, _file->_cursorPositionY};
}

void TextCursor::setPosition(TextCursor::Position pos, bool extendSelection) {
    setPositionPreservingVerticalMovementColumn(pos, extendSelection);
    if (!_file || _file->terminal()) { // TODO rethink this hack to allow using setPosition before terminal is connected
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        updateVerticalMovementColumn(lay);
    }
}

void TextCursor::setPositionPreservingVerticalMovementColumn(TextCursor::Position pos, bool extendSelection) {
    const bool hadSel = hasSelection();
    if (extendSelection) {
        if (!hadSel) {
            _file->_startSelectX = _file->_cursorPositionX;
            _file->_startSelectY = _file->_cursorPositionY;
        }
    } else {
        clearSelection();
    }

    _file->_cursorPositionY = std::max(std::min(pos.y, _doc->_text.size() - 1), 0);
    _file->_cursorPositionX = std::max(std::min(pos.x, _doc->_text[_file->_cursorPositionY].size()), 0);

    // We are not allowed to jump between characters. Therefore, we go once to the left and again to the right.
    if (_file->_cursorPositionX > 0) {
        if (!_file || _file->terminal()) { // TODO rethink this hack to allow using setPosition before terminal is connected
            Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
            _file->_cursorPositionX = lay.previousCursorPosition(_file->_cursorPositionX, Tui::ZTextLayout::SkipCharacters);
            _file->_cursorPositionX = lay.nextCursorPosition(_file->_cursorPositionX, Tui::ZTextLayout::SkipCharacters);
        }
    }

    if (extendSelection) {
        _file->_endSelectX = _file->_cursorPositionX;
        _file->_endSelectY = _file->_cursorPositionY;
    }
}

TextCursor::Position TextCursor::anchor() {
    if (!hasSelection()) {
        // FIXME: This should not be needed anymore when hasSelection() := anchor() == position()
        return position();
    }
    return Position{_file->_startSelectX, _file->_startSelectY};
}

void TextCursor::setAnchorPosition(TextCursor::Position pos) {
    clearSelection();

    _file->_startSelectY = std::max(std::min(pos.y, _doc->_text.size() - 1), 0);
    _file->_startSelectX = std::max(std::min(pos.x, _doc->_text[_file->_startSelectY].size()), 0);

    // We are not allowed to jump between characters. Therefore, we go once to the left and again to the right.
    if (_file->_startSelectX > 0) {
        Tui::ZTextLayout lay = _createTextLayout(_file->_startSelectY, false);
        _file->_startSelectX = lay.previousCursorPosition(_file->_startSelectX, Tui::ZTextLayout::SkipCharacters);
        _file->_startSelectX = lay.nextCursorPosition(_file->_startSelectX, Tui::ZTextLayout::SkipCharacters);
    }

    if (_file->_startSelectY != _file->_cursorPositionY || _file->_startSelectX != _file->_cursorPositionX) {
        _file->_endSelectX = _file->_cursorPositionX;
        _file->_endSelectY = _file->_cursorPositionY;
    } else {
        clearSelection();
    }
}

int TextCursor::verticalMovementColumn() {
    return _file->_saveCursorPositionX;
}

void TextCursor::setVerticalMovementColumn(int column) {
    _file->_saveCursorPositionX = std::max(0, column);
}

TextCursor::Position TextCursor::selectionStartPos() const {
    if (hasSelection()) {
        return std::min(Position{_file->_startSelectX, _file->_startSelectY},
                        Position{_file->_endSelectX, _file->_endSelectY});
    } else {
        return Position{_file->_cursorPositionX, _file->_cursorPositionY};
    }
}

TextCursor::Position TextCursor::selectionEndPos() const {
    if (hasSelection()) {
        return std::max(Position{_file->_startSelectX, _file->_startSelectY},
                        Position{_file->_endSelectX, _file->_endSelectY});
    } else {
        return Position{_file->_cursorPositionX, _file->_cursorPositionY};
    }
}

void TextCursor::selectAll() {
    moveToStartOfDocument(false);
    moveToEndOfDocument(true);
}

bool TextCursor::hasSelection() const {
    return _file->_startSelectX != -1;
}

bool TextCursor::atStart() const {
    return _file->_cursorPositionY == 0 && _file->_cursorPositionX == 0;
}

bool TextCursor::atEnd() const {
    return _file->_cursorPositionY == _doc->_text.size() - 1
            && _file->_cursorPositionX == _doc->_text[_file->_cursorPositionY].size();
}

bool TextCursor::atLineStart() const {
    return _file->_cursorPositionX == 0;
}

bool TextCursor::atLineEnd() const {
    return _file->_cursorPositionX == _doc->_text[_file->_cursorPositionY].size();
}

void TextCursor::updateVerticalMovementColumn(const Tui::ZTextLayout &layoutForCursorLine) {
    Tui::ZTextLineRef tlr = layoutForCursorLine.lineForTextPosition(_file->_cursorPositionX);
    _file->_saveCursorPositionX = tlr.cursorToX(_file->_cursorPositionX, Tui::ZTextLayout::Leading);
}

void TextCursor::tmp_ensureInRange() {
    // FIXME: Remove when everything uses TextCursor and TextCursor can ensure it does not point outside of the line.
    if (_doc->_text[_file->_cursorPositionY].size() < _file->_cursorPositionX) {
        _file->_cursorPositionX = _doc->_text[_file->_cursorPositionY].size();
    }
}
