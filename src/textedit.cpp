// SPDX-License-Identifier: BSL-1.0

#include "textedit.h"

#include <QFutureWatcher>

#include <Tui/ZClipboard.h>
#include <Tui/ZPainter.h>
#include <Tui/ZSymbol.h>


ZTextEdit::ZTextEdit(Tui::ZTextMetrics textMetrics, Tui::ZWidget *parent)
    : ZTextEdit(textMetrics, nullptr, parent)
{
}

ZTextEdit::ZTextEdit(Tui::ZTextMetrics textMetrics, Tui::ZDocument *document, Tui::ZWidget *parent)
    : Tui::ZWidget(parent), _textMetrics(textMetrics),
      _doc(document ? document : new Tui::ZDocument(this)),
      _cursor(makeCursor()),
      _scrollPositionLine(_doc)
{    
    setFocusPolicy(Qt::StrongFocus);
    setCursorStyle(Tui::CursorStyle::Bar);


    QObject::connect(_doc, &Tui::ZDocument::modificationChanged, this, &ZTextEdit::modifiedChanged);

    QObject::connect(_doc, &Tui::ZDocument::lineMarkerChanged, this, [this](const Tui::ZDocumentLineMarker *marker) {
        if (marker == &_scrollPositionLine) {
            // Recalculate the scroll position:
            //  * In case any editing from outside of this class moved our scroll position line marker to ensure
            //    that the line marker still keeps the cursor visible
            // Internal changes trigger these signals as well, that should be safe, as long as adjustScrollPosition
            // does not change the scroll position when called again as long as neither document nor the cursor is
            // changed inbetween.
            adjustScrollPosition();
        }
    });

    QObject::connect(_doc, &Tui::ZDocument::cursorChanged, this, [this](const Tui::ZDocumentCursor *changedCursor) {
        if (changedCursor == &_cursor) {
            // Ensure that even if editing from outside of this class moved the cursor position it is still in the
            // visible portion of the widget.
            adjustScrollPosition();

            // this is our main way to notify other components of cursor position changes.
            emitCursorPostionChanged();
        }
    });
}

void ZTextEdit::registerCommandNotifiers(Qt::ShortcutContext context) {
    if (_cmdCopy) {
        // exit if already done
        return;
    }

    _cmdCopy = new Tui::ZCommandNotifier("Copy", this, context);
    QObject::connect(_cmdCopy, &Tui::ZCommandNotifier::activated, this, &ZTextEdit::copy);
    _cmdCopy->setEnabled(canCopy());

    _cmdCut = new Tui::ZCommandNotifier("Cut", this, context);
    QObject::connect(_cmdCut, &Tui::ZCommandNotifier::activated, this, &ZTextEdit::cut);
    _cmdCut->setEnabled(canCut());

    _cmdPaste = new Tui::ZCommandNotifier("Paste", this, context);
    QObject::connect(_cmdPaste, &Tui::ZCommandNotifier::activated, this, &ZTextEdit::paste);
    updatePasteCommandEnabled();

    Tui::ZClipboard *clipboard = findFacet<Tui::ZClipboard>();
    if (clipboard) {
        QObject::connect(clipboard, &Tui::ZClipboard::contentsChanged, this, &ZTextEdit::updatePasteCommandEnabled);
    }

    _cmdUndo = new Tui::ZCommandNotifier("Undo", this, context);
    QObject::connect(_cmdUndo, &Tui::ZCommandNotifier::activated, this, &ZTextEdit::undo);
    _cmdUndo->setEnabled(_doc->isUndoAvailable());
    QObject::connect(_doc, &Tui::ZDocument::undoAvailable, _cmdUndo, &Tui::ZCommandNotifier::setEnabled);

    _cmdRedo = new Tui::ZCommandNotifier("Redo", this, context);
    QObject::connect(_cmdRedo, &Tui::ZCommandNotifier::activated, this, &ZTextEdit::redo);
    _cmdRedo->setEnabled(_doc->isRedoAvailable());
    QObject::connect(_doc, &Tui::ZDocument::redoAvailable, _cmdRedo, &Tui::ZCommandNotifier::setEnabled);
}

void ZTextEdit::emitCursorPostionChanged() {
    const auto [cursorCodeUnit, cursorLine] = _cursor.position();
    Tui::ZTextLayout layNoWrap = textLayoutForLineWithoutWrapping(cursorLine);
    int cursorColumn = layNoWrap.lineAt(0).cursorToX(cursorCodeUnit, Tui::ZTextLayout::Leading);
    int utf8CodeUnit = _doc->line(cursorLine).leftRef(cursorCodeUnit).toUtf8().size();
    cursorPositionChanged(cursorColumn, utf8CodeUnit, cursorLine);
}

void ZTextEdit::setCursorPosition(Position position, bool extendSelection) {
    clearAdvancedSelection();

    _cursor.setPosition(position, extendSelection);

    updateCommands();
    adjustScrollPosition();
    update();
}

Tui::ZDocumentCursor::Position ZTextEdit::cursorPosition() const {
    return _cursor.position();
}

void ZTextEdit::setAnchorPosition(Position position) {
    clearAdvancedSelection();

    _cursor.setAnchorPosition(position);

    updateCommands();
    update();
}

Tui::ZDocumentCursor::Position ZTextEdit::anchorPosition() const {
    return _cursor.anchor();
}

void ZTextEdit::setSelection(Position anchor, Position position) {
    clearAdvancedSelection();

    _cursor.setAnchorPosition(anchor);
    _cursor.setPosition(position, true);

    updateCommands();
    adjustScrollPosition();
    update();
}

void ZTextEdit::setTextCursor(const Tui::ZDocumentCursor &cursor) {
    clearAdvancedSelection();

    _cursor.setAnchorPosition(cursor.anchor());
    _cursor.setPositionPreservingVerticalMovementColumn(cursor.position(), true);
    _cursor.setVerticalMovementColumn(cursor.verticalMovementColumn());

    updateCommands();
    adjustScrollPosition();
    update();
}

Tui::ZDocumentCursor ZTextEdit::textCursor() const {
    return _cursor;
}

Tui::ZDocumentCursor ZTextEdit::makeCursor() {
    return Tui::ZDocumentCursor(_doc, [this](int line, bool wrappingAllowed) {
        Tui::ZTextLayout lay(_textMetrics, _doc->line(line));
        Tui::ZTextOption option;
        option.setTabStopDistance(_tabsize);
        if (wrappingAllowed) {
            option.setWrapMode(_wrapMode);
            lay.setTextOption(option);
            lay.doLayout(std::max(rect().width() - allBordersWidth(), 0));
        } else {
            lay.setTextOption(option);
            lay.doLayout(std::numeric_limits<unsigned short>::max() - 1);
        }
        return lay;
    });
}

Tui::ZDocument *ZTextEdit::document() const {
    return _doc;
}

int ZTextEdit::lineNumberBorderWidth() const {
    if (_showLineNumbers) {
        return QString::number(_doc->lineCount()).size() + 1;
    }
    return 0;
}

void ZTextEdit::setTabStopDistance(int tab) {
    _tabsize = std::max(1, tab);
    adjustScrollPosition();
    update();
}

int ZTextEdit::tabStopDistance() const {
    return _tabsize;
}

void ZTextEdit::setShowLineNumbers(bool show) {
    _showLineNumbers = show;
    adjustScrollPosition();
    update();
}

bool ZTextEdit::showLineNumbers() const {
    return _showLineNumbers;
}

void ZTextEdit::setUseTabChar(bool tab) {
    _useTabChar = tab;
    update();
}

bool ZTextEdit::useTabChar() const {
    return _useTabChar;
}

void ZTextEdit::setWordWrapMode(Tui::ZTextOption::WrapMode wrap) {
    _wrapMode = wrap;
    if (_wrapMode != Tui::ZTextOption::WrapMode::NoWrap) {
        _scrollPositionColumn = 0;
    }
    adjustScrollPosition();
    update();
}

Tui::ZTextOption::WrapMode ZTextEdit::wordWrapMode() const {
    return _wrapMode;
}

void ZTextEdit::setOverwriteMode(bool mode) {
    if (_overwriteMode != mode) {
        _overwriteMode = mode;
        if (_overwriteMode) {
            setCursorStyle(_overwriteCursorStyle);
        } else {
            setCursorStyle(_insertCursorStyle);
        }
        overwriteModeChanged(_overwriteMode);
        update();
    }
}

void ZTextEdit::toggleOverwriteMode() {
    setOverwriteMode(!overwriteMode());
}

bool ZTextEdit::overwriteMode() const {
    return _overwriteMode;
}

void ZTextEdit::setSelectMode(bool mode) {
    if (_selectMode != mode) {
        _selectMode = mode;
        selectModeChanged(_selectMode);
    }
}

void ZTextEdit::toggleSelectMode() {
    setSelectMode(!_selectMode);
}

bool ZTextEdit::selectMode() const {
    return _selectMode;
}

void ZTextEdit::setInsertCursorStyle(Tui::CursorStyle style) {
    _insertCursorStyle = style;
    if (!_overwriteMode) {
        setCursorStyle(_insertCursorStyle);
    }
    update();
}

Tui::CursorStyle ZTextEdit::insertCursorStyle() const {
    return _insertCursorStyle;
}

void ZTextEdit::setOverwriteCursorStyle(Tui::CursorStyle style) {
    _overwriteCursorStyle = style;
    if (_overwriteMode) {
        setCursorStyle(_overwriteCursorStyle);
    }
    update();
}

Tui::CursorStyle ZTextEdit::overwriteCursorStyle() const {
    return _overwriteCursorStyle;
}

void ZTextEdit::setTabChangesFocus(bool enabled) {
    _tabChangesFocus = enabled;
}

bool ZTextEdit::tabChangesFocus() const {
    return _tabChangesFocus;
}

void ZTextEdit::setReadOnly(bool readOnly) {
    _readOnly = readOnly;
}

bool ZTextEdit::isReadOnly() const {
    return _readOnly;
}

void ZTextEdit::setUndoRedoEnabled(bool enabled) {
    _undoRedoEnabled = enabled;
}

bool ZTextEdit::isUndoRedoEnabled() const {
    return _undoRedoEnabled;
}

bool ZTextEdit::isModified() const {
    return _doc->isModified();
}

void ZTextEdit::insertText(const QString &str) {
    _cursor.insertText(str);

    updateCommands();
    adjustScrollPosition();
    update();
}

void ZTextEdit::insertTabAt(Tui::ZDocumentCursor &cur) {
    auto undoGroup = _doc->startUndoGroup(&_cursor);

    if (useTabChar()) {
        cur.insertText("\t");
    } else {
        Tui::ZTextLayout lay = textLayoutForLineWithoutWrapping(cur.position().line);
        Tui::ZTextLineRef tlr = lay.lineAt(0);
        const int colum = tlr.cursorToX(cur.position().codeUnit, Tui::ZTextLayout::Leading);
        const int remainingTabWidth = tabStopDistance() - colum % tabStopDistance();
        cur.insertText(QStringLiteral(" ").repeated(remainingTabWidth));
    }
    adjustScrollPosition();
    update();
}

void ZTextEdit::cut() {
    copy();
    removeSelectedText();
    updateCommands();
    adjustScrollPosition();
    update();
}

void ZTextEdit::copy() {
    if (_cursor.hasSelection()) {
        Tui::ZClipboard *clipboard = findFacet<Tui::ZClipboard>();
        if (clipboard) {
            clipboard->setContents(_cursor.selectedText());
        }
    }
}

void ZTextEdit::pasteEvent(Tui::ZPasteEvent *event) {
    QString text = event->text();

    text.replace(QString("\r\n"), QString("\n"));
    text.replace(QLatin1Char('\r'), QString(QLatin1Char('\n')));

    // Inserting might adjust the scroll position, so save it here and restore it later.
    const int line = _scrollPositionLine.line();
    _cursor.insertText(text);
    _scrollPositionLine.setLine(line);

    _doc->clearCollapseUndoStep();
    adjustScrollPosition();
    update();
}

void ZTextEdit::paste() {
    auto undoGroup = _doc->startUndoGroup(&_cursor);
    Tui::ZClipboard *clipboard = findFacet<Tui::ZClipboard>();
    if (clipboard && clipboard->contents().size()) {
        // Inserting might adjust the scroll position, so save it here and restore it later.
        const int line = _scrollPositionLine.line();
        _cursor.insertText(clipboard->contents());
        _scrollPositionLine.setLine(line);
        adjustScrollPosition();
        updateCommands();
        _doc->clearCollapseUndoStep();
        update();
    }
}

Tui::ZDocument::UndoGroup ZTextEdit::startUndoGroup() {
    return _doc->startUndoGroup(&_cursor);
}

void ZTextEdit::removeSelectedText() {
    if (_cursor.hasSelection()) {
        auto undoGroup = _doc->startUndoGroup(&_cursor);
        _cursor.removeSelectedText();
    }

    adjustScrollPosition();
    updateCommands();
    update();
}

void ZTextEdit::clearSelection() {
    clearAdvancedSelection();

    _cursor.clearSelection();
    setSelectMode(false);

    updateCommands();
    update();
}

void ZTextEdit::selectAll() {
    clearAdvancedSelection();

    _cursor.selectAll();
    updateCommands();
    adjustScrollPosition();
    update();
}

QString ZTextEdit::selectedText() const {
    return _cursor.selectedText();
}

bool ZTextEdit::hasSelection() const {
    return _cursor.hasSelection();
}

void ZTextEdit::undo() {
    clearAdvancedSelection();

    setSelectMode(false);
    _doc->undo(&_cursor);
    adjustScrollPosition();
    update();
}

void ZTextEdit::redo() {
    clearAdvancedSelection();

    setSelectMode(false);
    _doc->redo(&_cursor);
    adjustScrollPosition();
    update();
}

void ZTextEdit::paintEvent(Tui::ZPaintEvent *event) {

    Tui::ZColor fg;
    Tui::ZColor bg;

    if (focus()) {
        bg = getColor("textedit.focused.bg");
        fg = getColor("textedit.focused.fg");
    } else if (!isEnabled()) {
        bg = getColor("textedit.disabled.bg");
        fg = getColor("textedit.disabled.fg");
    } else {
        bg = getColor("textedit.bg");
        fg = getColor("textedit.fg");
    }
    const Tui::ZTextStyle base{fg, bg};

    Tui::ZColor lineNumberFg = getColor("textedit.linenumber.fg");
    Tui::ZColor lineNumberBg = getColor("textedit.linenumber.bg");

    const Tui::ZTextStyle selected{getColor("textedit.selected.fg"),
                                   getColor("textedit.selected.bg"),
                                   Tui::ZTextAttribute::Bold};


    auto *painter = event->painter();
    painter->clear(fg, bg);

    setCursorColor(fg.redOrGuess(), fg.greenOrGuess(), fg.blueOrGuess());

    Tui::ZTextOption option = textOption();

    Position selectionStartPos(-1, -1);
    Position selectionEndPos(-1, -1);

    if (_cursor.hasSelection()) {
        selectionStartPos = _cursor.selectionStartPos();
        selectionEndPos = _cursor.selectionEndPos();
    }

    const auto [cursorCodeUnit, cursorLine] = _cursor.position();

    int y = -_scrollPositionFineLine;
    for (int line = _scrollPositionLine.line(); y < rect().height() && line < _doc->lineCount(); line++) {
        QVector<Tui::ZFormatRange> highlights;

        Tui::ZTextLayout lay = textLayoutForLine(option, line);

        if (line > selectionStartPos.line && line < selectionEndPos.line) {
            // whole line
            highlights.append(Tui::ZFormatRange{0, _doc->lineCodeUnits(line), selected, selected});
        } else if (line > selectionStartPos.line && line == selectionEndPos.line) {
            // selection ends on this line
            highlights.append(Tui::ZFormatRange{0, selectionEndPos.codeUnit, selected, selected});
        } else if (line == selectionStartPos.line && line < selectionEndPos.line) {
            // selection starts on this line
            highlights.append(Tui::ZFormatRange{selectionStartPos.codeUnit,
                                                _doc->lineCodeUnits(line) - selectionStartPos.codeUnit, selected, selected});
        } else if (line == selectionStartPos.line && line == selectionEndPos.line) {
            // selection is contained in this line
            highlights.append(Tui::ZFormatRange{selectionStartPos.codeUnit,
                                                selectionEndPos.codeUnit - selectionStartPos.codeUnit, selected, selected});
        }

        const bool lineBreakSelected = selectionStartPos.line <= line && selectionEndPos.line > line;

        if (lineBreakSelected) {
            Tui::ZTextLineRef lastLine = lay.lineAt(lay.lineCount() - 1);
            const int lineEndX = -_scrollPositionColumn + lastLine.width() + allBordersWidth();
            painter->clearRect(lineEndX, y + lastLine.y(),
                               rect().width() - lineEndX, 1,
                               selected.foregroundColor(), selected.backgroundColor());
        }

        lay.draw(*painter, {-_scrollPositionColumn + allBordersWidth(), y}, base, &base, highlights);

        if (cursorLine == line) {
            if (focus()) {
                lay.showCursor(*painter, {-_scrollPositionColumn + allBordersWidth(), y}, cursorCodeUnit);
            }
        }

        if (_showLineNumbers) {
            for (int i = lay.lineCount() - 1; i > 0; i--) {
                painter->writeWithColors(0, y + i,
                                         QString(" ").repeated(lineNumberBorderWidth()),
                                         lineNumberFg, lineNumberBg);
            }
            QString numberText = QString::number(line + 1)
                    + QString(" ").repeated(allBordersWidth() - QString::number(line + 1).size());
            int lineNumberY = y;
            if (y < 0) {
                numberText.replace(' ', '^');
                lineNumberY = 0;
            }
            if (line == cursorLine) {
                painter->writeWithAttributes(0, lineNumberY, numberText, lineNumberFg, lineNumberBg, Tui::ZTextAttribute::Bold);
            } else {
                painter->writeWithColors(0, lineNumberY, numberText, lineNumberFg, lineNumberBg);
            }
        }
        y += lay.lineCount();
    }
}


void ZTextEdit::keyEvent(Tui::ZKeyEvent *event) {
    auto undoGroup = _doc->startUndoGroup(&_cursor);

    QString text = event->text();

    if(event->key() == Tui::Key_Space && event->modifiers() == 0) {
        text = " ";
    }

    const bool editable = !_readOnly;
    const bool undoredo = editable && _undoRedoEnabled;

    if (editable && event->key() == Tui::Key_Backspace && event->modifiers() == 0) {
        _detachedScrolling = false;
        setSelectMode(false);
        _cursor.deletePreviousCharacter();
        updateCommands();
        adjustScrollPosition();
        update();
    } else if (editable && event->key() == Tui::Key_Backspace && event->modifiers() == Tui::ControlModifier) {
        _detachedScrolling = false;
        setSelectMode(false);
        _cursor.deletePreviousWord();
        updateCommands();
        adjustScrollPosition();
        update();
    } else if (editable && event->key() == Tui::Key_Delete && event->modifiers() == 0) {
        _detachedScrolling = false;
        setSelectMode(false);
        _cursor.deleteCharacter();
        updateCommands();
        adjustScrollPosition();
        update();
    } else if (editable && event->key() == Tui::Key_Delete && event->modifiers() == Tui::ControlModifier) {
        _detachedScrolling = false;
        setSelectMode(false);
        _cursor.deleteWord();
        updateCommands();
        adjustScrollPosition();
        update();
    } else if (editable && text.size() && event->modifiers() == 0) {
        _detachedScrolling = false;
        setSelectMode(false);
        if (overwriteMode() && !_cursor.hasSelection() && !_cursor.atLineEnd()) {
            _cursor.deleteCharacter();
        }

        // Inserting might adjust the scroll position, so save it here and restore it later.
        const int line = _scrollPositionLine.line();
        _cursor.insertText(text);
        _scrollPositionLine.setLine(line);
        adjustScrollPosition();
        updateCommands();
        update();
    } else if (event->key() == Qt::Key_Left && (event->modifiers() == 0 || event->modifiers() == Tui::ShiftModifier)) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        _cursor.moveCharacterLeft(extendSelection);
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Left
               && (event->modifiers() == Tui::ControlModifier || event->modifiers() == (Tui::ControlModifier | Tui::ShiftModifier))) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        _cursor.moveWordLeft(extendSelection);
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Right && (event->modifiers() == 0 || event->modifiers() == Tui::ShiftModifier)) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        _cursor.moveCharacterRight(extendSelection);
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Right
               && (event->modifiers() == Tui::ControlModifier || event->modifiers() == (Tui::ControlModifier | Tui::ShiftModifier))) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        _cursor.moveWordRight(extendSelection);
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Down && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        _cursor.moveDown(extendSelection);
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Up && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        _cursor.moveUp(extendSelection);
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Home && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;

        if (_cursor.atLineStart()) {
            _cursor.moveToStartIndentedText(extendSelection);
        } else {
            _cursor.moveToStartOfLine(extendSelection);
        }
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Home
               && (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        _cursor.moveToStartOfDocument(extendSelection);
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_End && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        _cursor.moveToEndOfLine(extendSelection);
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_End
               && (event->modifiers() == Qt::ControlModifier || (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)))) {
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        _cursor.moveToEndOfDocument(extendSelection);
        updateCommands();
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_PageDown && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        // Note: Shift+PageUp/Down does not work with xterm's default settings.
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        const int amount = pageNavigationLineCount();
        for (int i = 0; i < amount; i++) {
            _cursor.moveDown(extendSelection);
        }
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_PageUp && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        // Note: Shift+PageUp/Down does not work with xterm's default settings.
        _detachedScrolling = false;
        clearAdvancedSelection();

        const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
        const int amount = pageNavigationLineCount();
        for (int i = 0; i < amount; i++) {
            _cursor.moveUp(extendSelection);
        }
        adjustScrollPosition();
        update();
        _doc->clearCollapseUndoStep();
    } else if (editable && event->key() == Qt::Key_Enter && (event->modifiers() & ~Qt::KeypadModifier) == 0) {
        _detachedScrolling = false;
        setSelectMode(false);
        clearAdvancedSelection();

        // Inserting might adjust the scroll position, so save it here and restore it later.
        const int line = _scrollPositionLine.line();
        _cursor.insertText("\n");
        _scrollPositionLine.setLine(line);
        updateCommands();
        adjustScrollPosition();
        update();
    } else if (editable && !_tabChangesFocus && event->key() == Qt::Key_Tab && event->modifiers() == 0) {
        _detachedScrolling = false;
        if (_cursor.hasSelection()) {
            // Add one level of indent to the selected lines.

            const auto [firstLine, lastLine] = getSelectedLinesSort();
            const auto [startLine, endLine] = getSelectedLines();

            Tui::ZDocumentCursor cur = makeCursor();

            for (int line = firstLine; line <= lastLine; line++) {
                // Don't create new trailing whitespace only lines
                if (_doc->lineCodeUnits(line) > 0) {
                    if (useTabChar()) {
                        cur.setPosition({0, line});
                        cur.insertText(QString("\t"));
                    } else {
                        cur.setPosition({0, line});
                        cur.insertText(QString(" ").repeated(tabStopDistance()));
                    }
                }
            }

            selectLines(startLine, endLine);
        } else {
            // a normal tab
            setSelectMode(false);
            insertTabAt(_cursor);
        }
        updateCommands();
        adjustScrollPosition();
        update();
    } else if (editable && !_tabChangesFocus && event->key() == Qt::Key_Tab && event->modifiers() == Qt::ShiftModifier) {
        _detachedScrolling = false;
        // returns current line if no selection is active
        const auto [firstLine, lastLine] = getSelectedLinesSort();
        const auto [startLine, endLine] = getSelectedLines();
        const auto [cursorCodeUnit, cursorLine] = _cursor.position();

        const bool reselect =_cursor.hasSelection();

        int cursorAdjust = 0;

        for (int line = firstLine; line <= lastLine; line++) {
            int codeUnitsToRemove = 0;
            if (_doc->lineCodeUnits(line) && _doc->line(line)[0] == '\t') {
                codeUnitsToRemove = 1;
            } else {
                while (true) {
                    if (codeUnitsToRemove < _doc->lineCodeUnits(line)
                        && codeUnitsToRemove < tabStopDistance()
                        && _doc->line(line)[codeUnitsToRemove] == ' ') {
                        codeUnitsToRemove++;
                    } else {
                        break;
                    }
                }
            }

            _cursor.setPosition({0, line});
            _cursor.setPosition({codeUnitsToRemove, line}, true);
            _cursor.removeSelectedText();
            if (line == cursorLine) {
                cursorAdjust = codeUnitsToRemove;
            }
        }

        // Update cursor / recreate selection
        if (!reselect) {
            setCursorPosition({cursorCodeUnit - cursorAdjust, cursorLine});
        } else {
            selectLines(startLine, endLine);
        }
        updateCommands();
        adjustScrollPosition();
        update();
    } else if ((event->text() == "c" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Insert && event->modifiers() == Qt::ControlModifier) ) {
        // Ctrl + C and Ctrl + Insert
        _detachedScrolling = false;
        copy();
    } else if (editable && ((event->text() == "v" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Insert && event->modifiers() == Qt::ShiftModifier))) {
        // Ctrl + V and Shift + Insert
        _detachedScrolling = false;
        setSelectMode(false);
        paste();
    } else if (editable && ((event->text() == "x" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Delete && event->modifiers() == Qt::ShiftModifier))) {
        // Ctrl + X and Shift + Delete
        _detachedScrolling = false;
        setSelectMode(false);
        cut();
    } else if (undoredo && event->text() == "z" && event->modifiers() == Qt::ControlModifier) {
        _detachedScrolling = false;
        undoGroup.closeGroup();
        undo();
    } else if (undoredo && event->text() == "y" && event->modifiers() == Qt::ControlModifier) {
        _detachedScrolling = false;
        undoGroup.closeGroup();
        redo();
    } else if (event->text() == "a" && event->modifiers() == Qt::ControlModifier) {
        // Ctrl + a
        selectAll();
        _doc->clearCollapseUndoStep();
    } else if (editable && event->key() == Qt::Key_Insert && event->modifiers() == 0) {
        _detachedScrolling = false;
        toggleOverwriteMode();
    } else if (event->key() == Qt::Key_F4 && event->modifiers() == 0) {
        _detachedScrolling = false;
        toggleSelectMode();
    } else {
        undoGroup.closeGroup();
        Tui::ZWidget::keyEvent(event);
    }
}

int ZTextEdit::scrollPositionLine() const {
    return _scrollPositionLine.line();
}

int ZTextEdit::scrollPositionColumn() const {
    return _scrollPositionColumn;
}

int ZTextEdit::scrollPositionFineLine() const {
    return _scrollPositionFineLine;
}

void ZTextEdit::setScrollPosition(int column, int line, int fineLine) {
    if (column < 0 || line < 0 || fineLine < 0) {
        return;
    }

    if (_scrollPositionColumn != column || _scrollPositionLine.line() != line || _scrollPositionFineLine != fineLine) {

        if (line >= _doc->lineCount()) {
            return;
        }

        if (fineLine > 0) {
            Tui::ZTextLayout lay = textLayoutForLine(textOption(), line);
            if (fineLine >= lay.lineCount()) {
                return;
            }
        }

        _scrollPositionColumn = column;
        _scrollPositionLine.setLine(line);
        _scrollPositionFineLine = fineLine;
        scrollPositionChanged(column, line, fineLine);
        update();
    }
}

int ZTextEdit::pageNavigationLineCount() const {
    return std::max(1, geometry().height() - 1);
}

Tui::ZDocumentCursor ZTextEdit::findSync(const QString &subString,
                                         Tui::ZDocument::FindFlags options)
{
    Tui::ZDocumentCursor cur = _doc->findSync(subString, textCursor(), options);
    if (cur.hasSelection()) {
        if (_selectMode) {
            if (options & FindFlag::FindBackward) {
                setCursorPosition(cur.anchor(), true);
            } else {
                setCursorPosition(cur.position(), true);
            }
        } else {
            setTextCursor(cur);
        }
    } else {
        clearSelection();
    }
    return cur;
}

Tui::ZDocumentCursor ZTextEdit::findSync(const QRegularExpression &regex,
                                         Tui::ZDocument::FindFlags options)
{
    Tui::ZDocumentCursor cur = _doc->findSync(regex, textCursor(), options);
    if (cur.hasSelection()) {
        if (_selectMode) {
            if (options & FindFlag::FindBackward) {
                setCursorPosition(cur.anchor(), true);
            } else {
                setCursorPosition(cur.position(), true);
            }
        } else {
            setTextCursor(cur);
        }
    } else {
        clearSelection();
    }
    return cur;
}

Tui::ZDocumentFindResult ZTextEdit::findSyncWithDetails(const QRegularExpression &regex, FindFlags options)
{
    Tui::ZDocumentFindResult res = _doc->findSyncWithDetails(regex, textCursor(), options);
    if (res.cursor().hasSelection()) {
        if (_selectMode) {
            if (options & FindFlag::FindBackward) {
                setCursorPosition(res.cursor().anchor(), true);
            } else {
                setCursorPosition(res.cursor().position(), true);
            }
        } else {
            setTextCursor(res.cursor());
        }
    } else {
        clearSelection();
    }
    return res;
}


void ZTextEdit::connectAsyncFindWatcher(QFuture<Tui::ZDocumentFindAsyncResult> res, FindFlags options) {
    auto watcher = new QFutureWatcher<Tui::ZDocumentFindAsyncResult>();

    QObject::connect(watcher, &QFutureWatcher<Tui::ZDocumentFindAsyncResult>::finished, this, [this, watcher,options] {
        if (!watcher->isCanceled()) {
            Tui::ZDocumentFindAsyncResult res = watcher->future().result();
            if (res.anchor() != res.cursor()) { // has a match?
                if (_selectMode) {
                    if (options & FindFlag::FindBackward) {
                        setCursorPosition(res.anchor(), true);
                    } else {
                        setCursorPosition(res.cursor(), true);
                    }
                } else {
                    setSelection(res.anchor(), res.cursor());
                }

            } else {
                clearSelection();
            }
            updateCommands();
            adjustScrollPosition();
        }
        watcher->deleteLater();
    });

    watcher->setFuture(res);
}

QFuture<Tui::ZDocumentFindAsyncResult> ZTextEdit::findAsync(const QString &subString, Tui::ZDocument::FindFlags options)
{
    QFuture<Tui::ZDocumentFindAsyncResult> res = _doc->findAsync(subString, textCursor(), options);
    connectAsyncFindWatcher(res, options);
    return res;
}

QFuture<Tui::ZDocumentFindAsyncResult> ZTextEdit::findAsync(const QRegularExpression &regex, FindFlags options) {
    QFuture<Tui::ZDocumentFindAsyncResult> res = _doc->findAsync(regex, textCursor(), options);
    connectAsyncFindWatcher(res, options);
    return res;
}

QFuture<Tui::ZDocumentFindAsyncResult> ZTextEdit::findAsyncWithPool(QThreadPool *pool, int priority,
                                                                    const QString &subString, FindFlags options) {
    QFuture<Tui::ZDocumentFindAsyncResult> res = _doc->findAsyncWithPool(pool, priority, subString, textCursor(), options);
    connectAsyncFindWatcher(res, options);
    return res;
}

QFuture<Tui::ZDocumentFindAsyncResult> ZTextEdit::findAsyncWithPool(QThreadPool *pool,
                                                                    int priority,
                                                                    const QRegularExpression &regex, FindFlags options) {
    QFuture<Tui::ZDocumentFindAsyncResult> res = _doc->findAsyncWithPool(pool, priority, regex, textCursor(), options);
    connectAsyncFindWatcher(res, options);
    return res;
}

void ZTextEdit::clear() {
    _doc->reset();
    _scrollPositionLine.setLine(0);
    _scrollPositionColumn = 0;
    _scrollPositionFineLine = 0;

    cursorPositionChanged(0, 0, 0);
    scrollPositionChanged(0, 0, 0);
    setSelectMode(false);
    update();
}

void ZTextEdit::readFrom(QIODevice *file) {
    readFrom(file, {0, 0});
}

void ZTextEdit::readFrom(QIODevice *file, Position initialPosition) {
    _doc->readFrom(file, initialPosition, &_cursor);
    adjustScrollPosition();
    updateCommands();
    update();
}

void ZTextEdit::writeTo(QIODevice *file) const {
    _doc->writeTo(file, _doc->crLfMode());
    _doc->markUndoStateAsSaved();
}


Tui::ZTextOption ZTextEdit::textOption() const {
    Tui::ZTextOption option;
    option.setWrapMode(_wrapMode);
    option.setTabStopDistance(_tabsize);

    return option;
}

Tui::ZTextLayout ZTextEdit::textLayoutForLine(const Tui::ZTextOption &option, int line) const {
    Tui::ZTextLayout lay(_textMetrics, _doc->line(line));
    lay.setTextOption(option);
    if (_wrapMode != Tui::ZTextOption::WrapMode::NoWrap) {
        lay.doLayout(std::max(rect().width() - allBordersWidth(), 0));
    } else {
        lay.doLayout(std::numeric_limits<unsigned short>::max() - 1);
    }
    return lay;
}

Tui::ZTextLayout ZTextEdit::textLayoutForLineWithoutWrapping(int line) const {
    Tui::ZTextOption option = textOption();
    option.setWrapMode(Tui::ZTextOption::NoWrap);
    return textLayoutForLine(option, line);
}

const Tui::ZTextMetrics &ZTextEdit::textMetrics() const {
    return _textMetrics;
}

void ZTextEdit::resizeEvent(Tui::ZResizeEvent *event) {
    if (event->size().height() > 0 && event->size().width() > 0) {
        adjustScrollPosition();
    }
}

void ZTextEdit::adjustScrollPosition() {

    if (geometry().width() <= 0 && geometry().height() <= 0) {
        return;
    }

    int newScrollPositionColumn = scrollPositionColumn();
    int newScrollPositionLine = scrollPositionLine();
    int newScrollPositionFineLine = scrollPositionFineLine();

    if (_detachedScrolling) {
        if (newScrollPositionLine >= _doc->lineCount()) {
            newScrollPositionLine = _doc->lineCount() - 1;
        }

        setScrollPosition(newScrollPositionColumn, newScrollPositionLine, newScrollPositionFineLine);
        return;
    }

    const auto [cursorCodeUnit, cursorLine] = _cursor.position();

    int viewWidth = geometry().width() - allBordersWidth();

    // horizontal scroll position
    if (_wrapMode == Tui::ZTextOption::WrapMode::NoWrap) {
        Tui::ZTextLayout layNoWrap = textLayoutForLineWithoutWrapping(cursorLine);
        int cursorColumn = layNoWrap.lineAt(0).cursorToX(cursorCodeUnit, Tui::ZTextLayout::Leading);

        if (cursorColumn - newScrollPositionColumn >= viewWidth) {
             newScrollPositionColumn = cursorColumn - viewWidth + 1;
        }
        if (cursorColumn > 0) {
            if (cursorColumn - newScrollPositionColumn < 1) {
                newScrollPositionColumn = cursorColumn - 1;
            }
        } else {
            newScrollPositionColumn = 0;
        }
    } else {
        newScrollPositionColumn = 0;
    }

    // vertical scroll position
    if (_wrapMode == Tui::ZTextOption::WrapMode::NoWrap) {
        if (cursorLine >= 0) {
            if (cursorLine - newScrollPositionLine < 1) {
                newScrollPositionLine = cursorLine;
                newScrollPositionFineLine = 0;
            }
        }

        if (cursorLine - newScrollPositionLine >= geometry().height() - 1) {
            newScrollPositionLine = cursorLine - geometry().height() + 2;
        }

        if (_doc->lineCount() - newScrollPositionLine < geometry().height() - 1) {
            newScrollPositionLine = std::max(0, _doc->lineCount() - geometry().height() + 1);
        }
    } else {
        Tui::ZTextOption option = textOption();

        const int availableLinesAbove = geometry().height() - 2;

        Tui::ZTextLayout layCursorLayout = textLayoutForLine(option, cursorLine);
        int linesAbove = layCursorLayout.lineForTextPosition(cursorCodeUnit).lineNumber();

        if (linesAbove >= availableLinesAbove) {
            if (newScrollPositionLine < cursorLine) {
                newScrollPositionLine = cursorLine;
                newScrollPositionFineLine = linesAbove - availableLinesAbove;
            }
            if (newScrollPositionLine == cursorLine) {
                if (newScrollPositionFineLine < linesAbove - availableLinesAbove) {
                    newScrollPositionFineLine = linesAbove - availableLinesAbove;
                }
            }
        } else {
            for (int line = cursorLine - 1; line >= 0; line--) {
                Tui::ZTextLayout lay = textLayoutForLine(option, line);
                if (linesAbove + lay.lineCount() >= availableLinesAbove) {
                    if (newScrollPositionLine < line) {
                        newScrollPositionLine = line;
                        newScrollPositionFineLine = (linesAbove + lay.lineCount()) - availableLinesAbove;
                    }
                    if (newScrollPositionLine == line) {
                        if (newScrollPositionFineLine < (linesAbove + lay.lineCount()) - availableLinesAbove) {
                            newScrollPositionFineLine = (linesAbove + lay.lineCount()) - availableLinesAbove;
                        }
                    }
                    break;
                }
                linesAbove += lay.lineCount();
            }
        }

        linesAbove = layCursorLayout.lineForTextPosition(cursorCodeUnit).lineNumber();

        if (newScrollPositionLine == cursorLine) {
            if (linesAbove < newScrollPositionFineLine) {
                newScrollPositionFineLine = linesAbove;
            }
        } else if (newScrollPositionLine > cursorLine) {
            newScrollPositionLine = cursorLine;
            newScrollPositionFineLine = linesAbove;
        }

        // scroll when window is larger than the document shown (unless scrolled to top)
        if (newScrollPositionLine && newScrollPositionLine + (geometry().height() - 1) > _doc->lineCount()) {
            int linesCounted = 0;

            for (int line = _doc->lineCount() - 1; line >= 0; line--) {
                Tui::ZTextLayout lay = textLayoutForLine(option, line);
                linesCounted += lay.lineCount();
                if (linesCounted >= geometry().height() - 1) {
                    if (newScrollPositionLine > line) {
                        newScrollPositionLine = line;
                        newScrollPositionFineLine = linesCounted - (geometry().height() - 1);
                    } else if (newScrollPositionLine == line &&
                               newScrollPositionFineLine > linesCounted - (geometry().height() - 1)) {
                        newScrollPositionFineLine = linesCounted - (geometry().height() - 1);
                    }
                    break;
                }
            }
        }
    }

    setScrollPosition(newScrollPositionColumn, newScrollPositionLine, newScrollPositionFineLine);

    int max = 0;
    for (int i = newScrollPositionLine; i < _doc->lineCount() && i < newScrollPositionLine + geometry().height(); i++) {
        if (max < _doc->lineCodeUnits(i)) {
            max = _doc->lineCodeUnits(i);
        }
    }
    scrollRangeChanged(std::max(0, max - viewWidth), std::max(0, _doc->lineCount() - geometry().height()));
}

int ZTextEdit::allBordersWidth() const {
    return lineNumberBorderWidth();
}

bool ZTextEdit::canPaste() {
    Tui::ZClipboard *clipboard = findFacet<Tui::ZClipboard>();
    if (clipboard) {
        QString _clipboard = clipboard->contents();
        return !_clipboard.isEmpty();
    } else {
        return false;
    }
}

bool ZTextEdit::canCopy() {
    return _cursor.hasSelection();
}

bool ZTextEdit::canCut() {
    return _cursor.hasSelection();
}

void ZTextEdit::updateCommands() {
    if (_cmdCopy) {
        _cmdCopy->setEnabled(canCopy());
    }
    if (_cmdCut) {
        _cmdCut->setEnabled(canCut());
    }
}

void ZTextEdit::updatePasteCommandEnabled() {
    if (_cmdPaste) {
        _cmdPaste->setEnabled(canPaste());
    }
}

void ZTextEdit::clearAdvancedSelection() {
    // derived classes can override this
}

void ZTextEdit::enableDetachedScrolling() {
    _detachedScrolling = true;
}

void ZTextEdit::disableDetachedScrolling() {
    _detachedScrolling = false;
}

bool ZTextEdit::isDetachedScrolling() const {
    return _detachedScrolling;
}

void ZTextEdit::detachedScrollUp() {
    if (_scrollPositionFineLine > 0) {
        enableDetachedScrolling();
        setScrollPosition(_scrollPositionColumn, _scrollPositionLine.line(), _scrollPositionFineLine - 1);
    } else if (_scrollPositionLine.line() > 0) {
        enableDetachedScrolling();

        int newScrollPositionLine = scrollPositionLine() - 1;
        int newScrollPositionFineLine = 0;

        if (wordWrapMode() != Tui::ZTextOption::WrapMode::NoWrap) {
            Tui::ZTextLayout lay = textLayoutForLine(textOption(), newScrollPositionLine);
            newScrollPositionFineLine = lay.lineCount() - 1;
        }
        setScrollPosition(_scrollPositionColumn, newScrollPositionLine, newScrollPositionFineLine);
    }
}

void ZTextEdit::detachedScrollDown() {
    if (wordWrapMode() != Tui::ZTextOption::WrapMode::NoWrap) {
        Tui::ZTextLayout lay = textLayoutForLine(textOption(), _scrollPositionLine.line());
        if (lay.lineCount() - 1 > _scrollPositionFineLine) {
            setScrollPosition(_scrollPositionColumn, _scrollPositionLine.line(), _scrollPositionFineLine + 1);
            return;
        }
    }

    if (document()->lineCount() - 1 > _scrollPositionLine.line()) {
        enableDetachedScrolling();
        setScrollPosition(_scrollPositionColumn, _scrollPositionLine.line() + 1, 0);
    }
}

QPair<int, int> ZTextEdit::getSelectedLinesSort() {
    auto lines = getSelectedLines();
    return {std::min(lines.first, lines.second), std::max(lines.first, lines.second)};
}

QPair<int, int> ZTextEdit::getSelectedLines() {
    int startY;
    int endY;

    const auto [startCodeUnit, startLine] = _cursor.anchor();
    const auto [endCodeUnit, endLine] = _cursor.position();
    startY = startLine;
    endY = endLine;

    if (startLine < endLine) {
        if (endCodeUnit == 0) {
            endY--;
        }
    } else if (endLine < startLine) {
        if (startCodeUnit == 0) {
            startY--;
        }
    }

    return {std::max(0, startY), std::max(0, endY)};
}

void ZTextEdit::selectLines(int startLine, int endLine) {
    if (startLine > endLine) {
        _cursor.setPosition({_doc->lineCodeUnits(startLine), startLine});
        _cursor.setPosition({0, endLine}, true);
    } else {
        _cursor.setPosition({0, startLine});
        _cursor.setPosition({_doc->lineCodeUnits(endLine), endLine}, true);
    }
}
