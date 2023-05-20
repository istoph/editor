// SPDX-License-Identifier: BSL-1.0

#include "document.h"

#include <QTimer>

#include <Tui/Misc/SurrogateEscape.h>

#include "file.h"


Document::Document(QObject *parent) : QObject (parent) {
    _lines.append(QString());
    initalUndoStep(nullptr);
}

void Document::reset() {
    _lines.clear();
    _lines.append(QString());

    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        m->setLine(0);
    }
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        c->setPosition({0, 0});
    }

    initalUndoStep(nullptr);

    debugConsistencyCheck(nullptr);
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

    // Clear line markers and cursors while _lines still has contents.
    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        m->setLine(0);
    }
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        c->setPosition({0, 0});
    }

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

    debugConsistencyCheck(nullptr);

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

DocumentSnapshot Document::snapshot() const {
    DocumentSnapshot ret;
    ret.pimpl->_lines = _lines;
    return ret;
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

    // We need to capture how lines got reordered to also adjust the cursor and line markers in the same pattern.
    // Basically this is std::stable_sort(_lines.begin() + first, _lines.begin() + last) but also capturing the reordering

    std::vector<int> reorderBuffer;
    reorderBuffer.resize(last - first);
    for (int i = 0; i < last - first; i++) {
        reorderBuffer[i] = first + i;
    }

    std::stable_sort(reorderBuffer.begin(), reorderBuffer.end(), [&](int lhs, int rhs) {
        return _lines[lhs] < _lines[rhs];
    });

    std::vector<QString> tmp;
    tmp.resize(last - first);
    for (int i = 0; i < last - first; i++) {
        tmp[i] = _lines[first + i];
    }

    // Apply reorderBuffer to _lines
    for (int i = 0; i < last - first; i++) {
        _lines[first + i] = tmp[reorderBuffer[i] - first];
    }

    std::vector<int> reorderBufferInverted;
    reorderBufferInverted.resize(last - first);
    for (int i = 0; i < last - first; i++) {
        reorderBufferInverted[reorderBuffer[i] - first] = first + i;
    }

    // And apply reorderBuffer to cursors
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        const auto [cursorCodeUnit, cursorLine] = c->position();

        bool positionMustBeSet = false;

        if (first <= anchorLine && anchorLine < last) {
            c->setAnchorPosition({anchorCodeUnit, reorderBufferInverted[anchorLine - first]});
            positionMustBeSet = true;
        }

        if (first <= cursorLine && cursorLine < last) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, reorderBufferInverted[cursorLine - first]}, true);
            positionMustBeSet = false;
        }

        if (positionMustBeSet) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine}, true);
        }
    }
    // Also line markers
    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        if (first <= m->line() && m->line() < last) {
            m->setLine(reorderBufferInverted[m->line() - first]);
        }
    }

    debugConsistencyCheck(nullptr);

    saveUndoStep(cursorForUndoStep);
}

void Document::tmp_moveLine(int from, int to, TextCursor *cursorForUndoStep) {
    _lines.insert(to, _lines[from]);
    if (from < to) {
        _lines.remove(from);
    } else {
        _lines.remove(from + 1);
    }

    auto transform = [&](int line, int data, auto fn) {
        if (from < to) {
            // from         -> mid
            // mid          -> from
            // to           -> to
            if (line > from && line < to) {
                fn(line - 1, data);
            } else if (line == from) {
                fn(to - 1, data);
            }
        } else {
            // to           -> from
            // mid          -> to
            // from         -> mid
            if (line >= to && line < from) {
                fn(line + 1, data);
            } else if (line == from) {
                fn(to, data);
            }
        }
    };

    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        const auto [cursorCodeUnit, cursorLine] = c->position();

        bool positionMustBeSet = false;

        // anchor
        transform(anchorLine, anchorCodeUnit, [&](int line, int codeUnit) {
            c->setAnchorPosition({codeUnit, line});
            positionMustBeSet = true;
        });

        // position
        transform(cursorLine, cursorCodeUnit, [&](int line, int codeUnit) {
            c->setPositionPreservingVerticalMovementColumn({codeUnit, line}, true);
            positionMustBeSet = false;
        });

        if (positionMustBeSet) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine}, true);
        }
    }
    // similar for line markers
    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        transform(m->line(), 0, [&](int line, int) {
            m->setLine(line);
        });
    }

    debugConsistencyCheck(nullptr);

    saveUndoStep(cursorForUndoStep);
}

void Document::debugConsistencyCheck(const TextCursor *exclude) {
    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        if (m->line() < 0) {
            qFatal("Document::debugConsistencyCheck: A line marker has a negative position");
            abort();
        } else if (m->line() >= _lines.size()) {
            qFatal("Document::debugConsistencyCheck: A line marker is beyond the maximum line");
            abort();
        }
    }
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        if (c == exclude) continue;
        c->debugConsistencyCheck();
    }
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

    // ensure all cursors have valid positions.
    // Cursor positions after undo are still wonky as the undo state does not have enough information to properly
    // adjust the cursors, but at least all cursors should be on valid positions after this
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        const auto [cursorCodeUnit, cursorLine] = c->position();

        c->setAnchorPosition({anchorCodeUnit, anchorLine});
        c->setPosition({cursorCodeUnit, cursorLine}, true);
    }
    // similar for line markers
    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        if (m->line() >= _lines.size()) {
            m->setLine(_lines.size() - 1);
        }
    }

    debugConsistencyCheck(nullptr);

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

    // ensure all cursors have valid positions.
    // Cursor positions after undo are still wonky as the undo state does not have enough information to properly
    // adjust the cursors, but at least all cursors should be on valid positions after this
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        const auto [cursorCodeUnit, cursorLine] = c->position();

        c->setAnchorPosition({anchorCodeUnit, anchorLine});
        c->setPosition({cursorCodeUnit, cursorLine}, true);
    }
    // similar for line markers
    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        if (m->line() >= _lines.size()) {
            m->setLine(_lines.size() - 1);
        }
    }

    debugConsistencyCheck(nullptr);

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

void Document::registerLineMarker(LineMarker *marker) {
    lineMarkerList.appendOrMoveToLast(marker);
}

void Document::unregisterLineMarker(LineMarker *marker) {
    lineMarkerList.remove(marker);
}

void Document::scheduleChangeSignals() {
    if (_changeScheduled) return;

    _changeScheduled = true;

    QTimer::singleShot(0, this, [this] {
        _changeScheduled = false;

        for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
            if (c->_changed) {
                c->_changed = false;
                cursorChanged(c);
            }
        }
        for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
            if (m->_changed) {
                m->_changed = false;
                lineMarkerChanged(m);
            }
        }
    });
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

void Document::registerTextCursor(TextCursor *cursor) {
    cursorList.appendOrMoveToLast(cursor);
}

void Document::unregisterTextCursor(TextCursor *cursor) {
    cursorList.remove(cursor);
}

void Document::removeFromLine(TextCursor *cursor, int line, int codeUnitStart, int codeUnits) {
    _lines[line].remove(codeUnitStart, codeUnits);

    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        if (cursor == c) continue;

        bool positionMustBeSet = false;

        // anchor
        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        if (anchorLine == line) {
            if (anchorCodeUnit >= codeUnitStart + codeUnits) {
                c->setAnchorPosition({anchorCodeUnit - codeUnits, anchorLine});
                positionMustBeSet = true;
            } else if (anchorCodeUnit >= codeUnitStart) {
                c->setAnchorPosition({codeUnitStart, anchorLine});
                positionMustBeSet = true;
            }
        }

        // position
        const auto [cursorCodeUnit, cursorLine] = c->position();
        if (cursorLine == line) {
            if (cursorCodeUnit >= codeUnitStart + codeUnits) {
                c->setPosition({cursorCodeUnit - codeUnits, cursorLine}, true);
                positionMustBeSet = false;
            } else if (cursorCodeUnit >= codeUnitStart) {
                c->setPosition({codeUnitStart, cursorLine}, true);
                positionMustBeSet = false;
            }
        }

        if (positionMustBeSet) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine}, true);
        }
    }

    debugConsistencyCheck(cursor);
}

void Document::insertIntoLine(TextCursor *cursor, int line, int codeUnitStart, const QString &data) {
    _lines[line].insert(codeUnitStart, data);

    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        if (cursor == c) continue;

        bool positionMustBeSet = false;

        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        const auto [cursorCodeUnit, cursorLine] = c->position();

        if (c->hasSelection()) {
            const bool anchorBefore = anchorLine < cursorLine || (anchorLine == cursorLine && anchorCodeUnit < cursorCodeUnit);

            // anchor
            if (anchorLine == line) {
                const int anchorAdj = anchorBefore ? 0 : -1;
                if (anchorCodeUnit + anchorAdj >= codeUnitStart) {
                    c->setAnchorPosition({anchorCodeUnit + data.size(), anchorLine});
                    positionMustBeSet = true;
                }
            }

            // position
            if (cursorLine == line) {
                const int cursorAdj = anchorBefore ? -1 : 1;
                if (cursorCodeUnit + cursorAdj >= codeUnitStart) {
                    c->setPosition({cursorCodeUnit + data.size(), cursorLine}, true);
                    positionMustBeSet = false;
                }
            }

            if (positionMustBeSet) {
                c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine}, true);
            }
        } else {
            if (cursorLine == line && cursorCodeUnit >= codeUnitStart) {
                c->setPosition({cursorCodeUnit + data.size(), cursorLine}, false);
            }
        }

    }

    debugConsistencyCheck(cursor);
}

void Document::appendToLine(TextCursor *cursor, int line, const QString &data) {
    _lines[line].append(data);

    debugConsistencyCheck(cursor);
}

void Document::removeLines(TextCursor *cursor, int start, int count) {
    _lines.remove(start, count);

    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        if (m->line() > start + count) {
            m->setLine(m->line() - count);
        } else if (m->line() > start) {
            m->setLine(start);
        }
    }
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        if (cursor == c) continue;

        bool positionMustBeSet = false;

        // anchor
        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        if (anchorLine > start + count) {
            c->setAnchorPosition({anchorCodeUnit, anchorLine - count});
            positionMustBeSet = true;
        } else if (anchorLine >= _lines.size()) {
            c->setAnchorPosition({_lines[_lines.size() - 1].size(), _lines.size() - 1});
            positionMustBeSet = true;
        } else if (anchorLine >= start) {
            c->setAnchorPosition({0, start});
            positionMustBeSet = true;
        }

        // position
        const auto [cursorCodeUnit, cursorLine] = c->position();
        if (cursorLine > start + count) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine - count}, true);
            positionMustBeSet = false;
        } else if (cursorLine >= _lines.size()) {
            c->setPositionPreservingVerticalMovementColumn({_lines[_lines.size() - 1].size(), _lines.size() - 1}, true);
            positionMustBeSet = false;
        } else if (cursorLine >= start) {
            c->setPositionPreservingVerticalMovementColumn({0, start}, true);
            positionMustBeSet = false;
        }

        if (positionMustBeSet) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine}, true);
        }
    }

    debugConsistencyCheck(cursor);
}

void Document::insertLine(TextCursor *cursor, int before, const QString &data) {
    _lines.insert(before, data);

    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        if (m->line() >= before) {
            m->setLine(m->line() + 1);
        }
    }
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        if (cursor == c) continue;

        bool positionMustBeSet = false;

        // anchor
        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        if (anchorLine > before) {
            c->setAnchorPosition({anchorCodeUnit, anchorLine + 1});
            positionMustBeSet = true;
        }

        // position
        const auto [cursorCodeUnit, cursorLine] = c->position();
        if (cursorLine > before) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine + 1}, true);
            positionMustBeSet = false;
        }

        if (positionMustBeSet) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine}, true);
        }
    }

    debugConsistencyCheck(cursor);
}

void Document::splitLine(TextCursor *cursor, TextCursor::Position pos) {
    _lines.insert(pos.line + 1, _lines[pos.line].mid(pos.codeUnit));
    _lines[pos.line].resize(pos.codeUnit);

    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        if (m->line() > pos.line || (m->line() == pos.line && pos.codeUnit == 0)) {
            m->setLine(m->line() + 1);
        }
    }
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        if (cursor == c) continue;

        bool positionMustBeSet = false;

        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        const auto [cursorCodeUnit, cursorLine] = c->position();

        // anchor

        if (c->hasSelection()) {
            const bool anchorBefore = anchorLine < cursorLine || (anchorLine == cursorLine && anchorCodeUnit < cursorCodeUnit);

            if (anchorLine > pos.line) {
                c->setAnchorPosition({anchorCodeUnit, anchorLine + 1});
                positionMustBeSet = true;
            } else if (anchorLine == pos.line) {
                const int anchorAdj = anchorBefore ? 0 : -1;
                if (anchorCodeUnit + anchorAdj >= pos.codeUnit) {
                    c->setAnchorPosition({anchorCodeUnit - pos.codeUnit, anchorLine + 1});
                    positionMustBeSet = true;
                }
            }

            // position
            if (cursorLine > pos.line) {
                c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine + 1}, true);
                positionMustBeSet = false;
            } else if (cursorLine == pos.line) {
                const int cursorAdj = anchorBefore ? -1 : 1;
                if (cursorCodeUnit + cursorAdj >= pos.codeUnit) {
                    c->setPosition({cursorCodeUnit - pos.codeUnit, cursorLine + 1}, true);
                    positionMustBeSet = false;
                }
            }

            if (positionMustBeSet) {
                c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine}, true);
            }
        } else {
            if (cursorLine > pos.line) {
                c->setPosition({cursorCodeUnit, cursorLine + 1}, false);
            } else if (cursorLine == pos.line && cursorCodeUnit >= pos.codeUnit) {
                c->setPosition({cursorCodeUnit - pos.codeUnit, cursorLine + 1}, false);
            }
        }
    }

    debugConsistencyCheck(cursor);
}

void Document::mergeLines(TextCursor *cursor, int line) {
    const int originalLineCodeUnits = _lines[line].size();
    _lines[line].append(_lines[line + 1]);
    if (line + 1 < _lines.size()) {
        _lines.remove(line + 1, 1);
    } else {
        _lines[line + 1].clear();
    }

    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        if (m->line() > line) {
            m->setLine(m->line() - 1);
        }
    }
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        if (cursor == c) continue;

        bool positionMustBeSet = false;

        // anchor
        const auto [anchorCodeUnit, anchorLine] = c->anchor();
        if (anchorLine > line + 1) {
            c->setAnchorPosition({anchorCodeUnit, anchorLine - 1});
            positionMustBeSet = true;
        } else if (anchorLine == line + 1) {
            c->setAnchorPosition({originalLineCodeUnits + anchorCodeUnit, line});
            positionMustBeSet = true;
        }

        // position
        const auto [cursorCodeUnit, cursorLine] = c->position();
        if (cursorLine > line + 1) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine - 1}, true);
            positionMustBeSet = false;
        } else if (cursorLine == line + 1) {
            c->setPosition({originalLineCodeUnits + cursorCodeUnit, line}, true);
            positionMustBeSet = false;
        }

        if (positionMustBeSet) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine}, true);
        }
    }

    debugConsistencyCheck(cursor);
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
    _doc->registerTextCursor(this);
}

TextCursor::~TextCursor() {
    _doc->unregisterTextCursor(this);
}

void TextCursor::insertText(const QString &text) {
    auto lines = text.split("\n");

    removeSelectedText();

    if (text.size()) {
        if (_doc->_nonewline && atEnd() && lines.size() > 1 && lines.last().size() == 0) {
            lines.removeLast();
            _doc->_nonewline = false;
        }
        _doc->insertIntoLine(this, _cursorLine, _cursorCodeUnit, lines.front());
        _cursorCodeUnit += lines.front().size();
        for (int i = 1; i < lines.size(); i++) {
            _doc->splitLine(this, {_cursorCodeUnit, _cursorLine});
            _cursorLine++;
            _cursorCodeUnit = 0;
            _doc->insertIntoLine(this, _cursorLine, _cursorCodeUnit, lines.at(i));
            _cursorCodeUnit = lines.at(i).size();
        }
        _anchorCodeUnit = _cursorCodeUnit;
        _anchorLine = _cursorLine;
        scheduleChangeSignal();
    }

    Tui::ZTextLayout lay = _createTextLayout(_cursorLine, true);
    updateVerticalMovementColumn(lay);

    if (text.size()) {
        _doc->saveUndoStep(this);
    }

    _doc->debugConsistencyCheck(nullptr);
}

void TextCursor::removeSelectedText() {
    if (!hasSelection()) {
        return;
    }

    const Position start = selectionStartPos();
    const Position end = selectionEndPos();

    if (start.line == end.line) {
        // selection only on one line
        _doc->removeFromLine(this, start.line, start.codeUnit, end.codeUnit - start.codeUnit);
    } else {
        _doc->removeFromLine(this, start.line, start.codeUnit, _doc->_lines[start.line].size() - start.codeUnit);
        const auto orignalTextLines = _doc->_lines.size();
        if (start.line + 1 < end.line) {
            _doc->removeLines(this, start.line + 1, end.line - start.line - 1);
        }
        if (end.line == orignalTextLines) {
            // selected until the end of buffer, no last selection line to edit
        } else {
            _doc->removeFromLine(this, start.line + 1, 0, end.codeUnit);
            _doc->mergeLines(this, start.line);
        }
    }
    clearSelection();
    setPosition(start);

    _doc->saveUndoStep(this);
    _doc->debugConsistencyCheck(nullptr);
}

void TextCursor::clearSelection() {
    if (_anchorCodeUnit != _cursorCodeUnit || _anchorLine != _cursorLine) {
        _anchorCodeUnit = _cursorCodeUnit;
        _anchorLine = _cursorLine;
        scheduleChangeSignal();
    }
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
        Tui::ZTextLayout lay = _createTextLayout(_cursorLine, false);
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
        Tui::ZTextLayout lay = _createTextLayout(_cursorLine, false);
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
        Tui::ZTextLayout lay = _createTextLayout(_cursorLine, false);
        setPosition({lay.previousCursorPosition(currentCodeUnit, Tui::ZTextLayout::SkipWords), currentLine},
                    extendSelection);
    } else if (currentLine > 0) {
        setPosition({_doc->_lines[currentLine - 1].size(), currentLine - 1}, extendSelection);
    }
}

void TextCursor::moveWordRight(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit < _doc->_lines[currentLine].size()) {
        Tui::ZTextLayout lay = _createTextLayout(_cursorLine, false);
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
        int fineMoveCodeUnit = layStarting.lineAt(lineStarting.lineNumber() - 1).xToCursor(_VerticalMovementColumn);
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
        setPositionPreservingVerticalMovementColumn({la.xToCursor(_VerticalMovementColumn), currentLine - 1}, extendSelection);
    }
}

void TextCursor::moveDown(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();

    Tui::ZTextLayout layStarting = _createTextLayout(currentLine, true);
    Tui::ZTextLineRef lineStarting = layStarting.lineForTextPosition(currentCodeUnit);

    if (lineStarting.lineNumber() + 1 < layStarting.lineCount()) {
        int fineMoveCodeUnit = layStarting.lineAt(lineStarting.lineNumber() + 1).xToCursor(_VerticalMovementColumn);
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
        setPositionPreservingVerticalMovementColumn({la.xToCursor(_VerticalMovementColumn), currentLine + 1}, extendSelection);
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
    return Position{_cursorCodeUnit, _cursorLine};
}

void TextCursor::setPosition(TextCursor::Position pos, bool extendSelection) {
    setPositionPreservingVerticalMovementColumn(pos, extendSelection);
    if (!_widget || _widget->terminal()) { // TODO rethink this hack to allow using setPosition before terminal is connected
        Tui::ZTextLayout lay = _createTextLayout(_cursorLine, true);
        updateVerticalMovementColumn(lay);
    }
}

void TextCursor::setPositionPreservingVerticalMovementColumn(TextCursor::Position pos, bool extendSelection) {
    auto cursorLine = std::max(std::min(pos.line, _doc->_lines.size() - 1), 0);
    auto cursorCodeUnit = std::max(std::min(pos.codeUnit, _doc->_lines[cursorLine].size()), 0);

    // We are not allowed to jump between characters. Therefore, we go once to the left and again to the right.
    if (cursorCodeUnit > 0) {
        if (!_widget || _widget->terminal()) { // TODO rethink this hack to allow using setPosition before terminal is connected
            Tui::ZTextLayout lay = _createTextLayout(cursorLine, false);
            cursorCodeUnit = lay.previousCursorPosition(cursorCodeUnit, Tui::ZTextLayout::SkipCharacters);
            cursorCodeUnit = lay.nextCursorPosition(cursorCodeUnit, Tui::ZTextLayout::SkipCharacters);
        }
    }

    if (!extendSelection
            && (_anchorCodeUnit != cursorCodeUnit || _anchorLine != cursorLine)) {
        _anchorCodeUnit = cursorCodeUnit;
        _anchorLine = cursorLine;
        scheduleChangeSignal();
    }

    if (_cursorLine != cursorLine || _cursorCodeUnit != cursorCodeUnit) {
        _cursorLine = cursorLine;
        _cursorCodeUnit = cursorCodeUnit;
        scheduleChangeSignal();
    }
}

TextCursor::Position TextCursor::anchor() {
    return Position{_anchorCodeUnit, _anchorLine};
}

void TextCursor::setAnchorPosition(TextCursor::Position pos) {
    auto anchorLine = std::max(std::min(pos.line, _doc->_lines.size() - 1), 0);
    auto anchorCodeUnit = std::max(std::min(pos.codeUnit, _doc->_lines[anchorLine].size()), 0);

    // We are not allowed to jump between characters. Therefore, we go once to the left and again to the right.
    if (anchorCodeUnit > 0) {
        Tui::ZTextLayout lay = _createTextLayout(anchorLine, false);
        anchorCodeUnit = lay.previousCursorPosition(anchorCodeUnit, Tui::ZTextLayout::SkipCharacters);
        anchorCodeUnit = lay.nextCursorPosition(anchorCodeUnit, Tui::ZTextLayout::SkipCharacters);
    }

    if (_anchorLine != anchorLine || _anchorCodeUnit != anchorCodeUnit) {
        _anchorLine = anchorLine;
        _anchorCodeUnit = anchorCodeUnit;
        scheduleChangeSignal();
    }
}

int TextCursor::verticalMovementColumn() {
    return _VerticalMovementColumn;
}

void TextCursor::setVerticalMovementColumn(int column) {
    _VerticalMovementColumn = std::max(0, column);
}

TextCursor::Position TextCursor::selectionStartPos() const {
    return std::min(Position{_anchorCodeUnit, _anchorLine},
                    Position{_cursorCodeUnit, _cursorLine});
}

TextCursor::Position TextCursor::selectionEndPos() const {
    return std::max(Position{_anchorCodeUnit, _anchorLine},
                    Position{_cursorCodeUnit, _cursorLine});
}

void TextCursor::selectAll() {
    moveToStartOfDocument(false);
    moveToEndOfDocument(true);
}

bool TextCursor::hasSelection() const {
    return _cursorCodeUnit != _anchorCodeUnit
            || _cursorLine != _anchorLine;
}

bool TextCursor::atStart() const {
    return _cursorLine == 0 && _cursorCodeUnit == 0;
}

bool TextCursor::atEnd() const {
    return _cursorLine == _doc->_lines.size() - 1
            && _cursorCodeUnit == _doc->_lines[_cursorLine].size();
}

bool TextCursor::atLineStart() const {
    return _cursorCodeUnit == 0;
}

bool TextCursor::atLineEnd() const {
    return _cursorCodeUnit == _doc->_lines[_cursorLine].size();
}

void TextCursor::updateVerticalMovementColumn(const Tui::ZTextLayout &layoutForCursorLine) {
    Tui::ZTextLineRef tlr = layoutForCursorLine.lineForTextPosition(_cursorCodeUnit);
    _VerticalMovementColumn = tlr.cursorToX(_cursorCodeUnit, Tui::ZTextLayout::Leading);
}

void TextCursor::scheduleChangeSignal() {
    _changed = true;
    _doc->scheduleChangeSignals();
}

void TextCursor::debugConsistencyCheck() {
    if (_anchorLine >= _doc->_lines.size()) {
        qFatal("TextCursor::debugConsistencyCheck: _anchorLine beyond max");
        abort();
    } else if (_anchorLine < 0) {
        qFatal("TextCursor::debugConsistencyCheck: _anchorLine negative");
        abort();
    }

    if (_anchorCodeUnit < 0) {
        qFatal("TextCursor::debugConsistencyCheck: _anchorCodeUnit negative");
        abort();
    } else if (_anchorCodeUnit > _doc->_lines[_anchorLine].size()) {
        qFatal("TextCursor::debugConsistencyCheck: _anchorCodeUnit beyond max");
        abort();
    }

    if (_cursorLine >= _doc->_lines.size()) {
        qFatal("TextCursor::debugConsistencyCheck: _cursorLine beyond max");
        abort();
    } else if (_cursorLine < 0) {
        qFatal("TextCursor::debugConsistencyCheck: _cursorLine negative");
        abort();
    }

    if (_cursorCodeUnit < 0) {
        qFatal("TextCursor::debugConsistencyCheck: _cursorCodeUnit negative");
        abort();
    } else if (_cursorCodeUnit > _doc->_lines[_cursorLine].size()) {
        qFatal("TextCursor::debugConsistencyCheck: _cursorCodeUnit beyond max");
        abort();
    }
}

LineMarker::LineMarker(Document *doc) : LineMarker(doc, 0) {
}

LineMarker::LineMarker(Document *doc, int line) : _line(line), _doc(doc) {
    _doc->registerLineMarker(this);
}

LineMarker::~LineMarker() {
    _doc->unregisterLineMarker(this);
}

int LineMarker::line() {
    return _line;
}

void LineMarker::setLine(int line) {
    line = std::max(std::min(line, _doc->lineCount() - 1), 0);

    if (_line != line) {
        _line = line;
        _changed = true;
        _doc->scheduleChangeSignals();
    }
}


DocumentSnapshot::DocumentSnapshot() : pimpl(std::make_shared<DocumentSnapshotPrivate>()) {
}

DocumentSnapshot::DocumentSnapshot(const DocumentSnapshot &other) : pimpl(other.pimpl) {
}

DocumentSnapshot::~DocumentSnapshot() {
}

DocumentSnapshot &DocumentSnapshot::operator=(const DocumentSnapshot &other) {
    pimpl = other.pimpl;
    return *this;
}

int DocumentSnapshot::lineCount() const {
    return pimpl->_lines.size();
}

QString DocumentSnapshot::line(int line) const {
    return pimpl->_lines[line];
}

int DocumentSnapshot::lineCodeUnits(int line) const {
    return pimpl->_lines[line].size();
}
