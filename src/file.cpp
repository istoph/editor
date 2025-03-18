// SPDX-License-Identifier: BSL-1.0

#include "file.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QtConcurrent>

#ifdef SYNTAX_HIGHLIGHTING
#include <KSyntaxHighlighting/Format>
#endif

#include <Tui/Misc/SurrogateEscape.h>
#include <Tui/ZClipboard.h>
#include <Tui/ZCommandNotifier.h>
#include <Tui/ZDocumentCursor.h>
#include <Tui/ZDocumentLineMarker.h>
#include <Tui/ZDocumentSnapshot.h>
#include <Tui/ZImage.h>
#include <Tui/ZPainter.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZSymbol.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTextMetrics.h>

#include "attributes.h"
#include "searchcount.h"

// User Data values for ZFormatRange ranges.
#define FR_UD_SELECTION 1
#define FR_UD_LIVE_SEARCH 2
#define FR_UD_SYNTAX 3

File::File(Tui::ZTextMetrics textMetrics, Tui::ZWidget *parent)
    : ZTextEdit(textMetrics, parent)
{
    _lineMarker = std::make_unique<MarkerManager>();

    setInsertCursorStyle(Tui::CursorStyle::Underline);
    setOverwriteCursorStyle(Tui::CursorStyle::Block);
    setTabChangesFocus(false);
    initText();

    registerCommandNotifiers(Qt::WindowShortcut);

    _cmdSearchNext = new Tui::ZCommandNotifier("Search Next", this, Qt::WindowShortcut);
    QObject::connect(_cmdSearchNext, &Tui::ZCommandNotifier::activated, this, [this] {runSearch(false);});
    _cmdSearchNext->setEnabled(false);
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, Qt::NoModifier), this, Qt::WindowShortcut), &Tui::ZShortcut::activated,
          this, [this] {
            runSearch(false);
          });

    _cmdSearchPrevious = new Tui::ZCommandNotifier("Search Previous", this, Qt::WindowShortcut);
    QObject::connect(_cmdSearchPrevious, &Tui::ZCommandNotifier::activated, this, [this] {runSearch(true);});
    _cmdSearchPrevious->setEnabled(false);
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, Qt::ShiftModifier), this, Qt::WindowShortcut), &Tui::ZShortcut::activated,
          this, [this] {
            runSearch(true);
          });

    QObject::connect(document(), &Tui::ZDocument::lineMarkerChanged, this, [this](const Tui::ZDocumentLineMarker *marker) {
        if ((_blockSelectEndLine && marker == &*_blockSelectEndLine)) {
            // Recalculate the scroll position:
            //  * In case any editing from outside of this class moved our block selection cursor
            // Internal changes trigger these signals as well, that should be safe, as long as adjustScrollPosition
            // does not change the scroll position when called again as long as neither document nor the cursor or
            // block selection cursor is changed inbetween.
            adjustScrollPosition();
        }
        if (_blockSelectEndLine && marker == &*_blockSelectEndLine) {
            // When in block selection mode, the block selection end line marker is what we expose as cursor position
            // line, so send an update out.
            const auto [cursorCodeUnit, cursorLine, cursorColumn] = cursorPositionOrBlockSelectionEnd();
            int utf8PositionX = document()->line(cursorLine).left(cursorCodeUnit).toUtf8().size();
            cursorPositionChanged(cursorColumn, cursorCodeUnit, utf8PositionX, cursorLine);
        }
    });

#ifdef SYNTAX_HIGHLIGHTING
    qRegisterMetaType<Updates>();

    QObject::connect(document(), &Tui::ZDocument::contentsChanged, this, [this] {
        updateSyntaxHighlighting(false);
    });
#endif
}


void File::emitCursorPostionChanged() {
    const auto [cursorCodeUnit, cursorLine, cursorColumn] = cursorPositionOrBlockSelectionEnd();
    int utf8CodeUnit = document()->line(cursorLine).leftRef(cursorCodeUnit).toUtf8().size();
    cursorPositionChanged(cursorColumn, cursorCodeUnit, utf8CodeUnit, cursorLine);

    if (_stdin && document()->lineCount() - 1 == cursorLine) {
        _followMode = true;
        followStandardInputChanged(true);
    } else {
        _followMode = false;
        followStandardInputChanged(false);
    }
}

#ifdef SYNTAX_HIGHLIGHTING

void File::updateSyntaxHighlighting(bool force = false) {
    if (force) {
        document()->setLineUserData(0, nullptr);
    }
    if (!syntaxHighlightingActive() || !_syntaxHighlightDefinition.isValid()
            || !_syntaxHighlightingTheme.isValid()) {
        return;
    }

    Tui::ZDocumentSnapshot snapshot = document()->snapshot();
    SyntaxHighlightingSignalForwarder *forwarder = new SyntaxHighlightingSignalForwarder();
    forwarder->moveToThread(nullptr); // enable later pull to worker thread
    QObject::connect(forwarder, &SyntaxHighlightingSignalForwarder::updates, this, &File::ingestSyntaxHighlightingUpdates);

    QtConcurrent::run([forwarder, snapshot, &highlighter=_syntaxHighlightExporter] {
        forwarder->moveToThread(QThread::currentThread());

        Updates updates;
        updates.documentRevision = snapshot.revision();

        auto sendData = [&] {
            forwarder->updates(updates);
        };

        auto update = [&](int line, std::shared_ptr<ExtraData> &newData) {
            updates.data.append(newData);
            updates.lines.append(line);

            if (updates.data.size() > 100) {
                sendData();
                updates.data.clear();
                updates.lines.clear();
            }
        };

        KSyntaxHighlighting::State state;
        for (int line = 0; line < snapshot.lineCount(); line++) {
            if (!snapshot.isUpToDate()) {
                // Abandon work, the document has changed
                delete forwarder;
                return;
            }
            auto userData = std::static_pointer_cast<const ExtraData>(snapshot.lineUserData(line));
            if (!userData || userData->stateBegin != state || userData->lineRevision != snapshot.lineRevision(line)) {
                auto newData = std::make_shared<ExtraData>();
                newData->stateBegin = state;
                auto res = highlighter.highlightLineWrap(snapshot.line(line), state);
                newData->stateEnd = state = std::get<0>(res);
                newData->highlights = std::get<1>(res);
                newData->lineRevision = snapshot.lineRevision(line);

                update(line, newData);
            } else {
                state = userData->stateEnd;
            }
        }

        if (updates.data.size()) {
            sendData();
        }
        delete forwarder;
    });
}

void File::syntaxHighlightDefinition() {
    if (_syntaxHighlightDefinition.isValid()) {
        _syntaxHighlightExporter.setTheme(_syntaxHighlightingTheme);
        _syntaxHighlightExporter.setDefinition(_syntaxHighlightDefinition);
        _syntaxHighlightExporter.defBg = getColor("chr.editBg");
        _syntaxHighlightExporter.defFg = getColor("chr.editFg");
        _syntaxHighlightingLanguage = _syntaxHighlightDefinition.name();
        syntaxHighlightingLanguageChanged(_syntaxHighlightingLanguage);
    }
}

void File::ingestSyntaxHighlightingUpdates(Updates updates) {
    if (updates.documentRevision != document()->revision()) {
        // Lines numbers might have changed (by insertion or deletion of lines), we can't use this update
        return;
    }

    const int visibleLinesStart = scrollPositionLine();
    const int visibleLinesEnd = visibleLinesStart + geometry().height();

    bool needRepaint = false;

    for (int i = 0; i < updates.lines.size(); i++) {
        const int line = updates.lines[i];

        if (line < document()->lineCount() && document()->lineRevision(line) == updates.data[i]->lineRevision) {
            document()->setLineUserData(line, updates.data[i]);

            if (visibleLinesStart <= line && line <= visibleLinesEnd) {
                needRepaint = true;
            }
        }
    }

    if (needRepaint) {
        update();
    }
}

std::tuple<KSyntaxHighlighting::State, QVector<Tui::ZFormatRange>> HighlightExporter::highlightLineWrap(const QString &text,
                                                                                                        const KSyntaxHighlighting::State &state) {
    std::lock_guard lock{mutex};
    highlights.clear();
    auto newState = highlightLine(text, state);
    return {newState, this->highlights};
}

void HighlightExporter::applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) {
    Tui::ZTextAttributes attr;
    if (format.isBold(theme())) {
        attr |= Tui::ZTextAttribute::Bold;
    }
    if (format.isItalic(theme())) {
        attr |= Tui::ZTextAttribute::Italic;
    }
    if (format.isUnderline(theme())) {
        attr |= Tui::ZTextAttribute::Underline;
    }
    if (format.isStrikeThrough(theme())) {
        attr |= Tui::ZTextAttribute::Strike;
    }
    auto convert = [](QColor q) {
        return Tui::ZColor::fromRgb(q.red(), q.green(), q.blue());
    };
    Tui::ZColor fg = format.hasTextColor(theme()) ? convert(format.textColor(theme())) : defFg;
    Tui::ZColor bg = format.hasBackgroundColor(theme()) ? convert(format.backgroundColor(theme())) : defBg;
    Tui::ZTextStyle style(fg, bg, attr);
    highlights.append(Tui::ZFormatRange(offset, length, style, {Tui::Colors::darkGray, bg}, FR_UD_SYNTAX));
}
#endif

void File::setSyntaxHighlightingTheme(QString themeName) {
#ifdef SYNTAX_HIGHLIGHTING
    _syntaxHighlightingThemeName = themeName;
    _syntaxHighlightingTheme = _syntaxHighlightRepo.theme(_syntaxHighlightingThemeName);
    _syntaxHighlightExporter.setTheme(_syntaxHighlightingTheme);
    _syntaxHighlightExporter.defBg = getColor("chr.editBg");
    _syntaxHighlightExporter.defFg = getColor("chr.editFg");
    for (int line = 0; line < document()->lineCount(); line++) {
        document()->setLineUserData(line, nullptr);
    }
    updateSyntaxHighlighting(true);
#else
    (void)themeName;
#endif
}

void File::setSyntaxHighlightingLanguage(QString language) {
#ifdef SYNTAX_HIGHLIGHTING
    _syntaxHighlightDefinition = _syntaxHighlightRepo.definitionForName(language);
    syntaxHighlightDefinition();
    // rehighlight
    updateSyntaxHighlighting(true);
#else
    (void)language;
#endif
}

QString File::syntaxHighlightingLanguage() {
    return _syntaxHighlightingLanguage;
}

bool File::syntaxHighlightingActive() {
    return _syntaxHighlightingActive;
}

void File::setSyntaxHighlightingActive(bool active) {
#ifdef SYNTAX_HIGHLIGHTING
    _syntaxHighlightingActive = active;
    syntaxHighlightingEnabledChanged(_syntaxHighlightingActive);
    if (active) {
        // rehighlight, highlight have not been updated while syntax highlighting was disabled
        updateSyntaxHighlighting(true);
    }
    update();
#else
    (void)active;
#endif
}

File::~File() {
    if (_searchNextFuture) {
        _searchNextFuture->cancel();
        _searchNextFuture.reset();
    }
    _lineMarker->clearMarkers();
}



int File::convertTabsToSpaces() {
    auto undoGroup = startUndoGroup();

    int count = 0;

    Tui::ZTextOption option = textOption();
    option.setWrapMode(Tui::ZTextOption::NoWrap);

    // This changes code unit based positions in many lines. To avoid glitches reset selection and
    // manually save and restore cursor position. This should no longer be needed when TextCursor
    // is fixed to be able to keep its position even when edits happen.
    clearSelection();
    const auto [cursorCodeUnit, cursorLine] = cursorPosition();
    Tui::ZTextLayout lay = textLayoutForLine(option, cursorLine);
    int cursorPosition = lay.lineAt(0).cursorToX(cursorCodeUnit, Tui::ZTextLayout::Leading);

    Tui::ZDocumentCursor cur = makeCursor();

    for (int line = 0, found = -1; line < document()->lineCount(); line++) {
        found = document()->line(line).lastIndexOf("\t");
        if (found != -1) {
            Tui::ZTextLayout lay = textLayoutForLine(option, line);
            while (found != -1) {
                int columnStart = lay.lineAt(0).cursorToX(found, Tui::ZTextLayout::Leading);
                int columnEnd = lay.lineAt(0).cursorToX(found, Tui::ZTextLayout::Trailing);
                cur.setPosition({found, line});
                cur.moveCharacterRight(true);
                cur.insertText(QString(" ").repeated(columnEnd-columnStart));
                count++;
                found = document()->line(line).lastIndexOf("\t", found);
            }
        }
    }

    // Restore cursor position
    lay = textLayoutForLine(option, cursorLine);
    int newCursorPosition = lay.lineAt(0).xToCursor(cursorPosition);
    if (newCursorPosition - 1 >= 0) {
        setCursorPosition({newCursorPosition, cursorLine});
    }

    return count;
}


void File::setSaveAs(bool state) {
    _saveAs = state;
}
bool File::isSaveAs() {
    return _saveAs;
}


bool File::setFilename(QString filename) {
    QFileInfo filenameInfo(filename);
    if (filenameInfo.isSymLink()) {
        return setFilename(filenameInfo.symLinkTarget());
    }
    document()->setFilename(filenameInfo.absoluteFilePath());
    return true;
}

QString File::getFilename() {
    return document()->filename();
}

bool File::newText(QString filename) {
    initText();
    if (filename == "") {
        document()->setFilename("NEWFILE");
        setSaveAs(true);
    } else {
        setSaveAs(false);
        setFilename(filename);
    }
    modifiedChanged(false);
    return true;
}

bool File::stdinText() {
    document()->setFilename("STDIN");
    _stdin = true;
    initText();
    setFollowStandardInput(true);
    followStandardInputChanged(true);
    modifiedChanged(true);
    setSaveAs(true);
    return true;
}

bool File::initText() {
    clear();
    return true;
}

bool File::saveText() {
    // QSaveFile does not take over the user and group. Therefore this should only be used if
    // user and group are the same and the editor also runs under this user.
    QFile file(getFilename());
    //QSaveFile file(getFilename());
    if (file.open(QIODevice::WriteOnly)) {
        bool ok = writeTo(&file);
        ok &= file.flush();

        //file.commit();
        file.close();
        if (!ok) {
            return false;
        }
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
    writableChanged(getWritable());
}

bool File::getWritable() {
    QFileInfo file(getFilename());
    if (file.isWritable()) {
        return true;
    }
    if (file.isSymLink()) {
        QFileInfo path(file.symLinkTarget());
        if (!file.exists() && path.isWritable()) {
            return true;
        }
        return false;
    }

    QFileInfo path(file.path());
    if (!file.exists() && path.isWritable()) {
        return true;
    }
    return false;
}

void File::setHighlightBracket(bool hb) {
    _bracket = hb;
}

bool File::highlightBracket() {
    return _bracket;
}

bool File::writeAttributes() {
    Attributes a{_attributesFile};
    return a.writeAttributes(getFilename(),
                             cursorPosition(),
                             scrollPositionColumn(), scrollPositionLine(), scrollPositionFineLine(),
                             _lineMarker->listMarker());
}

void File::setAttributesFile(QString attributesFile) {
    _attributesFile = attributesFile;
}

QString File::attributesFile() {
    return _attributesFile;
}

bool File::openText(QString filename) {
    setFilename(filename);
    QFile file(getFilename());
    if (file.open(QIODevice::ReadOnly)) {
        initText();

        Attributes a{_attributesFile};
        Tui::ZDocumentCursor::Position initialPosition = a.getAttributesCursorPosition(getFilename());
        const bool ok = readFrom(&file, initialPosition);
        file.close();

        if (!ok) {
            return false;
        }

        if (getWritable()) {
            setSaveAs(false);
        } else {
            setSaveAs(true);
        }

        checkWritable();

        modifiedChanged(false);

        setScrollPosition(a.getAttributesScrollCol(getFilename()),
                          a.getAttributesScrollLine(getFilename()),
                          a.getAttributesScrollFine(getFilename()));

        QList lm = a.getAttributesLineMarker(getFilename());
        _lineMarker->clearMarkers(); // delete all old markers before adding a new one
        for(int i = 0; i < lm.size(); i++) {
            _lineMarker->addMarker(document(), lm.at(i));
        }

        adjustScrollPosition();

#ifdef SYNTAX_HIGHLIGHTING
        _syntaxHighlightDefinition = _syntaxHighlightRepo.definitionForFileName(getFilename());
        syntaxHighlightDefinition();
#endif

        return true;
    }
    return false;
}


void File::cutline() {
    clearSelection();
    Tui::ZDocumentCursor cursor = textCursor();
    cursor.moveToStartOfLine();
    cursor.moveToEndOfLine(true);
    setTextCursor(cursor);
    cut();
    updateCommands();
    adjustScrollPosition();
}

void File::deleteLine() {
    Tui::ZDocumentCursor cursor = textCursor();
    auto undoGroup = document()->startUndoGroup(&cursor);
    if (cursor.hasSelection() || hasBlockSelection() || hasMultiInsert()) {
        const auto [start, end] = getSelectedLinesSort();
        clearSelection();
        cursor.setPosition({0,end});
        cursor.moveToEndOfLine();
        cursor.setPosition({0,start}, true);
        cursor.removeSelectedText();
    }
    cursor.deleteLine();
    setTextCursor(cursor);
    updateCommands();
    adjustScrollPosition();
}

void File::copy() {
    if (hasBlockSelection()) {
        Tui::ZClipboard *clipboard = findFacet<Tui::ZClipboard>();

        const int firstSelectBlockLine = std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line());
        const int lastSelectBlockLine = std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line());
        const int firstSelectBlockColumn = std::min(_blockSelectStartColumn, _blockSelectEndColumn);
        const int lastSelectBlockColumn = std::max(_blockSelectStartColumn, _blockSelectEndColumn);

        QString selectedText;
        for (int line = firstSelectBlockLine; line < document()->lineCount() && line <= lastSelectBlockLine; line++) {
            Tui::ZTextLayout laySel = textLayoutForLineWithoutWrapping(line);
            Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

            const int selFirstCodeUnitInLine = tlrSel.xToCursor(firstSelectBlockColumn);
            const int selLastCodeUnitInLine = tlrSel.xToCursor(lastSelectBlockColumn);
            selectedText += document()->line(line).mid(selFirstCodeUnitInLine, selLastCodeUnitInLine - selFirstCodeUnitInLine);
            if (line != lastSelectBlockLine) {
                selectedText += "\n";
            }
        }
        clipboard->setContents(selectedText);
    } else if (ZTextEdit::hasSelection()) {
        Tui::ZClipboard *clipboard = findFacet<Tui::ZClipboard>();
        clipboard->setContents(ZTextEdit::selectedText());
        setSelectMode(false);
    }
}

void File::paste() {
    auto undoGroup = startUndoGroup();
    Tui::ZClipboard *clipboard = findFacet<Tui::ZClipboard>();
    if (clipboard->contents().size()) {
        insertText(clipboard->contents());
        adjustScrollPosition();
    }
    document()->clearCollapseUndoStep();
}

void File::gotoLine(QString pos) {
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
}


bool File::formattingCharacters() const {
    return _formattingCharacters;
}
void File::setFormattingCharacters(bool formattingCharacters) {
    _formattingCharacters = formattingCharacters;
    update();
}

void File::setRightMarginHint(int hint) {
    if (hint < 1) {
        _rightMarginHint = 0;
    } else {
        _rightMarginHint = hint;
    }
    update();
}

int File::rightMarginHint() const {
    return _rightMarginHint;
}

bool File::colorTabs() const {
    return _colorTabs;
}
void File::setColorTabs(bool colorTabs) {
    _colorTabs = colorTabs;
    update();
}

bool File::colorTrailingSpaces() {
    return _colorTrailingSpaces;
}
void File::setColorTrailingSpaces(bool colorSpaceEnd) {
    _colorTrailingSpaces = colorSpaceEnd;
    update();
}

void File::setEatSpaceBeforeTabs(bool eat) {
    _eatSpaceBeforeTabs = eat;
}

bool File::eatSpaceBeforeTabs() {
    return _eatSpaceBeforeTabs;
}

QPair<int,int> File::getSelectedLinesSort() {
    auto lines = getSelectedLines();
    return {std::min(lines.first, lines.second), std::max(lines.first, lines.second)};
}

QPair<int,int> File::getSelectedLines() {
    int startY;
    int endeY;

    if (hasBlockSelection() || hasMultiInsert()) {
        startY = _blockSelectStartLine->line();
        endeY = _blockSelectEndLine->line();
    } else {
        const auto [startCodeUnit, startLine] = anchorPosition();
        const auto [endCodeUnit, endLine] = cursorPosition();
        startY = startLine;
        endeY = endLine;

        if (startLine < endLine) {
            if (endCodeUnit == 0) {
                endeY--;
            }
        } else if (endLine < startLine) {
            if (startCodeUnit == 0) {
                startY--;
            }
        }
    }

    return {std::max(0, startY), std::max(0, endeY)};
}

void File::selectLines(int startY, int endY) {
    clearSelection();
    if (startY > endY) {
        setSelection({document()->lineCodeUnits(startY), startY},
                     {0, endY});
    } else {
        setSelection({0, startY},
                     {document()->lineCodeUnits(endY), endY});
    }
}

void File::clearAdvancedSelection() {
    if (_currentSearchMatch) {
        _currentSearchMatch.reset();
    }

    if (_blockSelect) {
        disableBlockSelection();
    }
}

bool File::canCut() {
    return hasBlockSelection() || ZTextEdit::hasSelection();
}

bool File::canCopy() {
    return hasBlockSelection() || ZTextEdit::hasSelection();
}


QString File::selectedText() {
    QString selectText = "";
    if (hasBlockSelection()) {
        const int firstSelectBlockLine = std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line());
        const int lastSelectBlockLine = std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line());
        const int firstSelectBlockColumn = std::min(_blockSelectStartColumn, _blockSelectEndColumn);
        const int lastSelectBlockColumn = std::max(_blockSelectStartColumn, _blockSelectEndColumn);

        for (int line = firstSelectBlockLine; line < document()->lineCount() && line <= lastSelectBlockLine; line++) {
            Tui::ZTextLayout laySel = textLayoutForLineWithoutWrapping(line);
            Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

            const int selFirstCodeUnitInLine = tlrSel.xToCursor(firstSelectBlockColumn);
            const int selLastCodeUnitInLine = tlrSel.xToCursor(lastSelectBlockColumn);
            selectText += document()->line(line).mid(selFirstCodeUnitInLine, selLastCodeUnitInLine - selFirstCodeUnitInLine);
            if (line != lastSelectBlockLine) {
                selectText += "\n";
            }
        }
    } else {
        selectText = ZTextEdit::selectedText();
    }
    return selectText;
}

bool File::hasSelection() const {
    return hasBlockSelection() || hasMultiInsert() || ZTextEdit::hasSelection();
}

bool File::removeSelectedText() {
    Tui::ZDocumentCursor cursor = textCursor();
    auto undoGroup = document()->startUndoGroup(&cursor);

    if (!hasBlockSelection() && !hasMultiInsert() && !ZTextEdit::hasSelection()) {
        return false;
    }

    if (_blockSelect) {
        if (hasBlockSelection()) {
            blockSelectRemoveSelectedAndConvertToMultiInsert();
        }

        const int newCursorLine = _blockSelectEndLine->line();
        const int newCursorColumn = _blockSelectStartColumn;

        Tui::ZTextLayout lay = textLayoutForLineWithoutWrapping(newCursorLine);
        Tui::ZTextLineRef tlr = lay.lineAt(0);

        _blockSelect = false;
        _blockSelectStartLine.reset();
        _blockSelectEndLine.reset();
        _blockSelectStartColumn = _blockSelectEndColumn = -1;

        setCursorPosition({tlr.xToCursor(newCursorColumn), newCursorLine});
    } else {
        cursor.removeSelectedText();
        setTextCursor(cursor);
    }
    adjustScrollPosition();
    updateCommands();
    return true;
}

bool File::hasBlockSelection() const {
    return _blockSelect && _blockSelectStartColumn != _blockSelectEndColumn;
}

bool File::hasMultiInsert() const {
    return _blockSelect && _blockSelectStartColumn == _blockSelectEndColumn;
}

void File::activateBlockSelection() {
    // pre-condition: _blockSelect = false
    _blockSelect = true;

    const auto [cursorCodeUnit, cursorLine] = cursorPosition();
    _blockSelectStartLine.emplace(document(), cursorLine);
    _blockSelectEndLine.emplace(document(), cursorLine);

    Tui::ZTextLayout lay = textLayoutForLineWithoutWrapping(cursorLine);
    Tui::ZTextLineRef tlr = lay.lineAt(0);

    _blockSelectStartColumn = _blockSelectEndColumn = tlr.cursorToX(cursorCodeUnit, Tui::ZTextLayout::Leading);
}

void File::disableBlockSelection() {
    // pre-condition: _blockSelect = true
    _blockSelect = false;

    // just to be sure
    const int cursorLine = std::min(_blockSelectEndLine->line(), document()->lineCount() - 1);
    const int cursorColumn = _blockSelectEndColumn;

    _blockSelectStartLine.reset();
    _blockSelectEndLine.reset();
    _blockSelectStartColumn = _blockSelectEndColumn = -1;

    Tui::ZTextLayout lay = textLayoutForLineWithoutWrapping(cursorLine);
    Tui::ZTextLineRef tlr = lay.lineAt(0);
    setCursorPosition({tlr.xToCursor(cursorColumn), cursorLine});
}

void File::blockSelectRemoveSelectedAndConvertToMultiInsert() {
    const int firstSelectBlockLine = std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line());
    const int lastSelectBlockLine = std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line());
    const int firstSelectBlockColumn = std::min(_blockSelectStartColumn, _blockSelectEndColumn);
    const int lastSelectBlockColumn = std::max(_blockSelectStartColumn, _blockSelectEndColumn);

    for (int line = firstSelectBlockLine; line < document()->lineCount() && line <= lastSelectBlockLine; line++) {
        Tui::ZTextLayout laySel = textLayoutForLineWithoutWrapping(line);
        Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

        const int selFirstCodeUnitInLine = tlrSel.xToCursor(firstSelectBlockColumn);
        const int selLastCodeUnitInLine = tlrSel.xToCursor(lastSelectBlockColumn);

        Tui::ZDocumentCursor cur = makeCursor();
        cur.setPosition({selFirstCodeUnitInLine, line});
        cur.setPosition({selLastCodeUnitInLine, line}, true);
        cur.removeSelectedText();
    }

    _blockSelectStartColumn = _blockSelectEndColumn = firstSelectBlockColumn;
}

template<typename F>
void File::multiInsertForEachCursor(int flags, F f) {
    // pre-condition: hasMultiInsert() = true

    const int firstSelectBlockLine = std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line());
    const int lastSelectBlockLine = std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line());
    const int column = _blockSelectStartColumn;

    bool endSkipped = false;
    int fallbackColumn = _blockSelectStartColumn;

    for (int line = firstSelectBlockLine; line < document()->lineCount() && line <= lastSelectBlockLine; line++) {
        Tui::ZTextLayout laySel = textLayoutForLineWithoutWrapping(line);
        Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

        const int codeUnitInLine = tlrSel.xToCursor(column);

        Tui::ZDocumentCursor cur = makeCursor();
        cur.setPosition({codeUnitInLine, line});

        bool skip = false;
        if (tlrSel.width() < column) {
            if ((flags & mi_add_spaces)) {
                cur.insertText(QString(" ").repeated(column - tlrSel.width()));
            } else if (flags & mi_skip_short_lines) {
                skip = true;
            }
        }

        if (!skip) {
            f(cur);

            Tui::ZTextLayout layNew = textLayoutForLineWithoutWrapping(line);
            Tui::ZTextLineRef tlrNew = layNew.lineAt(0);
            fallbackColumn = tlrNew.cursorToX(cur.position().codeUnit, Tui::ZTextLayout::Leading);

            if (line == _blockSelectEndLine->line()) {
                _blockSelectStartColumn = _blockSelectEndColumn = fallbackColumn;
            }
        } else {
            if (line == _blockSelectEndLine->line()) {
                endSkipped = true;
            }
        }
    }

    if (endSkipped) {
        _blockSelectStartColumn = _blockSelectEndColumn = fallbackColumn;
    }
}

void File::multiInsertDeletePreviousCharacter() {
    // pre-condition: hasMultiInsert() = true
    multiInsertForEachCursor(mi_skip_short_lines, [&](Tui::ZDocumentCursor &cur) {
        const auto [cursorCodeUnit, cursorLine] = cur.position();
        if (cursorCodeUnit > 0) {
            cur.deletePreviousCharacter();
        }
    });
}

void File::multiInsertDeletePreviousWord() {
    // pre-condition: hasMultiInsert() = true
    multiInsertForEachCursor(mi_skip_short_lines, [&](Tui::ZDocumentCursor &cur) {
        const auto [cursorCodeUnit, cursorLine] = cur.position();
        if (cursorCodeUnit > 0) {
            cur.deletePreviousWord();
        }
    });
}

void File::multiInsertDeleteCharacter() {
    // pre-condition: hasMultiInsert() = true
    multiInsertForEachCursor(mi_skip_short_lines, [&](Tui::ZDocumentCursor &cur) {
        const auto [cursorCodeUnit, cursorLine] = cur.position();
        if (cursorCodeUnit < document()->lineCodeUnits(cursorLine)) {
            cur.deleteCharacter();
        }
    });
}

void File::multiInsertDeleteWord() {
    // pre-condition: hasMultiInsert() = true
    multiInsertForEachCursor(mi_skip_short_lines, [&](Tui::ZDocumentCursor &cur) {
        const auto [cursorCodeUnit, cursorLine] = cur.position();
        if (cursorCodeUnit < document()->lineCodeUnits(cursorLine)) {
            cur.deleteWord();
        }
    });
}

void File::multiInsertInsert(const QString &text) {
    // pre-condition: hasMultiInsert() = true
    multiInsertForEachCursor(mi_add_spaces, [&](Tui::ZDocumentCursor &cur) {
        cur.insertText(text);
    });
}

void File::toggleLineMarker() {
    if (_lineMarker->hasMarker(cursorPosition().line)) {
        _lineMarker->removeMarker(cursorPosition().line);
    } else {
        _lineMarker->addMarker(document(), cursorPosition().line);
    }
    update();
}

void File::gotoNextLineMarker() {
    int line = _lineMarker->nextMarker(cursorPosition().line);
    if (line >= 0) {
        setCursorPosition({0, line});
    }
}

void File::gotoPreviousLineMarker() {
    int line = _lineMarker->previousMarker(cursorPosition().line);
    if (line >= 0) {
        setCursorPosition({0, line});
    }
}

bool File::hasLineMarker() const {
    return _lineMarker->hasMarker();
}

bool File::hasLineMarker(int line) const {
    return _lineMarker->hasMarker(line);
}

int File::lineMarkerBorderWidth() const {
    if (hasLineMarker()) {
        if (lineNumberBorderWidth() > 0) {
            return 1;
        }
    } else {
        return 0;
    }
    return 2;
}

int File::allBordersWidth() const {
    return lineNumberBorderWidth() + lineMarkerBorderWidth();
}

void File::setSearchText(QString searchText) {
    int gen = ++(*searchGeneration);
    _searchText = searchText;
    searchTextChanged(_searchText);

    if (searchText == "") {
        _cmdSearchNext->setEnabled(false);
        _cmdSearchPrevious->setEnabled(false);
        setSearchVisible(false);
        return;
    } else {
        _cmdSearchNext->setEnabled(true);
        _cmdSearchPrevious->setEnabled(true);
        setSearchVisible(true);
    }

    if (_searchRegex || _searchText.contains('\n')) {
        // SearchCount currently does not support regular expression and does not support multi line matches,
        // just disable the search count display in these cases for now.
        searchCountChanged(-1);
    } else {
        SearchCountSignalForwarder *searchCountSignalForwarder = new SearchCountSignalForwarder();
        QObject::connect(searchCountSignalForwarder, &SearchCountSignalForwarder::searchCount, this, &File::searchCountChanged);

        QtConcurrent::run([searchCountSignalForwarder](Tui::ZDocumentSnapshot snap, QString searchText, Qt::CaseSensitivity caseSensitivity, int gen, std::shared_ptr<std::atomic<int>> searchGen) {
            SearchCount sc;
            QObject::connect(&sc, &SearchCount::searchCount, searchCountSignalForwarder, &SearchCountSignalForwarder::searchCount);
            sc.run(snap, searchText, caseSensitivity, gen, searchGen);
            searchCountSignalForwarder->deleteLater();
        }, document()->snapshot(), _searchText, _searchCaseSensitivity, gen, searchGeneration);
    }
}

void File::setSearchCaseSensitivity(Qt::CaseSensitivity searchCaseSensitivity) {
    _searchCaseSensitivity = searchCaseSensitivity;
    update();
}

void File::setSearchVisible(bool visible) {
    _searchVisible = visible;
    searchVisibleChanged(visible);
    update();
}

bool File::searchVisible() {
    return _searchVisible;
}

void File::setReplaceText(QString replaceText) {
    _replaceText = replaceText;
}

bool File::isSearchMatchSelected() {
    return _currentSearchMatch.has_value();
}

void File::setRegex(bool reg) {
    _searchRegex = reg;
}
void File::setSearchWrap(bool wrap) {
    _searchWrap = wrap;
    update();
}

bool File::searchWrap() {
    return _searchWrap;
}

void File::setSearchDirection(bool searchDirection) {
    _searchDirectionForward = searchDirection;
}

void File::toggleShowLineNumbers() {
    setShowLineNumbers(!showLineNumbers());
}

void File::setFollowStandardInput(bool follow) {
    _followMode = follow;
}

void File::replaceSelected() {
    if (!_currentSearchMatch || hasBlockSelection() || hasMultiInsert()) {
        return;
    }

    if (ZTextEdit::hasSelection()) {
        QString text;

        if (_searchRegex) {
            bool esc = false;
            for (QChar ch: _replaceText) {
                if (esc) {
                    if (ch >= '1' && ch <= '9') {
                        const int captureNumber = (ch.unicode() - '0');
                        if (std::holds_alternative<Tui::ZDocumentFindAsyncResult>(*_currentSearchMatch)) {
                            text += std::get<Tui::ZDocumentFindAsyncResult>(*_currentSearchMatch).regexCapture(captureNumber);
                        } else if (std::holds_alternative<Tui::ZDocumentFindResult>(*_currentSearchMatch)) {
                            text += std::get<Tui::ZDocumentFindResult>(*_currentSearchMatch).regexCapture(captureNumber);
                        }
                    } else if (ch == '\\') {
                        text += '\\';
                    }
                    esc = false;
                } else {
                    if (ch == '\\') {
                        esc = true;
                    } else {
                        text += ch;
                    }
                }
            }
        } else {
            text = _replaceText;
        }

        Tui::ZDocumentCursor cursor = textCursor();
        auto undoGroup = document()->startUndoGroup(&cursor);

        cursor.insertText(text);
        setTextCursor(cursor);

        adjustScrollPosition();
    }
}

void File::runSearch(bool direction) {
    if (_searchText != "") {
        setSearchVisible(true);
        if (_searchNextFuture) {
            _searchNextFuture->cancel();
            _searchNextFuture.reset();
        }

        const bool effectiveDirection = direction ^ _searchDirectionForward;

        Tui::ZDocument::FindFlags flags;
        if (_searchCaseSensitivity == Qt::CaseSensitive) {
            flags |= Tui::ZDocument::FindFlag::FindCaseSensitively;
        }
        if (_searchWrap) {
            flags |= Tui::ZDocument::FindFlag::FindWrap;
        }
        if (!effectiveDirection) {
            flags |= Tui::ZDocument::FindFlag::FindBackward;
        }

        auto watcher = new QFutureWatcher<Tui::ZDocumentFindAsyncResult>();

        QObject::connect(watcher, &QFutureWatcher<Tui::ZDocumentFindAsyncResult>::finished, this,
                         [this, watcher, effectiveDirection] {
            if (!watcher->isCanceled()) {
                Tui::ZDocumentFindAsyncResult res = watcher->future().result();
                if (res.anchor() != res.cursor()) { // has a match?
                    clearAdvancedSelection();

                    if (selectMode()) {
                        if (effectiveDirection) {
                            setCursorPosition(res.cursor(), true);
                        } else {
                            setCursorPosition(res.anchor(), true);
                        }
                    } else {
                        setSelection(res.anchor(), res.cursor());
                    }
                    //TODO:
                    //res.wrapped();

                    _currentSearchMatch.emplace(res);

                    updateCommands();

                    const auto [currentCodeUnit, currentLine] = cursorPosition();

                    setScrollPosition(scrollPositionColumn(), std::max(0, currentLine - 1), 0);
                    adjustScrollPosition();
                }
            }
            watcher->deleteLater();
        });

        if (_searchRegex) {
            _searchNextFuture.emplace(document()->findAsync(QRegularExpression(_searchText), textCursor(), flags));
        } else {
            _searchNextFuture.emplace(document()->findAsync(_searchText, textCursor(), flags));
        }
        watcher->setFuture(*_searchNextFuture);
    }
}

int File::replaceAll(QString searchText, QString replaceText) {
    setSearchText(searchText);
    setReplaceText(replaceText);

    // Get rid of block selections and multi insert.
    clearSelection();

    if (searchText.isEmpty()) {
        return 0;
    }

    Tui::ZDocumentCursor cursor = textCursor();
    auto undoGroup = document()->startUndoGroup(&cursor);
    int counter = 0;

    Tui::ZDocument::FindFlags flags;
    if (_searchCaseSensitivity == Qt::CaseSensitive) {
        flags |= Tui::ZDocument::FindFlag::FindCaseSensitively;
    }

    cursor.setPosition({0, 0});
    while (true) {
        Tui::ZDocumentCursor found = cursor;
        auto match = _currentSearchMatch;
        if (_searchRegex) {
            Tui::ZDocumentFindResult details = document()->findSyncWithDetails(QRegularExpression(_searchText),
                                                                               cursor, flags);
            match = details;
            found = details.cursor();
        } else {
            found = document()->findSync(_searchText, cursor, flags);
            match = std::monostate();
        }
        if (!found.hasSelection()) {  // has no match?
            break;
        }

        setSelection(found.anchor(), found.position());
        _currentSearchMatch = match;

        replaceSelected();
        cursor = textCursor();
        counter++;
    }

    const auto [currentCodeUnit, currentLine] = cursorPosition();
    if (currentLine - 1 > 0) {
        setScrollPosition(scrollPositionColumn(), currentLine - 1, 0);
    }

    adjustScrollPosition();
    // Update search count
    setSearchText(searchText);
    return counter;
}

Tui::ZTextOption File::textOption() const {
    Tui::ZTextOption option;
    option.setWrapMode(wordWrapMode());
    option.setTabStopDistance(tabStopDistance());

    Tui::ZTextOption::Flags flags;
    if (formattingCharacters()) {
        flags |= Tui::ZTextOption::ShowTabsAndSpaces;
    }
    if (colorTabs()) {
        flags |= Tui::ZTextOption::ShowTabsAndSpacesWithColors;
        if (!useTabChar()) {
            option.setTabColor([] (int pos, int size, int hidden, const Tui::ZTextStyle &base, const Tui::ZTextStyle &formating, const Tui::ZFormatRange* range) -> Tui::ZTextStyle {
                (void)formating;
                (void)size;
                if (range && range->userData() == FR_UD_SELECTION) {
                    if (pos == hidden) {
                        return { range->format().foregroundColor(), {0xff, 0x80, 0xff} };
                    }
                    return { range->format().foregroundColor(), {0xff, 0xb0, 0xff} };
                }
                if (range && range->userData() == FR_UD_LIVE_SEARCH) {
                    return {Tui::Colors::darkGray, {0xff, 0xdd, 0}, Tui::ZTextAttribute::Bold};
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
            option.setTabColor([] (int pos, int size, int hidden, const Tui::ZTextStyle &base, const Tui::ZTextStyle &formating, const Tui::ZFormatRange* range) -> Tui::ZTextStyle {
                (void)formating;
                (void)size;
                if (range && range->userData() == FR_UD_SELECTION) {
                    if (pos == hidden) {
                        return { range->format().foregroundColor(), {0x80, 0xff, 0xff} };
                    }
                    return { range->format().foregroundColor(), {0xb0, 0xff, 0xff} };
                }
                if (range && range->userData() == FR_UD_LIVE_SEARCH) {
                    return {Tui::Colors::darkGray, {0xff, 0xdd, 0}, Tui::ZTextAttribute::Bold};
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
    option.setFlags(flags);
    return option;
}


bool File::highlightBracketFind() {
    QString openBracket = "{[(<";
    QString closeBracket = "}])>";

    if (highlightBracket()) {
        const auto [cursorCodeUnit, cursorLine] = cursorPosition();

        if (cursorCodeUnit < document()->lineCodeUnits(cursorLine)) {
            for (int i = 0; i < openBracket.size(); i++) {
                if (document()->line(cursorLine)[cursorCodeUnit] == openBracket[i]) {
                    int y = 0;
                    int counter = 0;
                    int startX = cursorCodeUnit + 1;
                    for (int line = cursorLine; y++ < rect().height() && line < document()->lineCount(); line++) {
                        for (; startX < document()->lineCodeUnits(line); startX++) {
                            if (document()->line(line)[startX] == openBracket[i]) {
                                counter++;
                            } else if (document()->line(line)[startX] == closeBracket[i]) {
                                if (counter > 0) {
                                    counter--;
                                } else {
                                    _bracketPosition.line = line;
                                    _bracketPosition.codeUnit = startX;
                                    return true;
                                }
                            }
                        }
                        startX = 0;
                    }
                }

                if (document()->line(cursorLine)[cursorCodeUnit] == closeBracket[i]) {
                    int counter = 0;
                    int startX = cursorCodeUnit - 1;
                    for (int line = cursorLine; line >= 0;) {
                        for (; startX >= 0; startX--) {
                            if (document()->line(line)[startX] == closeBracket[i]) {
                                counter++;
                            } else if (document()->line(line)[startX] == openBracket[i]) {
                                if (counter > 0) {
                                    counter--;
                                } else {
                                    _bracketPosition.line = line;
                                    _bracketPosition.codeUnit = startX;
                                    return true;
                                }
                            }
                        }
                        if(--line >= 0) {
                            startX = document()->lineCodeUnits(line) - 1;
                        }
                    }
                }
            }
        }
    }
    _bracketPosition.line = -1;
    _bracketPosition.codeUnit = -1;
    return false;
}

void File::paintEvent(Tui::ZPaintEvent *event) {
    Tui::ZColor fg = getColor("chr.editFg");
    Tui::ZColor bg = getColor("chr.editBg");

    setCursorColor(fg.redOrGuess(), fg.greenOrGuess(), fg.blueOrGuess());

    highlightBracketFind();

    Tui::ZColor marginMarkBg = [](Tui::ZColorHSV base)
        {
            return Tui::ZColor::fromHsv(base.hue(), base.saturation(), base.value() * 0.75);
        }(bg.toHsv());


    std::optional<Tui::ZImage> leftOfMarginBuffer;
    std::optional<Tui::ZPainter> painterLeftOfMargin;

    auto scrollPositionColumns = scrollPositionColumn();

    auto *painter = event->painter();
    if (_rightMarginHint) {
        painter->clearRect(0, 0, -scrollPositionColumns + lineNumberBorderWidth() + lineMarkerBorderWidth() + _rightMarginHint, rect().height(), fg, bg);
        painter->clearRect(-scrollPositionColumns + lineNumberBorderWidth() + lineMarkerBorderWidth() + _rightMarginHint, 0, Tui::tuiMaxSize, rect().height(), fg, marginMarkBg);

        // One extra column to account for double wide character at last position
        leftOfMarginBuffer.emplace(terminal(), _rightMarginHint + 1, 1);
        painterLeftOfMargin.emplace(leftOfMarginBuffer->painter());
    } else {
        painter->clear(fg, bg);
    }

    Tui::ZTextOption option = textOption();
    Tui::ZTextOption optionCursorAtEndOfLine = option;
    if (colorTrailingSpaces()) {
        option.setFlags(optionCursorAtEndOfLine.flags() | Tui::ZTextOption::ShowTabsAndSpacesWithColors);
        option.setTrailingWhitespaceColor([] (const Tui::ZTextStyle &base, const Tui::ZTextStyle &formating, const Tui::ZFormatRange* range) -> Tui::ZTextStyle {
            (void)formating;
            if (range && range->userData() == FR_UD_SELECTION) {
                return { range->format().foregroundColor(), {0xff, 0x80, 0x80} };
            }
            return { base.foregroundColor(), {0x80, 0, 0} };
        });
    }

    const int firstSelectBlockLine = _blockSelect ? std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line()) : 0;
    const int lastSelectBlockLine = _blockSelect ? std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line()) : 0;
    const int firstSelectBlockColumn = std::min(_blockSelectStartColumn, _blockSelectEndColumn);
    const int lastSelectBlockColumn = std::max(_blockSelectStartColumn, _blockSelectEndColumn);

    Tui::ZDocumentCursor::Position startSelectCursor(-1, -1);
    Tui::ZDocumentCursor::Position endSelectCursor(-1, -1);

    Tui::ZDocumentCursor cursor = textCursor();

    if (cursor.hasSelection()) {
        startSelectCursor = cursor.selectionStartPos();
        endSelectCursor = cursor.selectionEndPos();
    }

    QVector<Tui::ZFormatRange> highlights;
    const Tui::ZTextStyle base{fg, bg};
    const Tui::ZTextStyle baseInMargin{fg, marginMarkBg};
    const Tui::ZTextStyle formatingChar{Tui::Colors::darkGray, bg};
    const Tui::ZTextStyle formatingCharInMargin{Tui::Colors::darkGray, marginMarkBg};
    const Tui::ZTextStyle selected{Tui::Colors::darkGray, fg, Tui::ZTextAttribute::Bold};
    const Tui::ZTextStyle multiInsertChar{fg, Tui::Colors::lightGray, Tui::ZTextAttribute::Blink | Tui::ZTextAttribute::Italic};
    const Tui::ZTextStyle multiInsertFormatingChar{Tui::Colors::darkGray, Tui::Colors::lightGray, Tui::ZTextAttribute::Blink};
    const Tui::ZTextStyle selectedFormatingChar{Tui::Colors::darkGray, fg};

    const auto [cursorCodeUnit, cursorLineReal] = cursor.position();
    const int cursorLine = _blockSelect ? _blockSelectEndLine->line() : cursorLineReal;

    QString strlinenumber;
    int y = -scrollPositionFineLine();
    int tmpLastLineWidth = 0;
    for (int line = scrollPositionLine(); y < rect().height() && line < document()->lineCount(); line++) {
        const bool multiIns = hasMultiInsert();
        const bool cursorAtEndOfCurrentLine = [&, cursorCodeUnit=cursorCodeUnit] {
            if (_blockSelect) {
                if (multiIns && firstSelectBlockLine <= line && line <= lastSelectBlockLine) {
                    auto testLayout = textLayoutForLineWithoutWrapping(line);
                    auto testLayoutLine = testLayout.lineAt(0);
                    return testLayoutLine.width() == firstSelectBlockColumn;
                }
                return false;
            } else {
                return line == cursorLine && document()->lineCodeUnits(cursorLine) == cursorCodeUnit;
            }
        }();
        Tui::ZTextLayout lay = cursorAtEndOfCurrentLine ? textLayoutForLine(optionCursorAtEndOfLine, line)
                                                        : textLayoutForLine(option, line);

        // highlights
        highlights.clear();

#ifdef SYNTAX_HIGHLIGHTING
        if (syntaxHighlightingActive()) {
            if (document()->lineUserData(line)) {
                auto extraData = std::static_pointer_cast<const ExtraData>(document()->lineUserData(line));
                if (line == cursorLine && extraData->lineRevision != document()->lineRevision(line)) {
                    // avoid glitches when using the cursor to edit lines
                    // the state can still be stale, but much more edits can be done without visible glitches
                    // with stale state.
                    highlights += std::get<1>(_syntaxHighlightExporter.highlightLineWrap(document()->line(line), extraData->stateBegin));
                } else {
                    highlights += extraData->highlights;
                }
            }
        }
#endif

        // search matches
        if (searchVisible() && _searchText != "") {
            int found = -1;
            if (_searchRegex) {
                QRegularExpression rx(_searchText);
                if (rx.isValid()) {
                    if (_searchCaseSensitivity == Qt::CaseInsensitive) {
                        rx.setPatternOptions(QRegularExpression::PatternOption::CaseInsensitiveOption);
                    }
                    QRegularExpressionMatchIterator i = rx.globalMatch(document()->line(line));
                    while (i.hasNext()) {
                        QRegularExpressionMatch match = i.next();
                        if (match.capturedLength() > 0) {
                            highlights.append(Tui::ZFormatRange{match.capturedStart(), match.capturedLength(),
                                                                {Tui::Colors::darkGray, {0xff, 0xdd, 0}, Tui::ZTextAttribute::Bold},
                                                                selectedFormatingChar,
                                                                FR_UD_LIVE_SEARCH});
                        }
                    }
                }
            } else {
                while ((found = document()->line(line).indexOf(_searchText, found + 1, _searchCaseSensitivity)) != -1) {
                    highlights.append(Tui::ZFormatRange{found, _searchText.size(),
                                                        {Tui::Colors::darkGray, {0xff, 0xdd, 0}, Tui::ZTextAttribute::Bold},
                                                        selectedFormatingChar,
                                                        FR_UD_LIVE_SEARCH});
                }
            }
        }
        if (_bracketPosition.codeUnit >= 0) {
            if (_bracketPosition.line == line) {
                highlights.append(Tui::ZFormatRange{_bracketPosition.codeUnit, 1, {Tui::Colors::cyan, bg,Tui::ZTextAttribute::Bold}, selectedFormatingChar});
            }
            if (line == cursorLine) {
                highlights.append(Tui::ZFormatRange{cursorCodeUnit, 1, {Tui::Colors::cyan, bg,Tui::ZTextAttribute::Bold}, selectedFormatingChar});
            }
        }

        // selection
        if (_blockSelect) {
            if (line >= firstSelectBlockLine && line <= lastSelectBlockLine) {
                Tui::ZTextLayout laySel = textLayoutForLineWithoutWrapping(line);
                Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

                if (firstSelectBlockColumn == lastSelectBlockColumn) {
                    highlights.append(Tui::ZFormatRange{tlrSel.xToCursor(firstSelectBlockColumn), 1,
                                                        multiInsertChar, multiInsertFormatingChar, FR_UD_SELECTION});
                } else {
                    const int selFirstCodeUnitInLine = tlrSel.xToCursor(firstSelectBlockColumn);
                    const int selLastCodeUnitInLine = tlrSel.xToCursor(lastSelectBlockColumn);
                    highlights.append(Tui::ZFormatRange{selFirstCodeUnitInLine, selLastCodeUnitInLine - selFirstCodeUnitInLine,
                                                        selected, selectedFormatingChar, FR_UD_SELECTION});
                }
            }
        } else {
            if (line > startSelectCursor.line && line < endSelectCursor.line) {
                // whole line
                highlights.append(Tui::ZFormatRange{0, document()->lineCodeUnits(line),
                                                    selected, selectedFormatingChar, FR_UD_SELECTION});
            } else if (line > startSelectCursor.line && line == endSelectCursor.line) {
                // selection ends on this line
                highlights.append(Tui::ZFormatRange{0, endSelectCursor.codeUnit,
                                                    selected, selectedFormatingChar, FR_UD_SELECTION});
            } else if (line == startSelectCursor.line && line < endSelectCursor.line) {
                // selection starts on this line
                highlights.append(Tui::ZFormatRange{startSelectCursor.codeUnit,
                                                    document()->lineCodeUnits(line) - startSelectCursor.codeUnit,
                                                    selected, selectedFormatingChar, FR_UD_SELECTION});
            } else if (line == startSelectCursor.line && line == endSelectCursor.line) {
                // selection is contained in this line
                highlights.append(Tui::ZFormatRange{startSelectCursor.codeUnit,
                                                    endSelectCursor.codeUnit - startSelectCursor.codeUnit,
                                                    selected, selectedFormatingChar, FR_UD_SELECTION});
            }
        }
        if (_rightMarginHint && lay.maximumWidth() > _rightMarginHint) {
            if (lay.lineCount() > leftOfMarginBuffer->height() && leftOfMarginBuffer->height() < rect().height()) {
                leftOfMarginBuffer.emplace(terminal(), _rightMarginHint + 1,
                                           std::min(std::max(leftOfMarginBuffer->height() * 2, lay.lineCount()), rect().height()));
                painterLeftOfMargin.emplace(leftOfMarginBuffer->painter());
            }

            lay.draw(*painter, {-scrollPositionColumns + lineNumberBorderWidth() + lineMarkerBorderWidth(), y}, baseInMargin, &formatingCharInMargin, highlights);
            painterLeftOfMargin->clearRect(0, 0, _rightMarginHint + 1, lay.lineCount(), base.foregroundColor(), base.backgroundColor());
            lay.draw(*painterLeftOfMargin, {0, 0}, base, &formatingChar, highlights);
            painter->drawImageWithTiling(-scrollPositionColumns + lineNumberBorderWidth() + lineMarkerBorderWidth(), y,
                                              *leftOfMarginBuffer, 0, 0, _rightMarginHint, lay.lineCount(),
                                              Tui::ZTilingMode::NoTiling, Tui::ZTilingMode::Put);
        } else {
            if (_formattingCharacters) {
                lay.draw(*painter, {-scrollPositionColumns + lineNumberBorderWidth() + lineMarkerBorderWidth(), y}, base, &formatingChar, highlights);
            } else {
                lay.draw(*painter, {-scrollPositionColumns + lineNumberBorderWidth() + lineMarkerBorderWidth(), y}, base, &base, highlights);
            }
        }
        Tui::ZTextLineRef lastLine = lay.lineAt(lay.lineCount()-1);
        tmpLastLineWidth = lastLine.width();

        bool lineBreakSelected = false;
        if (_blockSelect) {
            const int lastSelectBlockHighlightColumn = lastSelectBlockColumn + (multiIns ? 1 : 0);
            if (firstSelectBlockLine <= line && line <= lastSelectBlockLine) {
                lineBreakSelected = firstSelectBlockColumn <= lastLine.width() && lastLine.width() < lastSelectBlockHighlightColumn;

                // FIXME this does not work with soft wrapped lines, so disable for now when in a wrapped line
                if (lay.lineCount() == 1 && lastLine.width() + 1 < lastSelectBlockHighlightColumn) {
                    Tui::ZTextStyle markStyle = selected;
                    if (firstSelectBlockColumn == lastSelectBlockColumn) {
                        markStyle = multiInsertChar;
                    }
                    const int firstColumnAfterLineBreakMarker = std::max(lastLine.width() + 1, firstSelectBlockColumn);
                    painter->clearRect(-scrollPositionColumns + firstColumnAfterLineBreakMarker + lineNumberBorderWidth() + lineMarkerBorderWidth(),
                                       y + lastLine.y(), std::max(1, lastSelectBlockColumn - firstColumnAfterLineBreakMarker),
                                       1, markStyle.foregroundColor(), markStyle.backgroundColor());
                }
            }
        } else {
            lineBreakSelected = startSelectCursor.line <= line && endSelectCursor.line > line;
        }

        if (lineBreakSelected) {
            if (formattingCharacters()) {
                Tui::ZTextStyle markStyle = selectedFormatingChar;
                if (multiIns) {
                    markStyle = multiInsertChar;
                }
                painter->writeWithAttributes(-scrollPositionColumns + lastLine.width() + lineNumberBorderWidth() + lineMarkerBorderWidth(), y + lastLine.y(), QStringLiteral("¶"),
                                         markStyle.foregroundColor(), markStyle.backgroundColor(), markStyle.attributes());
            } else {
                Tui::ZTextStyle markStyle = selected;
                if (multiIns) {
                    markStyle = multiInsertChar;
                }
                painter->clearRect(-scrollPositionColumns + lastLine.width() + lineNumberBorderWidth() + lineMarkerBorderWidth(), y + lastLine.y(), 1, 1, markStyle.foregroundColor(), markStyle.backgroundColor());
            }
        } else if (formattingCharacters()) {
            const Tui::ZTextStyle &markStyle = (_rightMarginHint && lastLine.width() > _rightMarginHint) ? formatingCharInMargin : formatingChar;
            painter->writeWithAttributes(-scrollPositionColumns + lastLine.width() + lineNumberBorderWidth() + lineMarkerBorderWidth(), y + lastLine.y(), QStringLiteral("¶"),
                                         markStyle.foregroundColor(), markStyle.backgroundColor(), markStyle.attributes());
        }

        if (cursorLine == line) {
            if (focus()) {
                if (_blockSelect) {
                    painter->setCursor(-scrollPositionColumns + lineNumberBorderWidth() + lineMarkerBorderWidth() + _blockSelectEndColumn, y);
                } else {
                    lay.showCursor(*painter, {-scrollPositionColumns + lineNumberBorderWidth() + lineMarkerBorderWidth(), y}, cursorCodeUnit);
                }
            }
        }
        // linenumber
        if (showLineNumbers()) {
            // Wrapping
            for (int i = lay.lineCount() - 1; i > 0; i--) {
                painter->writeWithColors(0, y + i, QString(" ").repeated(lineNumberBorderWidth() + lineMarkerBorderWidth()),
                                         getColor("chr.linenumberFg"), getColor("chr.linenumberBg"));
            }
            if (hasLineMarker(line)) {
                strlinenumber = QString::number(line + 1)
                                + QString("*") + QString(" ").repeated(lineNumberBorderWidth() - QString::number(line + 1).size());
            } else {
                strlinenumber = QString::number(line + 1)
                                + QString(" ").repeated(lineNumberBorderWidth() + lineMarkerBorderWidth() - QString::number(line + 1).size());
            }
            int lineNumberY = y;
            if (y < 0) {
                strlinenumber.replace(' ', '^');
                lineNumberY = 0;
            }
            if (line == cursorLine || hasLineMarker(line)) {
                painter->writeWithAttributes(0, lineNumberY, strlinenumber,
                                             getColor("chr.linenumberFg"), getColor("chr.linenumberBg"),
                                             Tui::ZTextAttribute::Bold);
            } else {
                painter->writeWithColors(0, lineNumberY, strlinenumber,
                                         getColor("chr.linenumberFg"), getColor("chr.linenumberBg"));
            }
        } else {
            for (int i = lay.lineCount() - 1; i > 0; i--) {
                painter->writeWithColors(0, y + i, QString(" ").repeated(lineMarkerBorderWidth()),
                                         getColor("chr.linenumberFg"), getColor("chr.linenumberBg"));
            }
            if (hasLineMarker(line)) {
                strlinenumber = QString("*") + QString(" ").repeated(lineMarkerBorderWidth() -1);
            } else {
                strlinenumber = QString(" ").repeated(lineMarkerBorderWidth());
            }

            int lineNumberY = y;
            if (y < 0) {
                strlinenumber.replace(' ', '^');
                lineNumberY = 0;
            }

            painter->writeWithAttributes(0, lineNumberY, strlinenumber,
                                         getColor("chr.linenumberFg"), getColor("chr.linenumberBg"),
                                         Tui::ZTextAttribute::Bold);
        }
        y += lay.lineCount();
    }
    if (document()->newlineAfterLastLineMissing()) {
        if (formattingCharacters() && y < rect().height() && scrollPositionColumns == 0) {
            const Tui::ZTextStyle &markStyle = (_rightMarginHint && tmpLastLineWidth > _rightMarginHint) ? formatingCharInMargin : formatingChar;

            painter->writeWithAttributes(-scrollPositionColumns + tmpLastLineWidth + lineNumberBorderWidth() + lineMarkerBorderWidth(), y - 1, "♦",
                                         markStyle.foregroundColor(), markStyle.backgroundColor(), markStyle.attributes());
        }
        painter->writeWithAttributes(0 + lineNumberBorderWidth() + lineMarkerBorderWidth(), y, "\\ No newline at end of file",
                                     formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
    } else {
        if (formattingCharacters() && y < rect().height() && scrollPositionColumns == 0) {
            painter->writeWithAttributes(0 + lineNumberBorderWidth() + lineMarkerBorderWidth(), y, "♦",
                                         formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
        }
    }
}

int File::pageNavigationLineCount() const {
    if (wordWrapMode()) {
        return std::max(1, geometry().height() - 2);
    }
    return std::max(1, geometry().height() - 1);
}

void File::appendLine(const QString &line) {
    Tui::ZDocumentCursor cur = makeCursor();
    if (document()->lineCount() == 1 && document()->lineCodeUnits(0) == 0) {
        cur.insertText(line);
        // We reposition the cursor so that the cursor is not moved in front of the cur coursor.
        Tui::ZDocumentCursor cursor = textCursor();
        cursor.setPosition({0, 0});
        setTextCursor(cursor);
    } else {
        cur.moveToEndOfDocument();
        cur.insertText("\n" + line);
    }
    if (_followMode) {
        Tui::ZDocumentCursor cursor = textCursor();
        cursor.setPosition({cursor.position().codeUnit, document()->lineCount() - 1});
        setTextCursor(cursor);
    }
    adjustScrollPosition();
}

void File::insertText(const QString &str) { // TODO das ist kein insertText... Oder vielleicht doch?
    auto undoGroup = startUndoGroup();

    if (_blockSelect) {
        if (hasBlockSelection()) {
            blockSelectRemoveSelectedAndConvertToMultiInsert();
        }

        QStringList source = str.split('\n');
        if (source.last().isEmpty()) {
            source.removeLast();
        }

        if (source.size()) {
            const int firstSelectBlockLine = std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line());
            const int lastSelectBlockLine = std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line());
            const int column = _blockSelectStartColumn;

            int sourceLine = 0;

            for (int line = firstSelectBlockLine; line < document()->lineCount() && line <= lastSelectBlockLine; line++) {
                if (source.size() != 1 && sourceLine >= source.size()) {
                    break;
                }

                Tui::ZTextLayout laySel = textLayoutForLineWithoutWrapping(line);
                Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

                Tui::ZDocumentCursor cur = makeCursor();

                const int codeUnitInLine = tlrSel.xToCursor(column);
                cur.setPosition({codeUnitInLine, line});

                if (tlrSel.width() < column) {
                    cur.insertText(QString(" ").repeated(column - tlrSel.width()));
                }

                cur.insertText(source[sourceLine]);

                if (source.size() > 1) {
                    sourceLine += 1;
                    if (line == lastSelectBlockLine) {
                        if (sourceLine < source.size()) {
                            // Now sure what do do with the overflowing lines, for now just dump them in the last line
                            for (; sourceLine < source.size(); sourceLine++) {
                                cur.insertText("|" + source[sourceLine]);
                            }
                        }
                    }
                } else {
                    // keep repeating the one line for all selected lines
                }
                if (line == lastSelectBlockLine) {
                    Tui::ZTextLayout layNew = textLayoutForLineWithoutWrapping(line);
                    Tui::ZTextLineRef tlrNew = layNew.lineAt(0);
                    _blockSelectStartColumn = _blockSelectEndColumn = tlrNew.cursorToX(cur.position().codeUnit, Tui::ZTextLayout::Leading);
                }
            }
        }
    } else {
        // Inserting might adjust the scroll position, so save it here and restore it later.
        const int line = scrollPositionLine();
        Tui::ZDocumentCursor cursor = textCursor();
        cursor.insertText(str);
        setTextCursor(cursor);
        setScrollPosition(scrollPositionColumn(), line, scrollPositionFineLine());
    }
    adjustScrollPosition();
}

void File::sortSelecedLines() {
    if (hasBlockSelection() || hasMultiInsert() || ZTextEdit::hasSelection()) {
        const auto [startLine, endLine] = getSelectedLines();
        auto lines = getSelectedLinesSort();

        Tui::ZDocumentCursor cursor = textCursor();
        document()->sortLines(lines.first, lines.second + 1, &cursor);
        selectLines(startLine, endLine);
    }
    adjustScrollPosition();
}

bool File::event(QEvent *event) {
    if (!parent()) {
        return ZWidget::event(event);
    }

    if (event->type() == Tui::ZEventType::terminalChange()) {
        // We are not allowed to have the cursor position between characters. Character boundaries depend on the
        // detected terminal thus reset the position to get the needed adjustment now.
        Tui::ZDocumentCursor cursor = textCursor();
        if (!cursor.atLineStart() && terminal()) {
            if (_blockSelect) {
                disableBlockSelection();
            } else {
                setCursorPosition(cursor.position());
            }
        }
    }

    return ZWidget::event(event);
}

bool File::followStandardInput() {
    return _followMode;
}

void File::pasteEvent(Tui::ZPasteEvent *event) {
    QString text = event->text();
    if (_formattingCharacters) {
        text.replace(QString("·"), QString(" "));
        text.replace(QString("→"), QString(" "));
        text.replace(QString("¶"), QString(""));
    }
    text.replace(QString("\r\n"), QString('\n'));
    text.replace(QString('\r'), QString('\n'));

    insertText(text);
    document()->clearCollapseUndoStep();
    adjustScrollPosition();
}

void File::focusInEvent(Tui::ZFocusEvent *event) {
    Q_UNUSED(event);
    updateCommands();
    if (_searchText == "") {
        _cmdSearchNext->setEnabled(false);
        _cmdSearchPrevious->setEnabled(false);
    } else {
        _cmdSearchNext->setEnabled(true);
        _cmdSearchPrevious->setEnabled(true);
    }
    modifiedChanged(isModified());
}

bool File::isNewFile() {
    if (document()->filename() == "NEWFILE") {
        return true;
    }
    return false;
}

void File::keyEvent(Tui::ZKeyEvent *event) {
    auto undoGroup = startUndoGroup();

    QString text = event->text();
    const bool isAltShift = event->modifiers() == (Qt::AltModifier | Qt::ShiftModifier);
    const bool isAltCtrlShift = event->modifiers() == (Qt::AltModifier | Qt::ControlModifier | Qt::ShiftModifier);

    if (event->key() == Qt::Key_Space && event->modifiers() == 0) {
        text = " ";
    }

    auto delAndClearSelection = [this] {
        //Markierte Zeichen Löschen
        removeSelectedText();
        clearSelection();
        adjustScrollPosition();
    };

    if (event->key() == Qt::Key_Backspace && (event->modifiers() == 0 || event->modifiers() == Qt::ControlModifier)) {
        disableDetachedScrolling();
        setSelectMode(false);
        if (hasBlockSelection()) {
            delAndClearSelection();
        } else if (hasMultiInsert()) {
            if (event->modifiers() & Qt::ControlModifier) {
                multiInsertDeletePreviousWord();
            } else {
                multiInsertDeletePreviousCharacter();
            }
        } else {
            Tui::ZDocumentCursor cursor = textCursor();
            if (event->modifiers() & Qt::ControlModifier) {
                cursor.deletePreviousWord();
            } else {
                cursor.deletePreviousCharacter();
            }
            setTextCursor(cursor);
        }
        updateCommands();
        adjustScrollPosition();
    } else if (event->key() == Qt::Key_Delete && (event->modifiers() == 0 || event->modifiers() == Qt::ControlModifier)) {
        disableDetachedScrolling();
        setSelectMode(false);
        if (hasBlockSelection()) {
            delAndClearSelection();
        } else if (hasMultiInsert()) {
            if (event->modifiers() & Qt::ControlModifier) {
                multiInsertDeleteWord();
            } else {
                multiInsertDeleteCharacter();
            }
        } else {
            Tui::ZDocumentCursor cursor = textCursor();
            if (event->modifiers() & Qt::ControlModifier) {
                cursor.deleteWord();
            } else {
                if (cursor.atEnd()) {
                    if (document()->line(cursor.position().line) != "") {
                        document()->setNewlineAfterLastLineMissing(true);
                        cursor.deleteCharacter();
                    } else {
                        if (cursor.atStart() == cursor.atEnd()) {
                            document()->setNewlineAfterLastLineMissing(true);
                        }
                    }
                } else {
                    cursor.deleteCharacter();
                }
            }
            setTextCursor(cursor);
        }
        updateCommands();
        adjustScrollPosition();
    } else if (text.size() && event->modifiers() == 0) {
        disableDetachedScrolling();
        if (_formattingCharacters) {
            if (text == "·" || text == "→") {
                text = " ";
            } else if (event->text() == "¶") {
                //do not add the character
                text = "";
                delAndClearSelection();
            }
        }
        if (text.size()) {
            setSelectMode(false);

            if (_blockSelect) {
                if (hasBlockSelection()) {
                    blockSelectRemoveSelectedAndConvertToMultiInsert();
                }

                if (overwriteMode()) {
                    multiInsertDeleteCharacter();
                }
                multiInsertInsert(text);
            } else {
                Tui::ZDocumentCursor cursor = textCursor();
                // Inserting might adjust the scroll position, so save it here and restore it later.
                const int line = scrollPositionLine();

                if (overwriteMode()) {
                    cursor.overwriteText(text);
                } else {
                    cursor.insertText(text);
                }
                setTextCursor(cursor);
                setScrollPosition(scrollPositionColumn(), line, scrollPositionFineLine());
            }
            adjustScrollPosition();
        }
        updateCommands();
    } else if (event->text() == "S" && (event->modifiers() == Qt::AltModifier || event->modifiers() == (Qt::AltModifier | Qt::ShiftModifier))  && hasSelection()) {
        disableDetachedScrolling();
        // Alt + Shift + s sort selected lines
        sortSelecedLines();
        adjustScrollPosition();
        update();
    } else if (event->key() == Qt::Key_Left) {
        disableDetachedScrolling();
        if (isAltShift || isAltCtrlShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            if (_blockSelectEndColumn > 0) {
                const auto mode = event->modifiers() & Qt::ControlModifier ?
                            Tui::ZTextLayout::SkipWords : Tui::ZTextLayout::SkipCharacters;
                Tui::ZTextLayout lay = textLayoutForLineWithoutWrapping(_blockSelectEndLine->line());
                Tui::ZTextLineRef tlr = lay.lineAt(0);
                const int codeUnitInLine = tlr.xToCursor(_blockSelectEndColumn);
                if (codeUnitInLine != document()->lineCodeUnits(_blockSelectEndLine->line())) {
                    _blockSelectEndColumn = tlr.cursorToX(lay.previousCursorPosition(codeUnitInLine, mode),
                                                          Tui::ZTextLayout::Leading);
                } else {
                    _blockSelectEndColumn -= 1;
                }
            }
        } else {
            if (_blockSelect) {
                disableBlockSelection();
            }

            Tui::ZDocumentCursor cursor = textCursor();
            const bool extendSelection = event->modifiers() & Qt::ShiftModifier || selectMode();
            if (event->modifiers() & Tui::ControlModifier) {
                cursor.moveWordLeft(extendSelection);
            } else {
                cursor.moveCharacterLeft(extendSelection);
            }
            setTextCursor(cursor);
        }
        updateCommands();
        adjustScrollPosition();
        document()->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Right) {
        disableDetachedScrolling();
        if (isAltShift || isAltCtrlShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            const auto mode = event->modifiers() & Qt::ControlModifier ?
                        Tui::ZTextLayout::SkipWords : Tui::ZTextLayout::SkipCharacters;

            Tui::ZTextLayout lay = textLayoutForLineWithoutWrapping(_blockSelectEndLine->line());
            Tui::ZTextLineRef tlr = lay.lineAt(0);
            const int codeUnitInLine = tlr.xToCursor(_blockSelectEndColumn);
            if (tlr.cursorToX(codeUnitInLine, Tui::ZTextLayout::Leading) == _blockSelectEndColumn
                && codeUnitInLine != document()->lineCodeUnits(_blockSelectEndLine->line())) {
                _blockSelectEndColumn = tlr.cursorToX(lay.nextCursorPosition(codeUnitInLine, mode),
                                                      Tui::ZTextLayout::Leading);
            } else {
                _blockSelectEndColumn += 1;
            }
        } else {
            if (_blockSelect) {
                disableBlockSelection();
            }

            Tui::ZDocumentCursor cursor = textCursor();
            const bool extendSelection = event->modifiers() & Qt::ShiftModifier || selectMode();
            if (event->modifiers() & Tui::ControlModifier) {
                cursor.moveWordRight(extendSelection);
            } else {
                cursor.moveCharacterRight(extendSelection);
            }
            setTextCursor(cursor);
        }
        updateCommands();
        adjustScrollPosition();
        document()->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Down && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier || isAltShift)) {
        disableDetachedScrolling();
        if (isAltShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            if (document()->lineCount() - 1 > _blockSelectEndLine->line()) {
                _blockSelectEndLine->setLine(_blockSelectEndLine->line() + 1);
            }
        } else {
            clearAdvancedSelection();

            const bool extendSelection = event->modifiers() & Qt::ShiftModifier || selectMode();
            Tui::ZDocumentCursor cursor = textCursor();
            cursor.moveDown(extendSelection);
            setTextCursor(cursor);
        }
        updateCommands();
        adjustScrollPosition();
        document()->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Up && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier || isAltShift)) {
        disableDetachedScrolling();
        if (isAltShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            if (_blockSelectEndLine->line() > 0) {
                _blockSelectEndLine->setLine(_blockSelectEndLine->line() - 1);
            }
        } else {
            clearAdvancedSelection();

            const bool extendSelection = event->modifiers() & Qt::ShiftModifier || selectMode();
            Tui::ZDocumentCursor cursor = textCursor();
            cursor.moveUp(extendSelection);
            setTextCursor(cursor);
        }
        updateCommands();
        adjustScrollPosition();
        document()->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Home && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier || isAltShift)) {
        disableDetachedScrolling();
        if (isAltShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            if (_blockSelectEndColumn == 0) {
                int codeUnitInLine = 0;
                for (; codeUnitInLine <= document()->lineCodeUnits(_blockSelectEndLine->line()) - 1; codeUnitInLine++) {
                    if (document()->line(_blockSelectEndLine->line())[codeUnitInLine] != ' ' && document()->line(_blockSelectEndLine->line())[codeUnitInLine] != '\t') {
                        break;
                    }
                }

                Tui::ZTextLayout lay = textLayoutForLineWithoutWrapping(_blockSelectEndLine->line());
                Tui::ZTextLineRef tlr = lay.lineAt(0);

                _blockSelectEndColumn = tlr.cursorToX(codeUnitInLine, Tui::ZTextLayout::Leading);
            } else {
                _blockSelectEndColumn = 0;
            }
        } else {
            clearAdvancedSelection();

            const bool extendSelection = event->modifiers() == Qt::ShiftModifier || selectMode();

            Tui::ZDocumentCursor cursor = textCursor();
            if (cursor.atLineStart()) {
                cursor.moveToStartIndentedText(extendSelection);
            } else {
                cursor.moveToStartOfLine(extendSelection);
            }
            setTextCursor(cursor);
        }
        updateCommands();
        adjustScrollPosition();
        document()->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_End && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier || isAltShift)) {
        disableDetachedScrolling();
        if (isAltShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            Tui::ZTextLayout lay = textLayoutForLineWithoutWrapping(_blockSelectEndLine->line());
            Tui::ZTextLineRef tlr = lay.lineAt(0);

            _blockSelectEndColumn = tlr.cursorToX(document()->lineCodeUnits(_blockSelectEndLine->line()), Tui::ZTextLayout::Leading);
        } else {
            clearAdvancedSelection();

            Tui::ZDocumentCursor cursor = textCursor();
            if (event->modifiers() == Qt::ShiftModifier || selectMode()) {
                cursor.moveToEndOfLine(true);
            } else {
                cursor.moveToEndOfLine(false);
            }
            setTextCursor(cursor);
            updateCommands();
        }

        adjustScrollPosition();
        document()->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_PageDown && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        disableDetachedScrolling();
        if (_blockSelect && event->modifiers() == Qt::ShiftModifier) {
            if (document()->lineCount() > _blockSelectEndLine->line() + pageNavigationLineCount()) {
                _blockSelectEndLine->setLine(_blockSelectEndLine->line() + pageNavigationLineCount());
            } else {
                _blockSelectEndLine->setLine(document()->lineCount() - 1);
            }
        } else {
            clearAdvancedSelection();

            Tui::ZDocumentCursor cursor = textCursor();
            // Shift+PageUp/Down does not work with xterm's default settings.
            const bool extendSelection = event->modifiers() == Qt::ShiftModifier || selectMode();
            const int amount = pageNavigationLineCount();
            for (int i = 0; i < amount; i++) {
                cursor.moveDown(extendSelection);
            }
            setTextCursor(cursor);
        }
        adjustScrollPosition();
        document()->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_PageUp && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        disableDetachedScrolling();
        if (_blockSelect && event->modifiers() == Qt::ShiftModifier) {
            if (_blockSelectEndLine->line() > pageNavigationLineCount()) {
                _blockSelectEndLine->setLine(_blockSelectEndLine->line() - pageNavigationLineCount());
            } else {
                _blockSelectEndLine->setLine(0);
            }
        } else {
            clearAdvancedSelection();

            Tui::ZDocumentCursor cursor = textCursor();
            // Shift+PageUp/Down does not work with xterm's default settings.
            const bool extendSelection = event->modifiers() == Qt::ShiftModifier || selectMode();
            const int amount = pageNavigationLineCount();
            for (int i = 0; i < amount; i++) {
                cursor.moveUp(extendSelection);
            }
            setTextCursor(cursor);
        }
        adjustScrollPosition();
        document()->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Enter && (event->modifiers() & ~Qt::KeypadModifier) == 0) {
        disableDetachedScrolling();
        setSelectMode(false);
        if (_blockSelect) {
            delAndClearSelection();
        }
        // Inserting might adjust the scroll position, so save it here and restore it later.
        const int line = scrollPositionLine();
        Tui::ZDocumentCursor cursor = textCursor();
        cursor.insertText("\n");
        setTextCursor(cursor);
        setScrollPosition(scrollPositionColumn(), line, scrollPositionFineLine());
        updateCommands();
        adjustScrollPosition();
    } else if (event->key() == Qt::Key_Tab && event->modifiers() == 0) {
        disableDetachedScrolling();
        if (_blockSelect) {
            if (hasBlockSelection()) {
                blockSelectRemoveSelectedAndConvertToMultiInsert();
            }

            multiInsertForEachCursor(mi_add_spaces, [&](Tui::ZDocumentCursor &cur) {
                insertTabAt(cur);
            });
        } else if (ZTextEdit::hasSelection()) {
            // Add one level of indent to the selected lines.

            const auto [firstLine, lastLine] = getSelectedLinesSort();
            const auto [startLine, endLine] = getSelectedLines();

            const bool savedSelectMode = selectMode();
            clearSelection();

            Tui::ZDocumentCursor cur = makeCursor();

            for (int line = firstLine; line <= lastLine; line++) {
                if (document()->lineCodeUnits(line) > 0) {
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
            setSelectMode(savedSelectMode);
            adjustScrollPosition();
        } else {
            Tui::ZDocumentCursor cursor = textCursor();
            if (_eatSpaceBeforeTabs && !useTabChar()) {
                // If spaces in front of a tab
                const auto [cursorCodeUnit, cursorLine] = cursor.position();
                int leadingSpace = 0;
                for (; leadingSpace < document()->lineCodeUnits(cursorLine) && document()->line(cursorLine)[leadingSpace] == ' '; leadingSpace++);
                if (leadingSpace > cursorCodeUnit) {
                    cursor.setPosition({leadingSpace, cursorLine});
                }
            }
            // a normal tab
            insertTabAt(cursor);
            setTextCursor(cursor);
            updateCommands();
            adjustScrollPosition();
            update();
        }
    } else if (event->key() == Qt::Key_Tab && event->modifiers() == Qt::ShiftModifier) {
        disableDetachedScrolling();
        Tui::ZDocumentCursor cursor = textCursor();
        // returns current line if no selection is active
        const auto [firstLine, lastLine] = getSelectedLinesSort();
        const auto [startLine, endLine] = getSelectedLines();
        const auto [cursorCodeUnit, cursorLine] = cursor.position();

        const bool savedSelectMode = selectMode();
        const bool reselect = hasMultiInsert() || hasBlockSelection() || cursor.hasSelection();
        clearSelection();

        int cursorAdjust = 0;

        for (int line = firstLine; line <= lastLine; line++) {
            int codeUnitsToRemove = 0;
            if (document()->lineCodeUnits(line) && document()->line(line)[0] == '\t') {
                codeUnitsToRemove = 1;
            } else {
                while (true) {
                    if (codeUnitsToRemove < document()->lineCodeUnits(line)
                        && codeUnitsToRemove < tabStopDistance()
                        && document()->line(line)[codeUnitsToRemove] == ' ') {
                        codeUnitsToRemove++;
                    } else {
                        break;
                    }
                }
            }

            cursor.setPosition({0, line});
            cursor.setPosition({codeUnitsToRemove, line}, true);
            cursor.removeSelectedText();
            if (!reselect && line == cursorLine) {
                cursorAdjust = codeUnitsToRemove;
            }
        }

        // Update cursor / recreate selection
        if (!reselect) {
            setCursorPosition({cursorCodeUnit - cursorAdjust, cursorLine});
        } else {
            selectLines(startLine, endLine);
        }
        setSelectMode(savedSelectMode);
        adjustScrollPosition();
    } else if (event->text() == "d" && event->modifiers() == Qt::ControlModifier) {
        disableDetachedScrolling();
        // Ctrl + d -> delete single line
        deleteLine();
    } else if ((event->text() == "m") && event->modifiers() == Qt::ControlModifier) {
        // Not all terminals support this shortcut.
        toggleLineMarker();
    } else if (event->text() == "," && event->modifiers() == Qt::ControlModifier) {
        gotoPreviousLineMarker();
    } else if (event->text() == "." && event->modifiers() == Qt::ControlModifier) {
        gotoNextLineMarker();
    } else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Up) {
        // Fenster hoch Scrolen
        detachedScrollUp();
    } else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Down) {
        // Fenster runter Scrolen
        detachedScrollDown();
    } else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier) && event->key() == Qt::Key_Up) {
        disableDetachedScrolling();
        // returns current line if no selection is active
        const auto [firstLine, lastLine] = getSelectedLinesSort();

        if (firstLine > 0) {
            Tui::ZDocumentCursor cursor = textCursor();
            const auto scrollPositionYSave = scrollPositionLine();
            const auto [startLine, endLine] = getSelectedLines();
            const auto [cursorCodeUnit, cursorLine] = cursor.position();

            const bool savedSelectMode = selectMode();
            const bool reselect = hasMultiInsert() || hasBlockSelection() || cursor.hasSelection();
            clearSelection();

            // move lines up
            document()->moveLine(firstLine - 1, endLine, &cursor);

            // Update cursor / recreate selection
            if (!reselect) {
                setCursorPosition({cursorCodeUnit, cursorLine - 1});
            } else {
                selectLines(startLine - 1, endLine - 1);
            }
            setSelectMode(savedSelectMode);
            setScrollPosition(scrollPositionColumn(), scrollPositionYSave, scrollPositionFineLine());
            adjustScrollPosition();
        }
    } else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier) && event->key() == Qt::Key_Down) {
        disableDetachedScrolling();
        // returns current line if no selection is active
        const auto [firstLine, lastLine] = getSelectedLinesSort();

        if (lastLine < document()->lineCount() - 1) {
            Tui::ZDocumentCursor cursor = textCursor();
            const auto scrollPositionYSave = scrollPositionLine();
            const auto [startLine, endLine] = getSelectedLines();
            const auto [cursorCodeUnit, cursorLine] = cursor.position();

            const bool savedSelectMode = selectMode();
            const bool reselect = hasMultiInsert() || hasBlockSelection() || cursor.hasSelection();
            clearSelection();

            // Move lines down
            document()->moveLine(lastLine + 1, firstLine, &cursor);

            // Update cursor / recreate selection
            if (!reselect) {
                setCursorPosition({cursorCodeUnit, cursorLine + 1});
            } else {
                selectLines(startLine + 1, endLine + 1);
            }
            setSelectMode(savedSelectMode);
            setScrollPosition(scrollPositionColumn(), scrollPositionYSave, scrollPositionFineLine());
            adjustScrollPosition();
        }
    } else if (event->key() == Qt::Key_Escape && event->modifiers() == 0) {
        disableDetachedScrolling();
        setSearchVisible(false);
        setSelectMode(false);

        clearAdvancedSelection();
        adjustScrollPosition();
    } else if (event->key() == Qt::Key_Insert && event->modifiers() == 0) {
        disableDetachedScrolling();
        toggleOverwriteMode();
    } else if (event->key() == Qt::Key_F4 && event->modifiers() == 0) {
        disableDetachedScrolling();
        toggleSelectMode();
    } else {
        undoGroup.closeGroup();
        ZTextEdit::keyEvent(event);
    }
}

std::tuple<int, int, int> File::cursorPositionOrBlockSelectionEnd() {
    if (_blockSelect) {
        const int cursorLine = _blockSelectEndLine->line();
        const int cursorColumn = _blockSelectEndColumn;
        Tui::ZTextLayout layNoWrap = textLayoutForLineWithoutWrapping(cursorLine);
        const int cursorCodeUnit = layNoWrap.lineAt(0).xToCursor(cursorColumn);
        return std::make_tuple(cursorCodeUnit, cursorLine, cursorColumn);
    } else {
        const auto [cursorCodeUnit, cursorLine] = cursorPosition();
        Tui::ZTextLayout layNoWrap = textLayoutForLineWithoutWrapping(cursorLine);
        int cursorColumn = layNoWrap.lineAt(0).cursorToX(cursorCodeUnit, Tui::ZTextLayout::Leading);

        return std::make_tuple(cursorCodeUnit, cursorLine, cursorColumn);
    }
}

void File::adjustScrollPosition() {
    // diff to base class: uses cursorPositionOrBlockSelectionEnd(…)
    if (geometry().width() <= 0 && geometry().height() <= 0) {
        return;
    }

    int newScrollPositionLine = scrollPositionLine();
    int newScrollPositionColumn = scrollPositionColumn();
    int newScrollPositionFineLine = scrollPositionFineLine();

    if (isDetachedScrolling()) {
        if (newScrollPositionLine >= document()->lineCount()) {
            newScrollPositionLine = document()->lineCount() - 1;
        }

        setScrollPosition(newScrollPositionColumn, newScrollPositionLine, newScrollPositionFineLine);
        return;
    }

    const auto [cursorCodeUnit, cursorLine, cursorColumn] = cursorPositionOrBlockSelectionEnd();

    int viewWidth = geometry().width() - allBordersWidth();
    // horizontal scroll position
    if (wordWrapMode() == Tui::ZTextOption::WrapMode::NoWrap) {
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
    if (wordWrapMode() == Tui::ZTextOption::WrapMode::NoWrap) {
        if (cursorLine >= 0) {
            if (cursorLine - newScrollPositionLine < 1) {
                newScrollPositionLine = cursorLine;
                newScrollPositionFineLine = 0;
            }
        }

        if (cursorLine - newScrollPositionLine >= geometry().height() - 1) {
            newScrollPositionLine = cursorLine - geometry().height() + 2;
        }

        if (document()->lineCount() - newScrollPositionLine < geometry().height() - 1) {
            newScrollPositionLine = std::max(0, document()->lineCount() - geometry().height() + 1);
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

                    //_scrollPositionY = line;
                    //_scrollFineLine = (linesAbove + lay.lineCount()) - availableLinesAbove;
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
        if (newScrollPositionLine && newScrollPositionLine + (geometry().height() - 1) > document()->lineCount()) {
            int linesCounted = 0;
            QVector<int> sizes;

            for (int line = document()->lineCount() - 1; line >= 0; line--) {
                Tui::ZTextLayout lay = textLayoutForLine(option, line);
                sizes.append(lay.lineCount());
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

    int max=0;
    for (int i = newScrollPositionLine; i < document()->lineCount() && i < newScrollPositionLine + geometry().height(); i++) {
        if(max < document()->lineCodeUnits(i)) {
            max = document()->lineCodeUnits(i);
        }
    }
    scrollRangeChanged(std::max(0, max - viewWidth), std::max(0, document()->lineCount() - geometry().height()));

    if (hasSelection()) {
        auto l = getSelectedLines();
        int lines = 1 + std::max(l.second, l.first) - std::min(l.second, l.first);
        selectCharLines(selectedText().count(), lines);
    } else {
        selectCharLines(0, 0);
    }
    update();
}

