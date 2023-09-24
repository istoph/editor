// SPDX-License-Identifier: BSL-1.0

#include "document.h"

#include <QTimer>
#include <QThreadPool>

#include <Tui/Misc/SurrogateEscape.h>

#include "file.h"


Document::Document(QObject *parent) : QObject (parent) {
    _lines.append(LineData());
    initalUndoStep(0, 0);
}

Document::~Document() {
    // Invalidate all snapshots out there, if snapshots are used in threads that should signal them to stop their work
    _revision->store(_revision->load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
}

void Document::reset() {
    _lines.clear();
    _lines.append(LineData());

    for (LineMarker *m = lineMarkerList.first; m; m = m->markersList.next) {
        m->setLine(0);
    }
    for (TextCursor *c = cursorList.first; c; c = c->markersList.next) {
        c->setPosition({0, 0});
    }

    initalUndoStep(0, 0);

    debugConsistencyCheck(nullptr);
    noteContentsChange();
}

void Document::writeTo(QIODevice *file, bool crLfMode) {
    for (int i = 0; i < lineCount(); i++) {
        file->write(Tui::Misc::SurrogateEscape::encode(_lines[i].chars));
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
    return readFrom(file, {0, 0}, nullptr);
}

bool Document::readFrom(QIODevice *file, TextCursor::Position initialPosition, TextCursor *initialPositionCursor) {
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
        _lines.append({text});
    }

    if (_lines.isEmpty()) {
        _lines.append({""});
        _nonewline = true;
    }

    bool allLinesCrLf = false;

    for (int l = 0; l < lineCount(); l++) {
        if (_lines[l].chars.size() >= 1 && _lines[l].chars.at(_lines[l].chars.size() - 1) == QChar('\r')) {
            allLinesCrLf = true;
        } else {
            allLinesCrLf = false;
            break;
        }
    }
    if (allLinesCrLf) {
        for (int i = 0; i < lineCount(); i++) {
            _lines[i].chars.remove(_lines[i].chars.size() - 1, 1);
        }
    }

    if (initialPosition.line >= _lines.size()) {
        initialPosition.line = _lines.size() - 1;
    }
    if (initialPosition.codeUnit > _lines[initialPosition.line].chars.size()) {
        initialPosition.codeUnit = _lines[initialPosition.line].chars.size();
    }

    if (initialPositionCursor) {
        initialPositionCursor->setPosition(initialPosition);
        initialPosition = initialPositionCursor->position();
    }
    initalUndoStep(initialPosition.codeUnit, initialPosition.line);

    debugConsistencyCheck(nullptr);

    noteContentsChange();

    return allLinesCrLf;
}

int Document::lineCount() const {
    return _lines.size();
}

QString Document::line(int line) const {
    return _lines[line].chars;
}

int Document::lineCodeUnits(int line) const {
    return _lines[line].chars.size();
}

unsigned Document::lineRevision(int line) const {
    return _lines[line].revision;
}

void Document::setLineUserData(int line, std::shared_ptr<UserData> userData) {
    _lines[line].userData = userData;
}

std::shared_ptr<UserData> Document::lineUserData(int line) {
    return _lines[line].userData;
}

DocumentSnapshot Document::snapshot() const {
    DocumentSnapshot ret;
    ret.pimpl->_lines = _lines;
    ret.pimpl->_revision = *_revision;
    ret.pimpl->_revisionShared = _revision;
    return ret;
}

unsigned Document::revision() const {
    return *_revision;
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

void Document::sortLines(int first, int last, TextCursor *cursorForUndoStep) {

    // We need to capture how lines got reordered to also adjust the cursor and line markers in the same pattern.
    // Basically this is std::stable_sort(_lines.begin() + first, _lines.begin() + last) but also capturing the reordering

    std::vector<int> reorderBuffer;
    reorderBuffer.resize(last - first);
    for (int i = 0; i < last - first; i++) {
        reorderBuffer[i] = first + i;
    }

    std::stable_sort(reorderBuffer.begin(), reorderBuffer.end(), [&](int lhs, int rhs) {
        return _lines[lhs].chars < _lines[rhs].chars;
    });

    std::vector<LineData> tmp;
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

    noteContentsChange();

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

    noteContentsChange();

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

    noteContentsChange();

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

    noteContentsChange();

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

        if (_contentsChangedSignalToBeEmitted) {
            _contentsChangedSignalToBeEmitted = false;
            contentsChanged();
        }

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

void Document::noteContentsChange() {
    _contentsChangedSignalToBeEmitted = true;
    _revision->store(_revision->load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    scheduleChangeSignals();
}

Document::UndoGroup::UndoGroup(Document::UndoGroup &&other) {
    _doc = other._doc;
    _cursor = other._cursor;
    _closed = other._closed;

    other._doc = nullptr;
    other._cursor = nullptr;
}

Document::UndoGroup::~UndoGroup() {
    if (!_closed && _doc) {
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

void Document::markUndoStateAsSaved() {
    _savedUndoStep = _currentUndoStep;
}

void Document::initalUndoStep(int endCodeUnit, int endLine) {
    _collapseUndoStep = false;
    _groupUndo = 0;
    _undoStepCreationDeferred = false;
    _undoSteps.clear();
    _undoSteps.append({ _lines, endCodeUnit, endLine});
    _currentUndoStep = 0;
    _savedUndoStep = _currentUndoStep;
    emitModifedSignals();
}

namespace {
    struct SearchParameter {
        bool searchWrap = false;
        Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
        int startAtLine = 0;
        int startCodeUnit = 0;
        std::variant<QString, QRegularExpression> needle;
    };

    struct NoCanceler {
        bool isCanceled() {
            return false;
        }
    };

    DocumentFindAsyncResult noMatch(const DocumentSnapshot &snap) {
        return DocumentFindAsyncResult{{0, 0}, {0, 0}, snap.revision(), QRegularExpressionMatch{}};
    }

    DocumentFindAsyncResult noMatch(unsigned revision) {
        return DocumentFindAsyncResult{{0, 0}, {0, 0}, revision, QRegularExpressionMatch{}};
    }

    void replaceInvalidUtf16ForRegexSearch(QString &buffer, int start) {
        for (int i = start; i < buffer.size(); i++) {
            QChar ch = buffer[i];
            if (ch.isHighSurrogate()) {
                if (i + 1 < buffer.size() && QChar(buffer[i + 1]).isLowSurrogate()) {
                    // ok, skip low surrogate
                    i++;
                } else {
                    // not valid utf16, replace so it doesn't break regex search
                    buffer[i] = 0xFFFD;
                }
            } else if (ch.isLowSurrogate()) {
                // not valid utf16, replace so it doesn't break regex search
                // this might be a surrogate escape but libpcre (used by QRegularExpression) can't work
                // with surrogate escapes
                buffer[i] = 0xFFFD;
            }
        }
    }

    bool isPotententialMultiLineMatch(const QRegularExpression &regex) {
        // This is mostly at placeholder at the moment, should parse the regex quite a bit more.

        // Currently everything that contains characters that would be quoted is considered potentially
        // multi line. This of course has lots of false positives but is very easy and does not have false negatives.
        for (QChar ch: regex.pattern()) {
            if ((ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
                    || (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z'))
                    || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
                    || ch == QLatin1Char('_')) {
                // if not preceeded by any non literal characters this is a liternal match
            } else {
                return true;
            }
        }
        return false;
    }

    template <typename CANCEL>
    static DocumentFindAsyncResult snapshotSearchForwardRegex(DocumentSnapshot snap, SearchParameter search, CANCEL &canceler) {

        auto regex = std::get<QRegularExpression>(search.needle);
        if ((regex.patternOptions() & QRegularExpression::PatternOption::MultilineOption) == 0) {
            regex.setPatternOptions(regex.patternOptions() | QRegularExpression::PatternOption::MultilineOption);
        }
        if (regex.patternOptions().testFlag(QRegularExpression::PatternOption::CaseInsensitiveOption) !=
                (search.caseSensitivity == Qt::CaseInsensitive)) {
            regex.setPatternOptions(regex.patternOptions() ^ QRegularExpression::PatternOption::CaseInsensitiveOption);
        }

        int line = search.startAtLine;
        int found = search.startCodeUnit - 1;
        int end = snap.lineCount();

        bool hasWrapped = false;

        while (true) {
            for (; line < end; line++) {
                QString buffer = snap.line(line);
                replaceInvalidUtf16ForRegexSearch(buffer, 0);
                buffer += "\n";
                int foldedLine = line;
                QRegularExpressionMatchIterator remi
                        = regex.globalMatch(buffer, 0,
                                            foldedLine + 1 < snap.lineCount() ? QRegularExpression::MatchType::PartialPreferFirstMatch
                                                                              : QRegularExpression::MatchType::NormalMatch,
                                            QRegularExpression::MatchOption::DontCheckSubjectStringMatchOption);
                while (remi.hasNext()) {
                    QRegularExpressionMatch match = remi.next();
                    if (canceler.isCanceled()) {
                        return noMatch(snap);
                    }
                    if (match.hasPartialMatch()) {
                        const int cont = buffer.size();
                        foldedLine += 1;
                        buffer += snap.line(foldedLine);
                        replaceInvalidUtf16ForRegexSearch(buffer, cont);
                        buffer += "\n";
                        remi = regex.globalMatch(buffer, 0,
                                                 foldedLine + 1 < snap.lineCount() ? QRegularExpression::MatchType::PartialPreferFirstMatch
                                                                                   : QRegularExpression::MatchType::NormalMatch,
                                                 QRegularExpression::MatchOption::DontCheckSubjectStringMatchOption);
                        continue;
                    }
                    if (match.capturedLength() <= 0) continue;
                    if (match.capturedStart() < found + 1) continue;
                    found = match.capturedStart();
                    int foundLine = line;
                    while (found > snap.lineCodeUnits(foundLine)) {
                        found -= snap.lineCodeUnits(foundLine);
                        found -= 1; // the "\n" itself
                        foundLine += 1;
                    }
                    int endLine = line;
                    int endCodeUnit = match.capturedStart() + match.capturedLength();
                    while (endCodeUnit > snap.lineCodeUnits(endLine)) {
                        endCodeUnit -= snap.lineCodeUnits(endLine);
                        endCodeUnit -= 1; // the "\n" itself
                        endLine += 1;
                    }
                    return DocumentFindAsyncResult{{found, foundLine},
                                                   {endCodeUnit, endLine},
                                                   snap.revision(),
                                                   match};
                }
                // we searched everything until including folded line, so no need to try those lines again.
                line = foldedLine;
                found = -1;

                if (canceler.isCanceled()) {
                    return noMatch(snap);
                }
            }
            if (!search.searchWrap || hasWrapped) {
                return noMatch(snap);
            }
            hasWrapped = true;
            end = std::min(search.startAtLine + 1, snap.lineCount());
            line = 0;
        }
        return noMatch(snap);
    }

    template <typename CANCEL>
    static DocumentFindAsyncResult snapshotSearchForwardLiteral(DocumentSnapshot snap, SearchParameter search, CANCEL &canceler) {

        const QString needle = std::get<QString>(search.needle);
        const QStringList parts = needle.split('\n');

        int line = search.startAtLine;
        int found = search.startCodeUnit - 1;
        int end = snap.lineCount();

        bool hasWrapped = false;
        while (true) {
            for (; line < end; line++) {
                if (parts.size() > 1) {
                    const int numberLinesToCome = snap.lineCount() - line;
                    if (parts.size() > numberLinesToCome) {
                        found = -1;
                        continue;
                    }
                    const int markStart = snap.line(line).size() - parts.first().size();
                    if (found < markStart && snap.line(line).endsWith(parts.first(), search.caseSensitivity)) {
                        found = markStart;
                        if (snap.line(line + parts.size() - 1).startsWith(parts.last(), search.caseSensitivity)) {
                            for (int i = parts.size() - 2; i > 0; i--) {
                                if (snap.line(line + i).compare(parts.at(i), search.caseSensitivity)) {
                                    i = found = -1;
                                }
                            }
                            if (found != -1)
                                return DocumentFindAsyncResult{{found, line},
                                                               {parts.last().size(), line + parts.size() - 1},
                                                               snap.revision(),
                                                               QRegularExpressionMatch{}};
                       }
                    }
                    found = -1;
                } else {
                    found = snap.line(line).indexOf(needle, found + 1, search.caseSensitivity);

                    if (found != -1) {
                        const int length = needle.size();
                        return DocumentFindAsyncResult{{found, line},
                                                       {found + length, line},
                                                       snap.revision(),
                                                       QRegularExpressionMatch{}};
                    }
                }

                if (canceler.isCanceled()) {
                    return noMatch(snap);
                }
            }
            if (!search.searchWrap || hasWrapped) {
                return noMatch(snap);
            }
            hasWrapped = true;
            end = std::min(search.startAtLine + 1, snap.lineCount());
            line = 0;
        }
        return noMatch(snap);
    }

    template <typename CANCEL>
    static DocumentFindAsyncResult snapshotSearchForward(DocumentSnapshot snap, SearchParameter search, CANCEL &canceler) {
        const bool regularExpressionMode = std::holds_alternative<QRegularExpression>(search.needle);
        if (regularExpressionMode) {
            return snapshotSearchForwardRegex(snap, search, canceler);
        } else {
            return snapshotSearchForwardLiteral(snap, search, canceler);
        }
    }

    template <typename CANCEL>
    static DocumentFindAsyncResult snapshotSearchBackwardsRegex(DocumentSnapshot snap, SearchParameter search, CANCEL &canceler) {

        auto regex = std::get<QRegularExpression>(search.needle);
        if (regex.patternOptions().testFlag(QRegularExpression::PatternOption::CaseInsensitiveOption) !=
                (search.caseSensitivity == Qt::CaseInsensitive)) {
            regex.setPatternOptions(regex.patternOptions() ^ QRegularExpression::PatternOption::CaseInsensitiveOption);
        }

        if (isPotententialMultiLineMatch(regex)) {
            if ((regex.patternOptions() & QRegularExpression::PatternOption::MultilineOption) == 0) {
                regex.setPatternOptions(regex.patternOptions() | QRegularExpression::PatternOption::MultilineOption);
            }

            // Matching in reverse is quite hard to get right and performant. For now just get it right.
            // If this ever is a bottleneck in actual use, we need to think how to improve performance.
            // For some cases a simple cache of all match positions might be enough.

            QString buffer;
            int startIndex = -1;
            for (int i = 0; i < snap.lineCount(); i++) {
                if (i == search.startAtLine) {
                    startIndex = buffer.size() + search.startCodeUnit;
                }

                buffer += snap.line(i);
                buffer += "\n";

                if (canceler.isCanceled()) {
                    return noMatch(snap);
                }
            }

            if (startIndex == -1) {
                startIndex = buffer.size();
            }

            replaceInvalidUtf16ForRegexSearch(buffer, 0);

            std::optional<QRegularExpressionMatch> noWrapMatch;
            std::optional<QRegularExpressionMatch> wrapMatch;
            QRegularExpressionMatchIterator remi = regex.globalMatch(buffer);
            while (remi.hasNext()) {
                QRegularExpressionMatch match = remi.next();
                if (canceler.isCanceled()) {
                    return noMatch(snap);
                }
                if (match.capturedLength() <= 0) continue;

                if (match.capturedStart() <= startIndex - match.capturedLength()) {
                    noWrapMatch = match;
                    continue;
                }

                if (!search.searchWrap) {
                    // No wrapping requested, we have all we need.
                    break;
                }

                if (noWrapMatch) {
                    // No wrapping needed, we have the match.
                    break;
                }

                wrapMatch = match;
            }

            if (noWrapMatch || wrapMatch) {
                QRegularExpressionMatch match = noWrapMatch ? *noWrapMatch : *wrapMatch;
                int found = match.capturedStart();
                int foundLine = 0;
                while (found > snap.lineCodeUnits(foundLine)) {
                    found -= snap.lineCodeUnits(foundLine);
                    found -= 1; // the "\n" itself
                    foundLine += 1;
                }
                int endLine = 0;
                int endCodeUnit = match.capturedStart() + match.capturedLength();
                while (endCodeUnit > snap.lineCodeUnits(endLine)) {
                    endCodeUnit -= snap.lineCodeUnits(endLine);
                    endCodeUnit -= 1; // the "\n" itself
                    endLine += 1;
                }
                return DocumentFindAsyncResult{{found, foundLine},
                                               {endCodeUnit, endLine},
                                               snap.revision(),
                                               match};

            } else {
                return noMatch(snap);
            }
            // never reached
        }

        if ((regex.patternOptions() & QRegularExpression::PatternOption::MultilineOption)) {
            regex.setPatternOptions(regex.patternOptions() ^ QRegularExpression::PatternOption::MultilineOption);
        }

        int line = search.startAtLine;
        int searchAt = search.startCodeUnit;
        int end = 0;
        bool hasWrapped = false;
        while (true) {
            for (; line >= end;) {
                QString lineBuffer = snap.line(line);
                replaceInvalidUtf16ForRegexSearch(lineBuffer, 0);

                DocumentFindAsyncResult res = noMatch(snap);
                QRegularExpressionMatchIterator remi = regex.globalMatch(lineBuffer);
                while (remi.hasNext()) {
                    QRegularExpressionMatch match = remi.next();
                    if (canceler.isCanceled()) {
                        return noMatch(snap);
                    }
                    if (match.capturedLength() <= 0) continue;
                    if (match.capturedStart() <= searchAt - match.capturedLength()) {
                        res = DocumentFindAsyncResult{{match.capturedStart(), line},
                                                      {match.capturedStart() + match.capturedLength(), line},
                                                      snap.revision(),
                                                      match};
                        continue;
                    }
                    break;
                }
                if (res.anchor != res.cursor) {
                    return res;
                }

                if (canceler.isCanceled()) {
                    return noMatch(snap);
                }
                line -= 1;
                if (line >= 0) searchAt = snap.line(line).size();
            }
            if (!search.searchWrap || hasWrapped) {
                return noMatch(snap);
            }
            hasWrapped = true;
            end = search.startAtLine;
            line = snap.lineCount() - 1;
            searchAt = snap.lineCodeUnits(line);
        }

    }

    template <typename CANCEL>
    static DocumentFindAsyncResult snapshotSearchBackwardsLiteral(DocumentSnapshot snap, SearchParameter search, CANCEL &canceler) {

        const QString needle = std::get<QString>(search.needle);
        const QStringList parts = needle.split('\n');

        int line = search.startAtLine;
        int searchAt = search.startCodeUnit;
        int end = 0;

        bool hasWrapped = false;
        while (true) {
            for (; line >= end;) {
                if (parts.size() > 1) {
                    int endLine = line - parts.size() + 1;
                    if (endLine < 0) {
                        line = -1; //because the for loop does not do this for us.
                        searchAt = -1;
                        continue;
                    }
                    if (searchAt >= parts.last().size() && snap.line(line).startsWith(parts.last(), search.caseSensitivity)) {
                        searchAt = parts.last().size();
                        if (snap.line(endLine).endsWith(parts.first(), search.caseSensitivity)) {
                            for (int i = parts.size() - 2; i > 0; i--) {
                                if (snap.line(line - i).compare(parts.at(i), search.caseSensitivity)) {
                                    i = searchAt = -1;
                                }
                            }
                            if (searchAt != -1) {
                                int endAt = snap.line(endLine).size() - parts.first().size();
                                return DocumentFindAsyncResult{{endAt, endLine},
                                                               {searchAt, line},
                                                               snap.revision(),
                                                               QRegularExpressionMatch{}};
                            }
                        }
                    }
                    searchAt = -1;
                } else {
                    const int length = needle.size();
                    if (searchAt >= length) {
                        const int found = snap.line(line).lastIndexOf(needle,
                                                                      searchAt - length,
                                                                      search.caseSensitivity);
                        if (found != -1) {
                            return DocumentFindAsyncResult{{found, line},
                                                           {found + length, line},
                                                           snap.revision(),
                                                           QRegularExpressionMatch{}};
                        }
                    }
                }
                if (canceler.isCanceled()) {
                    return noMatch(snap);
                }
                line -= 1;
                if (line >= 0) searchAt = snap.line(line).size();
            }
            if (!search.searchWrap || hasWrapped) {
                return noMatch(snap);
            }
            hasWrapped = true;
            end = search.startAtLine;
            line = snap.lineCount() - 1;
            searchAt = snap.lineCodeUnits(line);
        }
    }

    template <typename CANCEL>
    static DocumentFindAsyncResult snapshotSearchBackwards(DocumentSnapshot snap, SearchParameter search, CANCEL &canceler) {
        const bool regularExpressionMode = std::holds_alternative<QRegularExpression>(search.needle);
        if (regularExpressionMode) {
            return snapshotSearchBackwardsRegex(snap, search, canceler);
        } else {
            return snapshotSearchBackwardsLiteral(snap, search, canceler);
        }
    }

    SearchParameter prepareSearchParameter(const Document *doc, const TextCursor &start, Document::FindFlags options) {
        SearchParameter res;

        res.searchWrap = options & Document::FindFlag::FindWrap;
        res.caseSensitivity = (options & Document::FindFlag::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive;

        auto [startCodeUnit, startLine] = start.selectionEndPos();
        if (options & Document::FindFlag::FindBackward) {
            if (start.hasSelection()) {
                if (options & Document::FindFlag::FindWrap || startLine > 0 || startCodeUnit > 1) {
                    startCodeUnit = startCodeUnit - 1;
                    if (startCodeUnit <= 0) {
                        if (--startLine < 0) {
                            startLine = doc->lineCount() - 1;
                        }
                        startCodeUnit = doc->lineCodeUnits(startLine);
                    }
                }
            }
        }

        res.startAtLine = startLine;
        res.startCodeUnit = startCodeUnit;
        return res;
    }

    void searchResultToTextCursor(TextCursor &cur, DocumentFindAsyncResult result) {
        if (result.anchor == result.cursor) {
            cur.clearSelection();
            return;
        }

        cur.setPosition(result.anchor);
        cur.setPosition(result.cursor, true);
    }

    class SearchOnThread : public QRunnable {
    public:
        void run() override {
            promise.reportStarted();

            if (promise.isCanceled()) {
                promise.reportFinished();
                return;
            }

            DocumentFindAsyncResult res = noMatch(snap);
            if (backwards) {
                res = snapshotSearchBackwards(snap, param, promise);
            } else {
                res = snapshotSearchForward(snap, param, promise);
            }

            promise.reportResult(res);

            promise.reportFinished();
        }

    public:
        QFutureInterface<DocumentFindAsyncResult> promise;
        DocumentSnapshot snap;
        SearchParameter param;
        bool backwards = false;
    };
}

TextCursor Document::findSync(const QString &subString, const TextCursor &start,
                              Document::FindFlags options) const {
    TextCursor res = start;

    if (subString.isEmpty()) {
        res.clearSelection();
        return res;
    }

    SearchParameter param = prepareSearchParameter(this, start, options);
    param.needle = subString;

    DocumentFindAsyncResult resTmp = noMatch(revision());
    NoCanceler noCancler;
    if (options & Document::FindFlag::FindBackward) {
        resTmp = snapshotSearchBackwards(snapshot(), param, noCancler);
    } else {
        resTmp = snapshotSearchForward(snapshot(), param, noCancler);
    }

    searchResultToTextCursor(res, resTmp);

    return res;
}

DocumentFindResult Document::findSyncWithDetails(const QRegularExpression &regex, const TextCursor &start,
                                                 Document::FindFlags options) const {
    TextCursor res = start;

    if (!regex.isValid()) {
        res.clearSelection();
        return {res, QRegularExpressionMatch{}};
    }

    SearchParameter param = prepareSearchParameter(this, start, options);
    param.needle = regex;

    DocumentFindAsyncResult resTmp = noMatch(revision());
    NoCanceler noCancler;
    if (options & Document::FindFlag::FindBackward) {
        resTmp = snapshotSearchBackwards(snapshot(), param, noCancler);
    } else {
        resTmp = snapshotSearchForward(snapshot(), param, noCancler);
    }

    searchResultToTextCursor(res, resTmp);

    return {res, resTmp._match};
}

TextCursor Document::findSync(const QRegularExpression &regex, const TextCursor &start, Document::FindFlags options) const {
    return findSyncWithDetails(regex, start, options).cursor;
}

QFuture<DocumentFindAsyncResult> Document::findAsync(const QString &subString, const TextCursor &start,
                                                     Document::FindFlags options) const {
    return findAsyncWithPool(QThreadPool::globalInstance(), 0, subString, start, options);
}

QFuture<DocumentFindAsyncResult> Document::findAsync(const QRegularExpression &expr, const TextCursor &start,
                                                     Document::FindFlags options) const {
    return findAsyncWithPool(QThreadPool::globalInstance(), 0, expr, start, options);
}

QFuture<DocumentFindAsyncResult> Document::findAsyncWithPool(QThreadPool *pool, int priority,
                                                             const QString &subString, const TextCursor &start,
                                                             Document::FindFlags options) const {

    QFutureInterface<DocumentFindAsyncResult> promise;

    QFuture<DocumentFindAsyncResult> future = promise.future();

    if (subString.isEmpty()) {
        DocumentFindAsyncResult res{TextCursor::Position(0, 0), TextCursor::Position(0, 0),
                                    revision(), QRegularExpressionMatch{}};

        promise.reportStarted();
        promise.reportResult(res);
        promise.reportFinished();
        return future;
    }

    SearchParameter param = prepareSearchParameter(this, start, options);
    param.needle = subString;

    SearchOnThread *runnable = new SearchOnThread();
    runnable->param = param;
    runnable->backwards = options & Document::FindFlag::FindBackward;
    runnable->snap = snapshot();
    runnable->promise = std::move(promise);

    pool->start(runnable, priority);

    return future;
}

QFuture<DocumentFindAsyncResult> Document::findAsyncWithPool(QThreadPool *pool, int priority,
                                                             const QRegularExpression &expr, const TextCursor &start,
                                                             Document::FindFlags options) const {

    QFutureInterface<DocumentFindAsyncResult> promise;

    QFuture<DocumentFindAsyncResult> future = promise.future();

    if (!expr.isValid()) {
        DocumentFindAsyncResult res{TextCursor::Position(0, 0), TextCursor::Position(0, 0),
                                    revision(), QRegularExpressionMatch{}};

        promise.reportStarted();
        promise.reportResult(res);
        promise.reportFinished();
        return future;
    }

    SearchParameter param = prepareSearchParameter(this, start, options);
    param.needle = expr;

    SearchOnThread *runnable = new SearchOnThread();
    runnable->param = param;
    runnable->backwards = options & Document::FindFlag::FindBackward;
    runnable->snap = snapshot();
    runnable->promise = std::move(promise);

    pool->start(runnable, priority);

    return future;
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
    _lines[line].revision++;
    _lines[line].chars.remove(codeUnitStart, codeUnits);

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
    noteContentsChange();
}

void Document::insertIntoLine(TextCursor *cursor, int line, int codeUnitStart, const QString &data) {
    _lines[line].revision++;
    _lines[line].chars.insert(codeUnitStart, data);

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
    noteContentsChange();
}

void Document::appendToLine(TextCursor *cursor, int line, const QString &data) {
    _lines[line].revision++;
    _lines[line].chars.append(data);

    debugConsistencyCheck(cursor);
    noteContentsChange();
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
        if (anchorLine >= start + count) {
            c->setAnchorPosition({anchorCodeUnit, anchorLine - count});
            positionMustBeSet = true;
        } else if (anchorLine >= _lines.size()) {
            c->setAnchorPosition({_lines[_lines.size() - 1].chars.size(), _lines.size() - 1});
            positionMustBeSet = true;
        } else if (anchorLine >= start) {
            c->setAnchorPosition({0, start});
            positionMustBeSet = true;
        }

        // position
        const auto [cursorCodeUnit, cursorLine] = c->position();
        if (cursorLine >= start + count) {
            c->setPositionPreservingVerticalMovementColumn({cursorCodeUnit, cursorLine - count}, true);
            positionMustBeSet = false;
        } else if (cursorLine >= _lines.size()) {
            c->setPositionPreservingVerticalMovementColumn({_lines[_lines.size() - 1].chars.size(), _lines.size() - 1}, true);
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
    noteContentsChange();
}

void Document::insertLine(TextCursor *cursor, int before, const QString &data) {
    _lines.insert(before, {data});

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
    noteContentsChange();
}

void Document::splitLine(TextCursor *cursor, TextCursor::Position pos) {
    _lines[pos.line].revision++;
    _lines.insert(pos.line + 1, {_lines[pos.line].chars.mid(pos.codeUnit)});
    _lines[pos.line].chars.resize(pos.codeUnit);

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
    noteContentsChange();
}

void Document::mergeLines(TextCursor *cursor, int line) {
    const int originalLineCodeUnits = _lines[line].chars.size();
    _lines[line].revision++;
    _lines[line].chars.append(_lines[line + 1].chars);
    if (line + 1 < _lines.size()) {
        _lines.remove(line + 1, 1);
    } else {
        _lines[line + 1].chars.clear();
        _lines[line + 1].revision++;
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
    noteContentsChange();
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

bool operator<=(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.line, a.codeUnit) <= std::tie(b.line, b.codeUnit);
}

bool operator>(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.line, a.codeUnit) > std::tie(b.line, b.codeUnit);
}

bool operator>=(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.line, a.codeUnit) >= std::tie(b.line, b.codeUnit);
}

bool operator==(const TextCursor::Position &a, const TextCursor::Position &b) {
    return std::tie(a.codeUnit, a.line) == std::tie(b.codeUnit, b.line);
}

bool operator!=(const TextCursor::Position &a, const TextCursor::Position &b) {
    return !(a == b);
}


TextCursor::TextCursor(Document *doc, Tui::ZWidget *widget, std::function<Tui::ZTextLayout(int line, bool wrappingAllowed)> createTextLayout)
    : _doc(doc), _widget(widget), _createTextLayout(createTextLayout) {
    _doc->registerTextCursor(this);
}

TextCursor::TextCursor(const TextCursor &other)
    : _cursorCodeUnit(other._cursorCodeUnit), _cursorLine(other._cursorLine),
      _anchorCodeUnit(other._anchorCodeUnit), _anchorLine(other._anchorLine),
      _VerticalMovementColumn(other._VerticalMovementColumn),
      _doc(other._doc), _widget(other._widget), _createTextLayout(other._createTextLayout)
{
    _doc->registerTextCursor(this);
}

TextCursor::~TextCursor() {
    _doc->unregisterTextCursor(this);
}

TextCursor &TextCursor::operator=(const TextCursor &other) {
    if (_doc != other._doc) {
        _doc->unregisterTextCursor(this);
        _doc = other._doc;
        _doc->registerTextCursor(this);
        scheduleChangeSignal();
    }

    _widget = other._widget;
    _createTextLayout = other._createTextLayout;

    if (_cursorCodeUnit != other._cursorCodeUnit
        || _cursorLine != other._cursorLine
        || _anchorCodeUnit != other._anchorCodeUnit
        || _anchorLine != other._anchorLine
        || _VerticalMovementColumn != other._VerticalMovementColumn) {
        _cursorCodeUnit = other._cursorCodeUnit;
        _cursorLine = other._cursorLine;
        _anchorCodeUnit = other._anchorCodeUnit;
        _anchorLine = other._anchorLine;
        _VerticalMovementColumn = other._VerticalMovementColumn;
        scheduleChangeSignal();
    }

    return *this;
}

void TextCursor::insertText(const QString &text) {
    auto undoGroup = _doc->startUndoGroup(this);
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
        _doc->removeFromLine(this, start.line, start.codeUnit, _doc->_lines[start.line].chars.size() - start.codeUnit);
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
        return _doc->_lines[start.line].chars.mid(start.codeUnit, end.codeUnit - start.codeUnit);
    } else {
        QString res = _doc->_lines[start.line].chars.mid(start.codeUnit);
        for (int line = start.line + 1; line < end.line; line++) {
            res += "\n";
            res += _doc->_lines[line].chars;
        }
        res += "\n";
        res += _doc->_lines[end.line].chars.mid(0, end.codeUnit);
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
    auto undoGroup = _doc->startUndoGroup(this);
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
        setPosition({_doc->_lines[currentLine - 1].chars.size(), currentLine - 1}, extendSelection);
    } else {
        // here we update the selection and the vertical movemend position
        setPosition({currentCodeUnit, currentLine}, extendSelection);
    }
}

void TextCursor::moveCharacterRight(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit < _doc->_lines[currentLine].chars.size()) {
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
        setPosition({_doc->_lines[currentLine - 1].chars.size(), currentLine - 1}, extendSelection);
    }
}

void TextCursor::moveWordRight(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    if (currentCodeUnit < _doc->_lines[currentLine].chars.size()) {
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
    for (; i < _doc->_lines[currentLine].chars.size(); i++) {
        if (_doc->_lines[currentLine].chars[i] != ' ' && _doc->_lines[currentLine].chars[i] != '\t') {
            break;
        }
    }

    setPosition({i, currentLine}, extendSelection);
}

void TextCursor::moveToEndOfLine(bool extendSelection) {
    const auto [currentCodeUnit, currentLine] = position();
    setPosition({_doc->_lines[currentLine].chars.size(), currentLine}, extendSelection);
}

void TextCursor::moveToStartOfDocument(bool extendSelection) {
    setPosition({0, 0}, extendSelection);
}

void TextCursor::moveToEndOfDocument(bool extendSelection) {
    setPosition({_doc->_lines.last().chars.size(), _doc->_lines.size() - 1}, extendSelection);
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
    auto cursorCodeUnit = std::max(std::min(pos.codeUnit, _doc->_lines[cursorLine].chars.size()), 0);

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
    auto anchorCodeUnit = std::max(std::min(pos.codeUnit, _doc->_lines[anchorLine].chars.size()), 0);

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
            && _cursorCodeUnit == _doc->_lines[_cursorLine].chars.size();
}

bool TextCursor::atLineStart() const {
    return _cursorCodeUnit == 0;
}

bool TextCursor::atLineEnd() const {
    return _cursorCodeUnit == _doc->_lines[_cursorLine].chars.size();
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
    } else if (_anchorCodeUnit > _doc->_lines[_anchorLine].chars.size()) {
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
    } else if (_cursorCodeUnit > _doc->_lines[_cursorLine].chars.size()) {
        qFatal("TextCursor::debugConsistencyCheck: _cursorCodeUnit beyond max");
        abort();
    }
}

LineMarker::LineMarker(Document *doc) : LineMarker(doc, 0) {
}

LineMarker::LineMarker(const LineMarker &other) : LineMarker(other._doc, other._line) {
}

LineMarker::LineMarker(Document *doc, int line) : _line(line), _doc(doc) {
    _doc->registerLineMarker(this);
}

LineMarker::~LineMarker() {
    _doc->unregisterLineMarker(this);
}

LineMarker &LineMarker::operator=(const LineMarker &other) {

    if (_doc != other._doc) {
        _doc->unregisterLineMarker(this);
        _doc = other._doc;
        _doc->registerLineMarker(this);
        _changed = true;
        _doc->scheduleChangeSignals();
    }

    if (_line == other._line) {
        _line = other._line;
        _changed = true;
        _doc->scheduleChangeSignals();
    }

    return *this;
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
    return pimpl->_lines[line].chars;
}

int DocumentSnapshot::lineCodeUnits(int line) const {
    return pimpl->_lines[line].chars.size();
}

unsigned DocumentSnapshot::lineRevision(int line) const {
    return pimpl->_lines[line].revision;
}

std::shared_ptr<UserData> DocumentSnapshot::lineUserData(int line) const {
    return pimpl->_lines[line].userData;
}

unsigned DocumentSnapshot::revision() const {
    return pimpl->_revision;
}

bool DocumentSnapshot::isUpToDate() const {
    return pimpl->_revision == pimpl->_revisionShared->load(std::memory_order_relaxed);
}

UserData::~UserData() {
}

int DocumentFindAsyncResult::regexLastCapturedIndex() const {
    return _match.lastCapturedIndex();
}

QString DocumentFindAsyncResult::regexCapture(int index) const {
    return _match.captured(index);
}

QString DocumentFindAsyncResult::regexCapture(const QString &name) const {
    return _match.captured(name);
}

DocumentFindAsyncResult::DocumentFindAsyncResult(TextCursor::Position anchor, TextCursor::Position cursor, unsigned revision, QRegularExpressionMatch match)
    : anchor(anchor), cursor(cursor), revision(revision), _match(match)
{
}

int DocumentFindResult::regexLastCapturedIndex() const {
    return _match.lastCapturedIndex();
}

QString DocumentFindResult::regexCapture(int index) const {
    return _match.captured(index);
}

QString DocumentFindResult::regexCapture(const QString &name) const {
    return _match.captured(name);
}

DocumentFindResult::DocumentFindResult(TextCursor cursor, QRegularExpressionMatch match)
    : cursor(cursor), _match(match)
{
}
