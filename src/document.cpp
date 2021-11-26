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

bool operator<(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.y, a.x) < std::tie(b.y, b.x);
}

bool operator>(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.y, a.x) > std::tie(b.y, b.x);
}

bool operator==(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.x, a.y) < std::tie(b.x, b.y);
}
