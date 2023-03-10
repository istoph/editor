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

    if (hasMultiInsert()) {

    } else if (hasBlockSelection()) {

    } else {
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
        }
    }
    Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
    updateVerticalMovementColumn(lay);
    _file->adjustScrollPosition();
    _doc->saveUndoStep(_file);
}

void TextCursor::removeSelectedText() {
    if (!hasAnySelection()) {
        return;
    }

    if (hasBlockSelection()) {
        const int codeUnitStart = selectionBlockStartPos().x;
        const int codeUnits = selectionBlockEndPos().x - selectionBlockStartPos().x;
        const int cursorLine = _file->_cursorPositionY;
        for(int line: getBlockSelectedLines()) {
            _doc->removeFromLine(line, codeUnitStart, codeUnits);
        }
        _file->setCursorPosition({codeUnitStart, cursorLine});
        _file->_endSelectX = codeUnitStart;
        _file->_startSelectX = codeUnitStart;
    } else if (hasSelection()) {
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
        _file->setCursorPosition({start.x, start.y});

        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        updateVerticalMovementColumn(lay);
        clearSelection();
    }

    _doc->saveUndoStep(_file);
}

void TextCursor::clearSelection() {
    _file->_startSelectX = _file->_startSelectY = -1;
    _file->_endSelectX = _file->_endSelectY = -1;
    _file->_blockSelect = false;
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
    auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit) {
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        setPosition({lay.previousCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipCharacters), currentLine},
                    extendSelection);
        updateVerticalMovementColumn(lay);
    } else if (currentLine > 0) {
        setPosition({_doc->_text[currentLine - 1].size(), currentLine - 1}, extendSelection);
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        updateVerticalMovementColumn(lay);
    }
}

void TextCursor::moveCharacterRight(bool extendSelection) {
    auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit < _doc->_text[currentLine].size()) {
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        setPosition({lay.nextCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipCharacters), currentLine},
                    extendSelection);
        updateVerticalMovementColumn(lay);
    } else if (currentLine + 1 < _doc->_text.size()) {
        setPosition({0, currentLine + 1}, extendSelection);
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        updateVerticalMovementColumn(lay);
    }
}

void TextCursor::moveWordLeft(bool extendSelection) {
    auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit) {
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        setPosition({lay.previousCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipWords), currentLine},
                    extendSelection);
        updateVerticalMovementColumn(lay);
    } else if (currentLine > 0) {
        setPosition({_doc->_text[currentLine - 1].size(), currentLine - 1}, extendSelection);
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        updateVerticalMovementColumn(lay);
    }
}

void TextCursor::moveWordRight(bool extendSelection) {
    auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit < _doc->_text[currentLine].size()) {
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        setPosition({lay.nextCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipWords), currentLine},
                    extendSelection);
        updateVerticalMovementColumn(lay);
    } else if (currentLine + 1 < _doc->_text.size()) {
        setPosition({0, currentLine + 1}, extendSelection);
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        updateVerticalMovementColumn(lay);
    }
}

void TextCursor::moveToStartOfLine(bool extendSelection) {
    auto [currentCodeUnit, currentLine] = position();
    setPosition({0, currentLine}, extendSelection);
    Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
    updateVerticalMovementColumn(lay);
}

void TextCursor::moveToStartIndentedText(bool extendSelection) {
    auto [currentCodeUnit, currentLine] = position();

    int i = 0;
    for (; i < _doc->_text[currentLine].size(); i++) {
        if (_doc->_text[currentLine][i] != ' ' && _doc->_text[currentLine][i] != '\t') {
            break;
        }
    }

    setPosition({i, currentLine}, extendSelection);
    Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
    updateVerticalMovementColumn(lay);
}

void TextCursor::moveToEndOfLine(bool extendSelection) {
    auto [currentCodeUnit, currentLine] = position();
    setPosition({_doc->_text[currentLine].size(), currentLine}, extendSelection);
    Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
    updateVerticalMovementColumn(lay);
}

void TextCursor::moveToStartOfDocument(bool extendSelection) {
    setPosition({0, 0}, extendSelection);
    Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
    updateVerticalMovementColumn(lay);
}

void TextCursor::moveToEndOfDocument(bool extendSelection) {
    setPosition({_doc->_text.last().size(), _doc->_text.size() - 1}, extendSelection);
    Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
    updateVerticalMovementColumn(lay);
}

TextCursor::Position TextCursor::position() {
    return Position{_file->_cursorPositionX, _file->_cursorPositionY};
}

void TextCursor::setPosition(TextCursor::Position pos, bool extendSelection) {
    const bool hadSel = hasSelection();
    if (extendSelection) {
        if (!hadSel) {
            // remove multi-insert and block selection
            clearSelection();
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
        Tui::ZTextLayout lay = _createTextLayout(_file->_cursorPositionY, false);
        _file->_cursorPositionX = lay.previousCursorPosition(_file->_cursorPositionX, Tui::ZTextLayout::SkipCharacters);
        _file->_cursorPositionX = lay.nextCursorPosition(_file->_cursorPositionX, Tui::ZTextLayout::SkipCharacters);
    }

    if (extendSelection) {
        _file->_endSelectX = _file->_cursorPositionX;
        _file->_endSelectY = _file->_cursorPositionY;
    }
}

TextCursor::Position TextCursor::anchor() {
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

TextCursor::Position TextCursor::selectionBlockStartPos() const {
    if (hasBlockSelection()) {
        return std::min(Position{_file->_startSelectX, _file->_startSelectY},
                        Position{_file->_endSelectX, _file->_endSelectY});
    } else {
        return Position{_file->_cursorPositionX, _file->_cursorPositionY};
    }
}

TextCursor::Position TextCursor::selectionBlockEndPos() const {
    if (hasBlockSelection()) {
        return std::max(Position{_file->_startSelectX, _file->_startSelectY},
                        Position{_file->_endSelectX, _file->_endSelectY});
    } else {
        return Position{_file->_cursorPositionX, _file->_cursorPositionY};
    }
}

bool TextCursor::hasSelection() const {
    return _file->_startSelectX != -1 && !hasBlockSelection() && !hasMultiInsert();
}

bool TextCursor::hasBlockSelection() const {
    return _file->_blockSelect && _file->_startSelectX != _file->_endSelectX;
}

bool TextCursor::hasAnySelection() const {
    return hasSelection() || hasBlockSelection();
}

bool TextCursor::hasMultiInsert() const {
    return _file->_blockSelect && _file->_startSelectX == _file->_endSelectX;
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

Range TextCursor::getBlockSelectedLines() {
    return Range {selectionStartPos().y, selectionBlockEndPos().y + 1};
}

TextCursor::Position TextCursor::insertLineBreakAt(Position cursor) {
    if(_doc->_nonewline && cursor.y == _doc->_text.size() -1 && _doc->_text[cursor.y].size() == cursor.x) {
        _doc->_nonewline = false;
    } else {
        _doc->splitLine(cursor);
        cursor.x = 0;
        cursor.y += 1;
    }
    return cursor;
}

void TextCursor::updateVerticalMovementColumn(const Tui::ZTextLayout &layoutForCursorLine) {
    Tui::ZTextLineRef tlr = layoutForCursorLine.lineForTextPosition(_file->_cursorPositionX);
    _file->_saveCursorPositionX = tlr.cursorToX(_file->_cursorPositionX, Tui::ZTextLayout::Leading);
}
