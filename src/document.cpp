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
    return std::tie(a.x, a.y) < std::tie(b.x, b.y);
}


TextCursor::TextCursor(Document *doc, File *file) : _doc(doc), _file(file) {
}

void TextCursor::insertText(const QString &text) {
    const auto lines = text.split("\n");

    if (hasMultiInsert()) {

    } else if (hasBlockSelection()) {

    } else {
        removeSelectedText();
        _doc->insertIntoLine(_file->_cursorPositionY, _file->_cursorPositionX, lines.front());
        _file->_cursorPositionX += lines.front().size();
        for (int i = 1; i < lines.size(); i++) {
            _doc->splitLine({_file->_cursorPositionX, _file->_cursorPositionY});
            _file->_cursorPositionY++;
            _file->_cursorPositionX = 0;
            _doc->insertIntoLine(_file->_cursorPositionY, _file->_cursorPositionX, lines.at(i));
        }
    }
    _file->safeCursorPosition();
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

        _file->safeCursorPosition();
        clearSelection();
    }

    _doc->saveUndoStep(_file);
}

void TextCursor::clearSelection() {
    _file->_startSelectX = _file->_startSelectY = -1;
    _file->_endSelectX = _file->_endSelectY = -1;
    _file->_blockSelect = false;
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
