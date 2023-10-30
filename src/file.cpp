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

#include "clipboard.h"
#include "searchcount.h"


File::File(Tui::ZTextMetrics textMetrics, Tui::ZWidget *parent)
    : Tui::ZWidget(parent), _textMetrics(textMetrics), _doc(std::make_shared<Tui::ZDocument>()),
      _cursor(createCursor()), _scrollPositionY(_doc.get())
{
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
    QObject::connect(_cmdUndo, &Tui::ZCommandNotifier::activated, this, &File::undo);
    _cmdUndo->setEnabled(false);
    QObject::connect(_doc.get(), &Tui::ZDocument::undoAvailable, _cmdUndo, &Tui::ZCommandNotifier::setEnabled);
    _cmdRedo = new Tui::ZCommandNotifier("Redo", this, Qt::WindowShortcut);
    QObject::connect(_cmdRedo, &Tui::ZCommandNotifier::activated, this, &File::redo);
    _cmdRedo->setEnabled(false);
    QObject::connect(_doc.get(), &Tui::ZDocument::redoAvailable, _cmdRedo, &Tui::ZCommandNotifier::setEnabled);

    _cmdSearchNext = new Tui::ZCommandNotifier("Search Next", this, Qt::WindowShortcut);
    QObject::connect(_cmdSearchNext, &Tui::ZCommandNotifier::activated, this, [this]{runSearch(false);});
    _cmdSearchNext->setEnabled(false);
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, 0), this, Qt::WindowShortcut), &Tui::ZShortcut::activated,
          [this] {
            runSearch(false);
          });

    _cmdSearchPrevious = new Tui::ZCommandNotifier("Search Previous", this, Qt::WindowShortcut);
    QObject::connect(_cmdSearchPrevious, &Tui::ZCommandNotifier::activated, this, [this]{runSearch(true);});
    _cmdSearchPrevious->setEnabled(false);
    QObject::connect(new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3, Qt::ShiftModifier), this, Qt::WindowShortcut), &Tui::ZShortcut::activated,
          [this] {
            runSearch(true);
          });

    QObject::connect(_doc.get(), &Tui::ZDocument::modificationChanged, this, &File::modifiedChanged);

    QObject::connect(_doc.get(), &Tui::ZDocument::lineMarkerChanged, this, [this](const Tui::ZDocumentLineMarker *marker) {
        if (marker == &_scrollPositionY || (_blockSelectEndLine && marker == &*_blockSelectEndLine)) {
            // Recalculate the scroll position:
            //  * In case any editing from outside of this class moved our scroll position line marker to ensure
            //    that the line marker still keeps the cursor visible
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
            int utf8PositionX = _doc->line(cursorLine).left(cursorCodeUnit).toUtf8().size();
            cursorPositionChanged(cursorColumn, utf8PositionX, cursorLine);
        }
    });

    QObject::connect(_doc.get(), &Tui::ZDocument::cursorChanged, this, [this](const Tui::ZDocumentCursor *marker) {
        if (marker == &_cursor) {
            // Ensure that even if editing from outside of this class moved the cursor position it is still in the
            // visible portion of the widget.
            adjustScrollPosition();

            // this is our main way to notify other components of cursor position changes.
            const auto [cursorCodeUnit, cursorLine, cursorColumn] = cursorPositionOrBlockSelectionEnd();
            int utf8PositionX = _doc->line(cursorLine).left(cursorCodeUnit).toUtf8().size();
            cursorPositionChanged(cursorColumn, utf8PositionX, cursorLine);
        }
    });

#ifdef SYNTAX_HIGHLIGHTING
    qRegisterMetaType<Updates>();

    QObject::connect(_doc.get(), &Tui::ZDocument::contentsChanged, this, [this] {
        updateSyntaxHighlighting(false);
    });
#endif

}

#ifdef SYNTAX_HIGHLIGHTING

void File::updateSyntaxHighlighting(bool force = false) {
    if (force) {
        _doc->setLineUserData(0, nullptr);
    }
    if (!syntaxHighlightingActive() || !_syntaxHighlightDefinition.isValid()
            || !_syntaxHighlightingTheme.isValid()) {
        return;
    }

    Tui::ZDocumentSnapshot snapshot = _doc->snapshot();
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
    if (updates.documentRevision != _doc->revision()) {
        // Lines numbers might have changed (by insertion or deletion of lines), we can't use this update
        return;
    }

    const int visibleLinesStart = _scrollPositionY.line();
    const int visibleLinesEnd = visibleLinesStart + geometry().height();

    bool needRepaint = false;

    for (int i = 0; i < updates.lines.size(); i++) {
        const int line = updates.lines[i];

        if (line < _doc->lineCount() && _doc->lineRevision(line) == updates.data[i]->lineRevision) {
            _doc->setLineUserData(line, updates.data[i]);

            if (visibleLinesStart <= line && line <= visibleLinesEnd) {
                needRepaint = true;
            }
        }
    }

    if (needRepaint) {
        update();
    }
}

std::tuple<KSyntaxHighlighting::State, QVector<Tui::ZFormatRange>> HighlightExporter::highlightLineWrap(const QString &text, const KSyntaxHighlighting::State &state) {
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
    highlights.append(Tui::ZFormatRange(offset, length, style, style));
}
#endif

void File::setSyntaxHighlightingTheme(QString themeName) {
#ifdef SYNTAX_HIGHLIGHTING
    _syntaxHighlightingThemeName = themeName;
    _syntaxHighlightingTheme = _syntaxHighlightRepo.theme(_syntaxHighlightingThemeName);
    _syntaxHighlightExporter.setTheme(_syntaxHighlightingTheme);
    _syntaxHighlightExporter.defBg = getColor("chr.editBg");
    _syntaxHighlightExporter.defFg = getColor("chr.editFg");
    for (int line = 0; line < _doc->lineCount(); line++) {
        _doc->setLineUserData(line, nullptr);
    }
    updateSyntaxHighlighting(true);
#endif
}

void File::setSyntaxHighlightingLanguage(QString language) {
#ifdef SYNTAX_HIGHLIGHTING
    _syntaxHighlightDefinition = _syntaxHighlightRepo.definitionForName(language);
    syntaxHighlightDefinition();
    // rehighlight
    updateSyntaxHighlighting(true);
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
#endif
}

File::~File() {
    if (_searchNextFuture) {
        _searchNextFuture->cancel();
        _searchNextFuture.reset();
    }
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

Tui::ZDocumentCursor::Position File::getAttributes() {
    if (_attributesfile.isEmpty()) {
        return {0, 0};
    }
    if (readAttributes()) {
        QJsonObject data = _jo.value(getFilename()).toObject();
        return {data.value("cursorPositionX").toInt(), data.value("cursorPositionY").toInt()};
    }
    return {0, 0};
}

bool File::writeAttributes() {
    QFileInfo filenameInfo(getFilename());
    if(!filenameInfo.exists() || _attributesfile.isEmpty()) {
        return false;
    }
    readAttributes();

    const auto [cursorCodeUnit, cursorLine] = _cursor.position();

    QJsonObject data;
    data.insert("cursorPositionX", cursorCodeUnit);
    data.insert("cursorPositionY", cursorLine);
    data.insert("scrollPositionX",_scrollPositionX);
    data.insert("scrollPositionY",_scrollPositionY.line());
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
        qWarning("%s%s", "can not save attributes file: ", _attributesfile.toUtf8().data());
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

int File::tabToSpace() {
    auto undoGroup = _doc->startUndoGroup(&_cursor);

    int count = 0;

    Tui::ZTextOption option = getTextOption(false);
    option.setWrapMode(Tui::ZTextOption::NoWrap);

    // This changes code unit based positions in many lines. To avoid glitches reset selection and
    // manually save and restore cursor position. This should no longer be needed when TextCursor
    // is fixed to be able to keep its position even when edits happen.
    resetSelect();
    const auto [cursorCodeUnit, cursorLine] = _cursor.position();
    Tui::ZTextLayout lay = getTextLayoutForLine(option, cursorLine);
    int cursorPosition = lay.lineAt(0).cursorToX(cursorCodeUnit, Tui::ZTextLayout::Leading);

    Tui::ZDocumentCursor cur = createCursor();

    for (int line = 0, found = -1; line < _doc->lineCount(); line++) {
        found = _doc->line(line).lastIndexOf("\t");
        if(found != -1) {
            Tui::ZTextLayout lay = getTextLayoutForLine(option, line);
            while (found != -1) {
                int columnStart = lay.lineAt(0).cursorToX(found, Tui::ZTextLayout::Leading);
                int columnEnd = lay.lineAt(0).cursorToX(found, Tui::ZTextLayout::Trailing);
                cur.setPosition({found, line});
                cur.moveCharacterRight(true);
                cur.insertText(QString(" ").repeated(columnEnd-columnStart));
                count++;
                found = _doc->line(line).lastIndexOf("\t", found);
            }
        }
    }

    // Restore cursor position
    lay = getTextLayoutForLine(option, cursorLine);
    int newCursorPosition = lay.lineAt(0).xToCursor(cursorPosition);
    if (newCursorPosition - 1 >= 0) {
        _cursor.setPosition({newCursorPosition, cursorLine});
    }

    return count;
}

Tui::ZDocumentCursor::Position File::getCursorPosition() {
    return _cursor.position();
}

void File::setCursorPosition(Tui::ZDocumentCursor::Position position) {
    if (_blockSelect) {
        disableBlockSelection();
    }

    _cursor.setPosition(position);

    updateCommands();
    adjustScrollPosition();
    update();
}

QPoint File::getScrollPosition() {
    return {_scrollPositionX, _scrollPositionY.line()};
}

void File::setSaveAs(bool state) {
    _saveAs = state;
}
bool File::isSaveAs() {
    return _saveAs;
}


bool File::setFilename(QString filename) {
    QFileInfo filenameInfo(filename);
    if(filenameInfo.isSymLink()) {
        return setFilename(filenameInfo.symLinkTarget());
    }
    _doc->setFilename(filenameInfo.absoluteFilePath());
    return true;
}

QString File::getFilename() {
    return _doc->filename();
}

bool File::newText(QString filename = "") {
    initText();
    if (filename == "") {
        _doc->setFilename("NEWFILE");
        setSaveAs(true);
    } else {
        setSaveAs(false);
        setFilename(filename);
    }
    modifiedChanged(false);
    return true;
}

bool File::stdinText() {
    _doc->setFilename("STDIN");
    initText();
    modifiedChanged(true);
    setSaveAs(true);
    return true;
}

bool File::initText() {
    _doc->reset();

    _scrollPositionX = 0;
    _scrollPositionY.setLine(0);
    _scrollFineLine = 0;
    _cursor.setPosition({0, 0});
    cursorPositionChanged(0, 0, 0);
    scrollPositionChanged(0, 0);
    update();
    return true;
}

bool File::saveText() {
    // QSaveFile does not take over the user and group. Therefore this should only be used if
    // user and group are the same and the editor also runs under this user.
    QFile file(getFilename());
    //QSaveFile file(getFilename());
    if (file.open(QIODevice::WriteOnly)) {
        _doc->writeTo(&file, _doc->crLfMode());

        //file.commit();
        file.close();
        _doc->markUndoStateAsSaved();
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
    if(file.isSymLink()) {
        QFileInfo path(file.symLinkTarget());
        if(!file.exists() && path.isWritable()) {
            return true;
        }
        return false;
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

        Tui::ZDocumentCursor::Position initialPosition = getAttributes();
        _doc->readFrom(&file, initialPosition, &_cursor);
        file.close();

        if (getWritable()) {
            setSaveAs(false);
        } else {
            setSaveAs(true);
        }

        checkWritable();

        modifiedChanged(false);

        // TODO refactor this
        if (!_jo.isEmpty() && _jo.contains(getFilename())) {
            QJsonObject data = _jo.value(getFilename()).toObject();
            _scrollPositionX = data.value("scrollPositionX").toInt();
            _scrollPositionY.setLine(data.value("scrollPositionY").toInt());
        }
        _scrollFineLine = 0;
        adjustScrollPosition();

#ifdef SYNTAX_HIGHLIGHTING
        _syntaxHighlightDefinition = _syntaxHighlightRepo.definitionForFileName(getFilename());
        syntaxHighlightDefinition();
#endif

        return true;
    }
    return false;
}

void File::cut() {
    copy();
    delSelect();
    updateCommands();
    adjustScrollPosition();
}

void File::cutline() {
    resetSelect();
    _cursor.moveToStartOfLine();
    _cursor.moveToEndOfLine(true);
    cut();
    updateCommands();
    adjustScrollPosition();
}

void File::deleteLine() {
    auto undoGroup = _doc->startUndoGroup(&_cursor);
    if (_cursor.hasSelection() || hasBlockSelection() || hasMultiInsert()) {
        const auto [start, end] = getSelectLinesSort();
        resetSelect();
        _cursor.setPosition({0,end});
        _cursor.moveToEndOfLine();
        _cursor.setPosition({0,start}, true);
        _cursor.removeSelectedText();
    }
    _cursor.deleteLine();
    updateCommands();
    adjustScrollPosition();
}

void File::copy() {
    if (hasBlockSelection()) {
        Clipboard *clipboard = findFacet<Clipboard>();

        const int firstSelectBlockLine = std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line());
        const int lastSelectBlockLine = std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line());
        const int firstSelectBlockColumn = std::min(_blockSelectStartColumn, _blockSelectEndColumn);
        const int lastSelectBlockColumn = std::max(_blockSelectStartColumn, _blockSelectEndColumn);

        QString selectedText;
        for (int line = firstSelectBlockLine; line < _doc->lineCount() && line <= lastSelectBlockLine; line++) {
            Tui::ZTextLayout laySel = getTextLayoutForLineWithoutWrapping(line);
            Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

            const int selFirstCodeUnitInLine = tlrSel.xToCursor(firstSelectBlockColumn);
            const int selLastCodeUnitInLine = tlrSel.xToCursor(lastSelectBlockColumn);
            selectedText += _doc->line(line).mid(selFirstCodeUnitInLine, selLastCodeUnitInLine - selFirstCodeUnitInLine);
            if (line != lastSelectBlockLine) {
                selectedText += "\n";
            }
        }
        clipboard->setClipboard(selectedText);
        _cmdPaste->setEnabled(true);
    } else if (_cursor.hasSelection()) {
        Clipboard *clipboard = findFacet<Clipboard>();
        clipboard->setClipboard(_cursor.selectedText());
        _cmdPaste->setEnabled(true);
    }
}

void File::paste() {
    auto undoGroup = _doc->startUndoGroup(&_cursor);
    Clipboard *clipboard = findFacet<Clipboard>();
    if (clipboard->getClipboard().size()) {
        insertAtCursorPosition(clipboard->getClipboard());
        adjustScrollPosition();
    }
    _doc->clearCollapseUndoStep();
}

bool File::isInsertable() {
    Clipboard *clipboard = findFacet<Clipboard>();
    QString _clipboard = clipboard->getClipboard();
    return !_clipboard.isEmpty();
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
}

bool File::setTabsize(int tab) {
    this->_tabsize = std::max(1, tab);
    return true;
}

int File::getTabsize() {
    return this->_tabsize;
}

void File::setTabOption(bool tab) {
    //false is space, true is real \t
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

void File::setRightMarginHint(int hint) {
    if (hint < 1) {
        _rightMarginHint = 0;
    } else {
        _rightMarginHint = hint;
    }
}

int File::rightMarginHint() const {
    return _rightMarginHint;
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

void File::setWrapOption(Tui::ZTextOption::WrapMode wrap) {
    _wrapOption = wrap;
    if (_wrapOption) {
        _scrollPositionX = 0;
    }
    adjustScrollPosition();
}

Tui::ZTextOption::WrapMode File::getWrapOption() {
    return _wrapOption;
}

QPair<int,int> File::getSelectLinesSort() {
    auto lines = getSelectLines();
    return {std::min(lines.first, lines.second), std::max(lines.first, lines.second)};
}

QPair<int,int> File::getSelectLines() {
    int startY;
    int endeY;

    if (hasBlockSelection() || hasMultiInsert()) {
        startY = _blockSelectStartLine->line();
        endeY = _blockSelectEndLine->line();
    } else {
        const auto [startCodeUnit, startLine] = _cursor.anchor();
        const auto [endCodeUnit, endLine] = _cursor.position();
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
    resetSelect();
    if(startY > endY) {
        _cursor.setPosition({_doc->lineCodeUnits(startY), startY});
        _cursor.setPosition({0, endY}, true);
    } else {
        _cursor.setPosition({0, startY});
        _cursor.setPosition({_doc->lineCodeUnits(endY), endY}, true);
    }
}

void File::resetSelect() {
    if (_blockSelect) {
        disableBlockSelection();
    }

    _cursor.clearSelection();
    setSelectMode(false);

    _cmdCopy->setEnabled(false);
    _cmdCut->setEnabled(false);
}

QString File::getSelectText() {
    QString selectText = "";
    if (hasBlockSelection()) {
        const int firstSelectBlockLine = std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line());
        const int lastSelectBlockLine = std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line());
        const int firstSelectBlockColumn = std::min(_blockSelectStartColumn, _blockSelectEndColumn);
        const int lastSelectBlockColumn = std::max(_blockSelectStartColumn, _blockSelectEndColumn);

        for (int line = firstSelectBlockLine; line < _doc->lineCount() && line <= lastSelectBlockLine; line++) {
            Tui::ZTextLayout laySel = getTextLayoutForLineWithoutWrapping(line);
            Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

            const int selFirstCodeUnitInLine = tlrSel.xToCursor(firstSelectBlockColumn);
            const int selLastCodeUnitInLine = tlrSel.xToCursor(lastSelectBlockColumn);
            selectText += _doc->line(line).mid(selFirstCodeUnitInLine, selLastCodeUnitInLine - selFirstCodeUnitInLine);
            if (line != lastSelectBlockLine) {
                selectText += "\n";
            }
        }
    } else if (_cursor.hasSelection()) {
        selectText = _cursor.selectedText();
    }
    return selectText;
}

bool File::isSelect() {
    return hasBlockSelection() || hasMultiInsert() || _cursor.hasSelection();
}

void File::selectAll() {
    resetSelect();
    _cursor.selectAll();
    updateCommands();
    adjustScrollPosition();
}

bool File::delSelect() {
    auto undoGroup = _doc->startUndoGroup(&_cursor);

    if (!hasBlockSelection() && !hasMultiInsert() && !_cursor.hasSelection()) {
        return false;
    }

    if(_blockSelect) {
        if (hasBlockSelection()) {
            blockSelectRemoveSelectedAndConvertToMultiInsert();
        }

        const int newCursorLine = _blockSelectEndLine->line();
        const int newCursorColumn = _blockSelectStartColumn;

        Tui::ZTextLayout lay = getTextLayoutForLineWithoutWrapping(newCursorLine);
        Tui::ZTextLineRef tlr = lay.lineAt(0);

        _blockSelect = false;
        _blockSelectStartLine.reset();
        _blockSelectEndLine.reset();
        _blockSelectStartColumn = _blockSelectEndColumn = -1;

        setCursorPosition({tlr.xToCursor(newCursorColumn), newCursorLine});
    } else {
        _cursor.removeSelectedText();
    }
    adjustScrollPosition();
    updateCommands();
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
    overwriteChanged(_overwrite);
}

bool File::isOverwrite() {
    return _overwrite;
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

    const auto [cursorCodeUnit, cursorLine] = _cursor.position();
    _blockSelectStartLine.emplace(_doc.get(), cursorLine);
    _blockSelectEndLine.emplace(_doc.get(), cursorLine);

    Tui::ZTextLayout lay = getTextLayoutForLineWithoutWrapping(cursorLine);
    Tui::ZTextLineRef tlr = lay.lineAt(0);

    _blockSelectStartColumn = _blockSelectEndColumn = tlr.cursorToX(cursorCodeUnit, Tui::ZTextLayout::Leading);
}

void File::disableBlockSelection() {
    // pre-condition: _blockSelect = true
    _blockSelect = false;

    // just to be sure
    const int cursorLine = std::min(_blockSelectEndLine->line(), _doc->lineCount() - 1);
    const int cursorColumn = _blockSelectEndColumn;

    _blockSelectStartLine.reset();
    _blockSelectEndLine.reset();
    _blockSelectStartColumn = _blockSelectEndColumn = -1;

    Tui::ZTextLayout lay = getTextLayoutForLineWithoutWrapping(cursorLine);
    Tui::ZTextLineRef tlr = lay.lineAt(0);
    _cursor.setPosition({tlr.xToCursor(cursorColumn), cursorLine});
}

void File::blockSelectRemoveSelectedAndConvertToMultiInsert() {
    const int firstSelectBlockLine = std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line());
    const int lastSelectBlockLine = std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line());
    const int firstSelectBlockColumn = std::min(_blockSelectStartColumn, _blockSelectEndColumn);
    const int lastSelectBlockColumn = std::max(_blockSelectStartColumn, _blockSelectEndColumn);

    for (int line = firstSelectBlockLine; line < _doc->lineCount() && line <= lastSelectBlockLine; line++) {
        Tui::ZTextLayout laySel = getTextLayoutForLineWithoutWrapping(line);
        Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

        const int selFirstCodeUnitInLine = tlrSel.xToCursor(firstSelectBlockColumn);
        const int selLastCodeUnitInLine = tlrSel.xToCursor(lastSelectBlockColumn);

        Tui::ZDocumentCursor cur = createCursor();
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

    for (int line = firstSelectBlockLine; line < _doc->lineCount() && line <= lastSelectBlockLine; line++) {
        Tui::ZTextLayout laySel = getTextLayoutForLineWithoutWrapping(line);
        Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

        const int codeUnitInLine = tlrSel.xToCursor(column);

        Tui::ZDocumentCursor cur = createCursor();
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

            Tui::ZTextLayout layNew = getTextLayoutForLineWithoutWrapping(line);
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
        if (cursorCodeUnit < _doc->lineCodeUnits(cursorLine)) {
            cur.deleteCharacter();
        }
    });
}

void File::multiInsertDeleteWord() {
    // pre-condition: hasMultiInsert() = true
    multiInsertForEachCursor(mi_skip_short_lines, [&](Tui::ZDocumentCursor &cur) {
        const auto [cursorCodeUnit, cursorLine] = cur.position();
        if (cursorCodeUnit < _doc->lineCodeUnits(cursorLine)) {
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

void File::setSearchText(QString searchText) {
    int gen = ++(*searchGeneration);
    _searchText = searchText.replace("\\n","\n");
    _searchText = searchText.replace("\\t","\t");
    searchTextChanged(_searchText);

    if(searchText == "") {
        _cmdSearchNext->setEnabled(false);
        _cmdSearchPrevious->setEnabled(false);
        return;
    } else {
        _cmdSearchNext->setEnabled(true);
        _cmdSearchPrevious->setEnabled(true);
    }

    SearchCountSignalForwarder *searchCountSignalForwarder = new SearchCountSignalForwarder();
    QObject::connect(searchCountSignalForwarder, &SearchCountSignalForwarder::searchCount, this, &File::searchCountChanged);

    QtConcurrent::run([searchCountSignalForwarder](Tui::ZDocumentSnapshot snap, QString searchText, Qt::CaseSensitivity caseSensitivity, int gen, std::shared_ptr<std::atomic<int>> searchGen) {
        SearchCount sc;
        QObject::connect(&sc, &SearchCount::searchCount, searchCountSignalForwarder, &SearchCountSignalForwarder::searchCount);
        sc.run(snap, searchText, caseSensitivity, gen, searchGen);
        searchCountSignalForwarder->deleteLater();
    }, _doc->snapshot(), _searchText, _searchCaseSensitivity, gen, searchGeneration);
}

void File::setSearchCaseSensitivity(Qt::CaseSensitivity searchCaseSensitivity) {
    _searchCaseSensitivity = searchCaseSensitivity;
}

void File::setReplaceText(QString replaceText) {
   _replaceText = replaceText;
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
    auto undoGroup = _doc->startUndoGroup(&_cursor);

    if (hasBlockSelection() || hasMultiInsert() || _cursor.hasSelection()) {
        auto undoGroup = _doc->startUndoGroup(&_cursor);

        if (_blockSelect) {
            disableBlockSelection();
        }

        QString text;

        bool esc = false;
        for (QChar ch: _replaceText) {
            if (esc) {
                switch (ch.unicode()) {
                    case 'n':
                        text += "\n";
                        break;
                    case 't':
                        text += "\t";
                        break;
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

        _cursor.insertText(text);

        adjustScrollPosition();
    }
}

void File::runSearch(bool direction) {
    if (_searchText != "") {
        if (_searchNextFuture) {
            _searchNextFuture->cancel();
            _searchNextFuture.reset();
        }

        const bool effectiveDirection = direction ^ _searchDirection;

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

        QObject::connect(watcher, &QFutureWatcher<Tui::ZDocumentFindAsyncResult>::finished, this, [this, watcher] {
            if (!watcher->isCanceled()) {
                Tui::ZDocumentFindAsyncResult res = watcher->future().result();
                if (res.anchor() != res.cursor()) { // has a match?
                    // Get rid of block selections and multi insert.
                    resetSelect();

                    _cursor.setPosition(res.anchor());
                    _cursor.setPosition(res.cursor(), true);

                    updateCommands();

                    const auto [currentCodeUnit, currentLine] = _cursor.position();

                    if (currentLine - 1 > 0) {
                        _scrollPositionY.setLine(currentLine - 1);
                    }
                    adjustScrollPosition();
                }
            }
            watcher->deleteLater();
        });

        if (_searchReg) {
            _searchNextFuture.emplace(_doc->findAsync(QRegularExpression(_searchText), _cursor, flags));
        } else {
            _searchNextFuture.emplace(_doc->findAsync(_searchText, _cursor, flags));
        }
        watcher->setFuture(*_searchNextFuture);
    }
}

int File::replaceAll(QString searchText, QString replaceText) {
    setSearchText(searchText);
    setReplaceText(replaceText);

    // Get rid of block selections and multi insert.
    resetSelect();

    if (searchText.isEmpty()) {
        return 0;
    }

    auto undoGroup = _doc->startUndoGroup(&_cursor);
    int counter = 0;

    Tui::ZDocument::FindFlags flags;
    if (_searchCaseSensitivity == Qt::CaseSensitive) {
        flags |= Tui::ZDocument::FindFlag::FindCaseSensitively;
    }

    _cursor.setPosition({0, 0});
    while (true) {
        Tui::ZDocumentCursor found = _cursor;
        if (_searchReg) {
            found = _doc->findSync(QRegularExpression(_searchText), _cursor, flags);
        } else {
            found = _doc->findSync(_searchText, _cursor, flags);
        }
        if (!found.hasSelection()) {  // has no match?
            break;
        }
        _cursor.setPosition(found.anchor());
        _cursor.setPosition(found.position(), true);

        setReplaceSelected();
        counter++;
    }

    const auto [currentCodeUnit, currentLine] = _cursor.position();
    if (currentLine - 1 > 0) {
        _scrollPositionY.setLine(currentLine - 1);
    }

    adjustScrollPosition();
    return counter;
}

Tui::ZTextOption File::getTextOption(bool lineWithCursor) {
    Tui::ZTextOption option;
    option.setWrapMode(_wrapOption);
    option.setTabStopDistance(_tabsize);

    Tui::ZTextOption::Flags flags;
    if (formattingCharacters()) {
        flags |= Tui::ZTextOption::ShowTabsAndSpaces;
    }
    if (colorTabs()) {
        flags |= Tui::ZTextOption::ShowTabsAndSpacesWithColors;
        if (!getTabOption()) {
            option.setTabColor([] (int pos, int size, int hidden, const Tui::ZTextStyle &base, const Tui::ZTextStyle &formating, const Tui::ZFormatRange* range) -> Tui::ZTextStyle {
                (void)formating;
                if (range) {
                    if (pos == hidden) {
                        return { range->format().foregroundColor(), {0xff, 0x80, 0xff} };
                    }
                    return { range->format().foregroundColor(), {0xff, 0xb0, 0xff} };
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
                if (range) {
                    if (pos == hidden) {
                        return { range->format().foregroundColor(), {0x80, 0xff, 0xff} };
                    }
                    return { range->format().foregroundColor(), {0xb0, 0xff, 0xff} };
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
        flags |= Tui::ZTextOption::ShowTabsAndSpacesWithColors;
        if (!lineWithCursor) {
            option.setTrailingWhitespaceColor([] (const Tui::ZTextStyle &base, const Tui::ZTextStyle &formating, const Tui::ZFormatRange* range) -> Tui::ZTextStyle {
                (void)formating;
                if (range) {
                    return { range->format().foregroundColor(), {0xff, 0x80, 0x80} };
                }
                return { base.foregroundColor(), {0x80, 0, 0} };
            });
        }
    }
    option.setFlags(flags);
    return option;
}

Tui::ZTextLayout File::getTextLayoutForLine(const Tui::ZTextOption &option, int line) {
    Tui::ZTextLayout lay(terminal()->textMetrics(), _doc->line(line));
    lay.setTextOption(option);
    if (_wrapOption) {
        lay.doLayout(std::max(rect().width() - shiftLinenumber(), 0));
    } else {
        lay.doLayout(std::numeric_limits<unsigned short>::max() - 1);
    }
    return lay;
}

bool File::highlightBracketFind() {
    QString openBracket = "{[(<";
    QString closeBracket = "}])>";

    if(getHighlightBracket()) {
        const auto [cursorCodeUnit, cursorLine] = _cursor.position();

        if (cursorCodeUnit < _doc->lineCodeUnits(cursorLine)) {
            for(int i=0; i<openBracket.size(); i++) {
                if (_doc->line(cursorLine)[cursorCodeUnit] == openBracket[i]) {
                    int y = 0;
                    int counter = 0;
                    int startX = cursorCodeUnit + 1;
                    for (int line = cursorLine; y++ < rect().height() && line < _doc->lineCount(); line++) {
                        for(; startX < _doc->lineCodeUnits(line); startX++) {
                            if (_doc->line(line)[startX] == openBracket[i]) {
                                counter++;
                            } else if (_doc->line(line)[startX] == closeBracket[i]) {
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

                if (_doc->line(cursorLine)[cursorCodeUnit] == closeBracket[i]) {
                    int counter = 0;
                    int startX = cursorCodeUnit - 1;
                    for (int line = cursorLine; line >= 0;) {
                        for(; startX >= 0; startX--) {
                            if (_doc->line(line)[startX] == closeBracket[i]) {
                                counter++;
                            } else if (_doc->line(line)[startX] == openBracket[i]) {
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
                            startX = _doc->lineCodeUnits(line) - 1;
                        }
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
        return QString::number(_doc->lineCount()).size() + 1;
    }
    return 0;
}

Tui::ZTextLayout File::getTextLayoutForLineWithoutWrapping(int line) {
    Tui::ZTextOption option = getTextOption(false);
    option.setWrapMode(Tui::ZTextOption::NoWrap);
    return getTextLayoutForLine(option, line);
}

Tui::ZDocumentCursor File::createCursor() {
    return Tui::ZDocumentCursor(_doc.get(), [this](int line, bool wrappingAllowed) {
        Tui::ZTextLayout lay(_textMetrics, _doc->line(line));
        Tui::ZTextOption option;
        option.setTabStopDistance(_tabsize);
        if (wrappingAllowed) {
            option.setWrapMode(_wrapOption);
            lay.setTextOption(option);
            lay.doLayout(std::max(rect().width() - shiftLinenumber(), 0));
        } else {
            lay.setTextOption(option);
            lay.doLayout(65000);
        }
        return lay;
    });
}

void File::paintEvent(Tui::ZPaintEvent *event) {
    Tui::ZColor fg = getColor("chr.editFg");
    Tui::ZColor bg = getColor("chr.editBg");
    highlightBracketFind();

    Tui::ZColor marginMarkBg = [](Tui::ZColorHSV base)
        {
            return Tui::ZColor::fromHsv(base.hue(), base.saturation(), base.value() * 0.75);
        }(bg.toHsv());


    std::optional<Tui::ZImage> leftOfMarginBuffer;
    std::optional<Tui::ZPainter> painterLeftOfMargin;

    auto *painter = event->painter();
    if (_rightMarginHint) {
        painter->clearRect(0, 0, -_scrollPositionX + shiftLinenumber() + _rightMarginHint, rect().height(), fg, bg);
        painter->clearRect(-_scrollPositionX + shiftLinenumber() + _rightMarginHint, 0, Tui::tuiMaxSize, rect().height(), fg, marginMarkBg);

        // One extra column to account for double wide character at last position
        leftOfMarginBuffer.emplace(terminal(), _rightMarginHint + 1, 1);
        painterLeftOfMargin.emplace(leftOfMarginBuffer->painter());
    } else {
        painter->clear(fg, bg);
    }

    Tui::ZTextOption option = getTextOption(false);

    const int firstSelectBlockLine = _blockSelect ? std::min(_blockSelectStartLine->line(), _blockSelectEndLine->line()) : 0;
    const int lastSelectBlockLine = _blockSelect ? std::max(_blockSelectStartLine->line(), _blockSelectEndLine->line()) : 0;
    const int firstSelectBlockColumn = std::min(_blockSelectStartColumn, _blockSelectEndColumn);
    const int lastSelectBlockColumn = std::max(_blockSelectStartColumn, _blockSelectEndColumn);

    Tui::ZDocumentCursor::Position startSelectCursor(-1, -1);
    Tui::ZDocumentCursor::Position endSelectCursor(-1, -1);

    if (_cursor.hasSelection()) {
        startSelectCursor = _cursor.selectionStartPos();
        endSelectCursor = _cursor.selectionEndPos();
    }

    QVector<Tui::ZFormatRange> highlights;
    const Tui::ZTextStyle base{fg, bg};
    const Tui::ZTextStyle baseInMargin{fg, marginMarkBg};
    const Tui::ZTextStyle formatingChar{Tui::Colors::darkGray, bg};
    const Tui::ZTextStyle formatingCharInMargin{Tui::Colors::darkGray, marginMarkBg};
    const Tui::ZTextStyle selected{Tui::Colors::darkGray,fg,Tui::ZTextAttribute::Bold};
    const Tui::ZTextStyle blockSelected{fg,Tui::Colors::lightGray,Tui::ZTextAttribute::Blink | Tui::ZTextAttribute::Italic};
    const Tui::ZTextStyle blockSelectedFormatingChar{Tui::Colors::darkGray, Tui::Colors::lightGray, Tui::ZTextAttribute::Blink};
    const Tui::ZTextStyle selectedFormatingChar{Tui::Colors::darkGray, fg};

    const auto [cursorCodeUnit, cursorLineReal] = _cursor.position();
    const int cursorLine = _blockSelect ? _blockSelectEndLine->line() : cursorLineReal;

    QString strlinenumber;
    int y = -_scrollFineLine;
    int tmpLastLineWidth = 0;
    for (int line = _scrollPositionY.line(); y < rect().height() && line < _doc->lineCount(); line++) {
        const bool multiIns = hasMultiInsert();
        const bool cursorAtEndOfCurrentLine = [&, cursorCodeUnit=cursorCodeUnit] {
            if (_blockSelect) {
                if (multiIns && firstSelectBlockLine <= line && line <= lastSelectBlockLine) {
                    auto testLayout = getTextLayoutForLineWithoutWrapping(line);
                    auto testLayoutLine = testLayout.lineAt(0);
                    return testLayoutLine.width() == firstSelectBlockColumn;
                }
                return false;
            } else {
                return line == cursorLine && _doc->lineCodeUnits(cursorLine) == cursorCodeUnit;
            }
        }();
        Tui::ZTextLayout lay = cursorAtEndOfCurrentLine ? getTextLayoutForLine(getTextOption(true), line)
                                                        : getTextLayoutForLine(option, line);

        // highlights
        highlights.clear();

#ifdef SYNTAX_HIGHLIGHTING
        if (syntaxHighlightingActive()) {
            if (_doc->lineUserData(line)) {
                auto extraData = std::static_pointer_cast<const ExtraData>(_doc->lineUserData(line));
                if (line == cursorLine && extraData->lineRevision != _doc->lineRevision(line)) {
                    // avoid glitches when using the cursor to edit lines
                    // the state can still be stale, but much more edits can be done without visible glitches
                    // with stale state.
                    highlights += std::get<1>(_syntaxHighlightExporter.highlightLineWrap(_doc->line(line), extraData->stateBegin));
                } else {
                    highlights += extraData->highlights;
                }
            }
        }
#endif

        // search matches
        if(_searchText != "") {
            int found = -1;
            if(_searchReg) {
                QRegularExpression rx(_searchText);
                QRegularExpressionMatchIterator i = rx.globalMatch(_doc->line(line));
                while (i.hasNext()) {
                    QRegularExpressionMatch match = i.next();
                    if(match.capturedLength() > 0) {
                        highlights.append(Tui::ZFormatRange{match.capturedStart(), match.capturedLength(), {Tui::Colors::darkGray,{0xff,0xdd,0},Tui::ZTextAttribute::Bold}, selectedFormatingChar});
                    }
                }
            } else {
                while ((found = _doc->line(line).indexOf(_searchText, found + 1, _searchCaseSensitivity)) != -1) {
                    highlights.append(Tui::ZFormatRange{found, _searchText.size(), {Tui::Colors::darkGray,{0xff,0xdd,0},Tui::ZTextAttribute::Bold}, selectedFormatingChar});
                }
            }
        }
        if(_bracketX >= 0) {
            if (_bracketY == line) {
                highlights.append(Tui::ZFormatRange{_bracketX, 1, {Tui::Colors::cyan, bg,Tui::ZTextAttribute::Bold}, selectedFormatingChar});
            }
            if (line == cursorLine) {
                highlights.append(Tui::ZFormatRange{cursorCodeUnit, 1, {Tui::Colors::cyan, bg,Tui::ZTextAttribute::Bold}, selectedFormatingChar});
            }
        }

        // selection
        if (_blockSelect) {
            if (line >= firstSelectBlockLine && line <= lastSelectBlockLine) {
                Tui::ZTextLayout laySel = getTextLayoutForLineWithoutWrapping(line);
                Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

                if (firstSelectBlockColumn == lastSelectBlockColumn) {
                    highlights.append(Tui::ZFormatRange{tlrSel.xToCursor(firstSelectBlockColumn), 1, blockSelected, blockSelectedFormatingChar});
                } else {
                    const int selFirstCodeUnitInLine = tlrSel.xToCursor(firstSelectBlockColumn);
                    const int selLastCodeUnitInLine = tlrSel.xToCursor(lastSelectBlockColumn);
                    highlights.append(Tui::ZFormatRange{selFirstCodeUnitInLine, selLastCodeUnitInLine - selFirstCodeUnitInLine, selected, selectedFormatingChar});
                }
            }
        } else {
            if (line > startSelectCursor.line && line < endSelectCursor.line) {
                // whole line
                highlights.append(Tui::ZFormatRange{0, _doc->lineCodeUnits(line), selected, selectedFormatingChar});
            } else if (line > startSelectCursor.line && line == endSelectCursor.line) {
                // selection ends on this line
                highlights.append(Tui::ZFormatRange{0, endSelectCursor.codeUnit, selected, selectedFormatingChar});
            } else if (line == startSelectCursor.line && line < endSelectCursor.line) {
                // selection starts on this line
                highlights.append(Tui::ZFormatRange{startSelectCursor.codeUnit, _doc->lineCodeUnits(line) - startSelectCursor.codeUnit, selected, selectedFormatingChar});
            } else if (line == startSelectCursor.line && line == endSelectCursor.line) {
                // selection is contained in this line
                highlights.append(Tui::ZFormatRange{startSelectCursor.codeUnit, endSelectCursor.codeUnit - startSelectCursor.codeUnit, selected, selectedFormatingChar});
            }
        }
        if (_rightMarginHint && lay.maximumWidth() > _rightMarginHint) {
            if (lay.lineCount() > leftOfMarginBuffer->height() && leftOfMarginBuffer->height() < rect().height()) {
                leftOfMarginBuffer.emplace(terminal(), _rightMarginHint + 1,
                                           std::min(std::max(leftOfMarginBuffer->height() * 2, lay.lineCount()), rect().height()));
                painterLeftOfMargin.emplace(leftOfMarginBuffer->painter());
            }

            lay.draw(*painter, {-_scrollPositionX + shiftLinenumber(), y}, baseInMargin, &formatingCharInMargin, highlights);
            painterLeftOfMargin->clearRect(0, 0, _rightMarginHint + 1, lay.lineCount(), base.foregroundColor(), base.backgroundColor());
            lay.draw(*painterLeftOfMargin, {0, 0}, base, &formatingChar, highlights);
            painter->drawImageWithTiling(-_scrollPositionX + shiftLinenumber(), y,
                                              *leftOfMarginBuffer, 0, 0, _rightMarginHint, lay.lineCount(),
                                              Tui::ZTilingMode::NoTiling, Tui::ZTilingMode::Put);
        } else {
            if (_formattingCharacters) {
                lay.draw(*painter, {-_scrollPositionX + shiftLinenumber(), y}, base, &formatingChar, highlights);
            } else {
                lay.draw(*painter, {-_scrollPositionX + shiftLinenumber(), y}, base, &base, highlights);
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
                        markStyle = blockSelected;
                    }
                    const int firstColumnAfterLineBreakMarker = std::max(lastLine.width() + 1, firstSelectBlockColumn);
                    painter->clearRect(-_scrollPositionX + firstColumnAfterLineBreakMarker + shiftLinenumber(),
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
                    markStyle = blockSelected;
                }
                painter->writeWithAttributes(-_scrollPositionX + lastLine.width() + shiftLinenumber(), y + lastLine.y(), QStringLiteral("¶"),
                                         markStyle.foregroundColor(), markStyle.backgroundColor(), markStyle.attributes());
            } else {
                Tui::ZTextStyle markStyle = selected;
                if (multiIns) {
                    markStyle = blockSelected;
                }
                painter->clearRect(-_scrollPositionX + lastLine.width() + shiftLinenumber(), y + lastLine.y(), 1, 1, markStyle.foregroundColor(), markStyle.backgroundColor());
            }
        } else if (formattingCharacters()) {
            const Tui::ZTextStyle &markStyle = (_rightMarginHint && lastLine.width() > _rightMarginHint) ? formatingCharInMargin : formatingChar;
            painter->writeWithAttributes(-_scrollPositionX + lastLine.width() + shiftLinenumber(), y + lastLine.y(), QStringLiteral("¶"),
                                         markStyle.foregroundColor(), markStyle.backgroundColor(), markStyle.attributes());
        }

        if (cursorLine == line) {
            if (focus()) {
                if (_blockSelect) {
                    painter->setCursor(-_scrollPositionX + shiftLinenumber() + _blockSelectEndColumn, y);
                } else {
                    lay.showCursor(*painter, {-_scrollPositionX + shiftLinenumber(), y}, cursorCodeUnit);
                }
            }
        }
        // linenumber
        if (_linenumber) {
            for (int i = lay.lineCount() - 1; i > 0; i--) {
                painter->writeWithColors(0, y + i, QString(" ").repeated(shiftLinenumber()), getColor("chr.linenumberFg"), getColor("chr.linenumberBg"));
            }
            strlinenumber = QString::number(line + 1) + QString(" ").repeated(shiftLinenumber() - QString::number(line + 1).size());
            int lineNumberY = y;
            if (y < 0) {
                strlinenumber.replace(' ', '^');
                lineNumberY = 0;
            }
            if (line == cursorLine) {
                painter->writeWithAttributes(0, lineNumberY, strlinenumber, getColor("chr.linenumberFg"), getColor("chr.linenumberBg"), Tui::ZTextAttribute::Bold);
            } else {
                painter->writeWithColors(0, lineNumberY, strlinenumber, getColor("chr.linenumberFg"), getColor("chr.linenumberBg"));
            }
        }
        y += lay.lineCount();
    }
    if (_doc->newlineAfterLastLineMissing()) {
        if (formattingCharacters() && y < rect().height() && _scrollPositionX == 0) {
            const Tui::ZTextStyle &markStyle = (_rightMarginHint && tmpLastLineWidth > _rightMarginHint) ? formatingCharInMargin : formatingChar;

            painter->writeWithAttributes(-_scrollPositionX + tmpLastLineWidth + shiftLinenumber(), y-1, "♦", markStyle.foregroundColor(), markStyle.backgroundColor(), markStyle.attributes());
        }
        painter->writeWithAttributes(0 + shiftLinenumber(), y, "\\ No newline at end of file", formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
    } else {
        if (formattingCharacters() && y < rect().height() && _scrollPositionX == 0) {
            painter->writeWithAttributes(0 + shiftLinenumber(), y, "♦", formatingChar.foregroundColor(), formatingChar.backgroundColor(), formatingChar.attributes());
        }
    }
}

void File::addTabAt(Tui::ZDocumentCursor &cur) {
    auto undoGroup = _doc->startUndoGroup(&_cursor);

    if (getTabOption()) {
        cur.insertText("\t");
    } else {
        Tui::ZTextLayout lay = getTextLayoutForLineWithoutWrapping(cur.position().line);
        Tui::ZTextLineRef tlr = lay.lineAt(0);
        const int colum = tlr.cursorToX(cur.position().codeUnit, Tui::ZTextLayout::Leading);
        const int remainingTabWidth = getTabsize() - colum % getTabsize();
        cur.insertText(QStringLiteral(" ").repeated(remainingTabWidth));
    }
}

int File::getVisibleLines() {
    if(getWrapOption()) {
        return std::max(1, geometry().height() - 2);
    }
    return std::max(1, geometry().height() - 1);
}

void File::appendLine(const QString &line) {
    Tui::ZDocumentCursor cur = createCursor();
    if (_doc->lineCount() == 1 && _doc->lineCodeUnits(0) == 0) {
        cur.insertText(line);
    } else {
        cur.moveToEndOfDocument();
        cur.insertText("\n" + line);
    }
    if(_follow) {
        _cursor.moveToEndOfDocument();
    }
    adjustScrollPosition();
}

void File::insertAtCursorPosition(const QString &str) {
    auto undoGroup = _doc->startUndoGroup(&_cursor);

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

            for (int line = firstSelectBlockLine; line < _doc->lineCount() && line <= lastSelectBlockLine; line++) {
                if (source.size() != 1 && sourceLine >= source.size()) {
                    break;
                }

                Tui::ZTextLayout laySel = getTextLayoutForLineWithoutWrapping(line);
                Tui::ZTextLineRef tlrSel = laySel.lineAt(0);

                Tui::ZDocumentCursor cur = createCursor();

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
                    Tui::ZTextLayout layNew = getTextLayoutForLineWithoutWrapping(line);
                    Tui::ZTextLineRef tlrNew = layNew.lineAt(0);
                    _blockSelectStartColumn = _blockSelectEndColumn = tlrNew.cursorToX(cur.position().codeUnit, Tui::ZTextLayout::Leading);
                }
            }
        }
    } else {
        // Inserting might adjust the scroll position, so save it here and restore it later.
        const int line = _scrollPositionY.line();
        _cursor.insertText(str);
        _scrollPositionY.setLine(line);
    }
    adjustScrollPosition();
}

void File::sortSelecedLines() {
    if (hasBlockSelection() || hasMultiInsert() || _cursor.hasSelection()) {
        const auto [startLine, endLine] = getSelectLines();
        auto lines = getSelectLinesSort();
        _doc->sortLines(lines.first, lines.second + 1, &_cursor);
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
        if (!_cursor.atLineStart() && terminal()) {
            if (_blockSelect) {
                disableBlockSelection();
            }
            _cursor.setPosition(_cursor.position());
        }
    }

    return ZWidget::event(event);
}

void File::pasteEvent(Tui::ZPasteEvent *event) {
    QString text = event->text();
    if(_formattingCharacters) {
        text.replace(QString("·"), QString(" "));
        text.replace(QString("→"), QString(" "));
        text.replace(QString("¶"), QString(""));
    }
    text.replace(QString("\r\n"), QString('\n'));
    text.replace(QString('\r'), QString('\n'));

    insertAtCursorPosition(text);
    _doc->clearCollapseUndoStep();
    adjustScrollPosition();
}

void File::resizeEvent(Tui::ZResizeEvent *event) {
    if (event->size().height() > 0 && event->size().width() > 0) {
        adjustScrollPosition();
    }
}

void File::focusInEvent(Tui::ZFocusEvent *event) {
    Q_UNUSED(event);
    updateCommands();
    if(_searchText == "") {
        _cmdSearchNext->setEnabled(false);
        _cmdSearchPrevious->setEnabled(false);
    } else {
        _cmdSearchNext->setEnabled(true);
        _cmdSearchPrevious->setEnabled(true);
    }
    modifiedChanged(isModified());
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
    if (_doc->filename() == "NEWFILE") {
        return true;
    }
    return false;
}

void File::undo() {
    if (_blockSelect) {
        disableBlockSelection();
    }
    setSelectMode(false);
    _doc->undo(&_cursor);
    adjustScrollPosition();
}

void File::redo() {
    if (_blockSelect) {
        disableBlockSelection();
    }
    setSelectMode(false);
    _doc->redo(&_cursor);
    adjustScrollPosition();
}

void File::keyEvent(Tui::ZKeyEvent *event) {
    auto undoGroup = _doc->startUndoGroup(&_cursor);

    QString text = event->text();
    const bool isAltShift = event->modifiers() == (Qt::AltModifier | Qt::ShiftModifier);
    const bool isAltCtrlShift = event->modifiers() == (Qt::AltModifier | Qt::ControlModifier | Qt::ShiftModifier);

    if(event->key() == Qt::Key_Space && event->modifiers() == 0) {
        text = " ";
    }

    auto delAndClearSelection = [this] {
        //Markierte Zeichen Löschen
        delSelect();
        resetSelect();
        adjustScrollPosition();
    };

    if (event->key() == Qt::Key_Backspace && (event->modifiers() == 0 || event->modifiers() == Qt::ControlModifier)) {
        _detachedScrolling = false;
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
            if (event->modifiers() & Qt::ControlModifier) {
                _cursor.deletePreviousWord();
            } else {
                _cursor.deletePreviousCharacter();
            }
        }
        updateCommands();
        adjustScrollPosition();
    } else if(event->key() == Qt::Key_Delete && (event->modifiers() == 0 || event->modifiers() == Qt::ControlModifier)) {
        _detachedScrolling = false;
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
            if (event->modifiers() & Qt::ControlModifier) {
                _cursor.deleteWord();
            } else {
                if (_cursor.atEnd()) {
                    _doc->setNewlineAfterLastLineMissing(true);
                }
                _cursor.deleteCharacter();
            }
        }
        updateCommands();
        adjustScrollPosition();
    } else if(text.size() && event->modifiers() == 0) {
        _detachedScrolling = false;
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

                if (isOverwrite()) {
                    multiInsertDeleteCharacter();
                }
                multiInsertInsert(text);
            } else {
                if (isOverwrite()
                    && !_cursor.hasSelection()
                    && !_cursor.atLineEnd()) {

                    _cursor.deleteCharacter();
                }
                // Inserting might adjust the scroll position, so save it here and restore it later.
                const int line = _scrollPositionY.line();
                _cursor.insertText(text);
                _scrollPositionY.setLine(line);
            }
            adjustScrollPosition();
        }
        updateCommands();
    } else if (event->text() == "S" && (event->modifiers() == Qt::AltModifier || event->modifiers() == Qt::AltModifier | Qt::ShiftModifier)  && isSelect()) {
        _detachedScrolling = false;
        // Alt + Shift + s sort selected lines
        sortSelecedLines();
        adjustScrollPosition();
        update();
    } else if (event->key() == Qt::Key_Left) {
        _detachedScrolling = false;
        if (isAltShift || isAltCtrlShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            if (_blockSelectEndColumn > 0) {
                const auto mode = event->modifiers() & Qt::ControlModifier ?
                            Tui::ZTextLayout::SkipWords : Tui::ZTextLayout::SkipCharacters;
                Tui::ZTextLayout lay = getTextLayoutForLineWithoutWrapping(_blockSelectEndLine->line());
                Tui::ZTextLineRef tlr = lay.lineAt(0);
                const int codeUnitInLine = tlr.xToCursor(_blockSelectEndColumn);
                if (codeUnitInLine != _doc->lineCodeUnits(_blockSelectEndLine->line())) {
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

            const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
            if (event->modifiers() & Tui::ControlModifier) {
                _cursor.moveWordLeft(extendSelection);
            } else {
                _cursor.moveCharacterLeft(extendSelection);
            }
        }
        updateCommands();
        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if(event->key() == Qt::Key_Right) {
        _detachedScrolling = false;
        if (isAltShift || isAltCtrlShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            const auto mode = event->modifiers() & Qt::ControlModifier ?
                        Tui::ZTextLayout::SkipWords : Tui::ZTextLayout::SkipCharacters;

            Tui::ZTextLayout lay = getTextLayoutForLineWithoutWrapping(_blockSelectEndLine->line());
            Tui::ZTextLineRef tlr = lay.lineAt(0);
            const int codeUnitInLine = tlr.xToCursor(_blockSelectEndColumn);
            if (tlr.cursorToX(codeUnitInLine, Tui::ZTextLayout::Leading) == _blockSelectEndColumn
                && codeUnitInLine != _doc->lineCodeUnits(_blockSelectEndLine->line())) {
                _blockSelectEndColumn = tlr.cursorToX(lay.nextCursorPosition(codeUnitInLine, mode),
                                                      Tui::ZTextLayout::Leading);
            } else {
                _blockSelectEndColumn += 1;
            }
        } else {
            if (_blockSelect) {
                disableBlockSelection();
            }

            const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
            if (event->modifiers() & Tui::ControlModifier) {
                _cursor.moveWordRight(extendSelection);
            } else {
                _cursor.moveCharacterRight(extendSelection);
            }
        }
        updateCommands();
        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Down && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier || isAltShift)) {
        _detachedScrolling = false;
        if (isAltShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            if (_doc->lineCount() - 1 > _blockSelectEndLine->line()) {
                _blockSelectEndLine->setLine(_blockSelectEndLine->line() + 1);
            }
        } else {
            if (_blockSelect) {
                disableBlockSelection();
            }

            const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
            _cursor.moveDown(extendSelection);
        }
        updateCommands();
        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_Up && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier || isAltShift)) {
        _detachedScrolling = false;
        if (isAltShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            if (_blockSelectEndLine->line() > 0) {
                _blockSelectEndLine->setLine(_blockSelectEndLine->line() - 1);
            }
        } else {
            if (_blockSelect) {
                disableBlockSelection();
            }

            const bool extendSelection = event->modifiers() & Qt::ShiftModifier || _selectMode;
            _cursor.moveUp(extendSelection);
        }
        updateCommands();
        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if(event->key() == Qt::Key_Home && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier || isAltShift)) {
        _detachedScrolling = false;
        if (isAltShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            if (_blockSelectEndColumn == 0) {
                int codeUnitInLine = 0;
                for (; codeUnitInLine <= _doc->lineCodeUnits(_blockSelectEndLine->line()) - 1; codeUnitInLine++) {
                    if(_doc->line(_blockSelectEndLine->line())[codeUnitInLine] != ' ' && _doc->line(_blockSelectEndLine->line())[codeUnitInLine] != '\t') {
                        break;
                    }
                }

                Tui::ZTextLayout lay = getTextLayoutForLineWithoutWrapping(_blockSelectEndLine->line());
                Tui::ZTextLineRef tlr = lay.lineAt(0);

                _blockSelectEndColumn = tlr.cursorToX(codeUnitInLine, Tui::ZTextLayout::Leading);
            } else {
                _blockSelectEndColumn = 0;
            }
        } else {
            if (_blockSelect) {
                disableBlockSelection();
            }

            const bool extendSelection = event->modifiers() == Qt::ShiftModifier || _selectMode;

            if (_cursor.atLineStart()) {
                _cursor.moveToStartIndentedText(extendSelection);
            } else {
                _cursor.moveToStartOfLine(extendSelection);
            }
        }
        updateCommands();
        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if(event->key() == Qt::Key_Home && (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) )) {
        _detachedScrolling = false;
        if (_blockSelect) {
            disableBlockSelection();
        }

        if(event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) || _selectMode) {
            _cursor.moveToStartOfDocument(true);
        } else {
            _cursor.moveToStartOfDocument(false);
        }

        updateCommands();
        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if(event->key() == Qt::Key_End && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier || isAltShift)) {
        _detachedScrolling = false;
        if (isAltShift) {
            if (!_blockSelect) {
                activateBlockSelection();
            }

            Tui::ZTextLayout lay = getTextLayoutForLineWithoutWrapping(_blockSelectEndLine->line());
            Tui::ZTextLineRef tlr = lay.lineAt(0);

            _blockSelectEndColumn = tlr.cursorToX(_doc->lineCodeUnits(_blockSelectEndLine->line()), Tui::ZTextLayout::Leading);
        } else {
            if (_blockSelect) {
                disableBlockSelection();
            }

            if (event->modifiers() == Qt::ShiftModifier || _selectMode) {
                _cursor.moveToEndOfLine(true);
            } else {
                _cursor.moveToEndOfLine(false);
            }
            updateCommands();
        }

        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if(event->key() == Qt::Key_End && (event->modifiers() == Qt::ControlModifier || (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)))) {
        _detachedScrolling = false;
        if (_blockSelect) {
            disableBlockSelection();
        }

        if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) || _selectMode) {
            _cursor.moveToEndOfDocument(true);
        } else {
            _cursor.moveToEndOfDocument(false);
        }

        updateCommands();
        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_PageDown && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        _detachedScrolling = false;
        if (_blockSelect && event->modifiers() == Qt::ShiftModifier) {
            if (_doc->lineCount() > _blockSelectEndLine->line() + getVisibleLines()) {
                _blockSelectEndLine->setLine(_blockSelectEndLine->line() + getVisibleLines());
            } else {
                _blockSelectEndLine->setLine(_doc->lineCount() - 1);
            }
        } else {
            if (_blockSelect) {
                disableBlockSelection();
            }

            // Shift+PageUp/Down does not work with xterm's default settings.
            const bool extendSelection = event->modifiers() == Qt::ShiftModifier || _selectMode;
            const int amount = getVisibleLines();
            for (int i = 0; i < amount; i++) {
                _cursor.moveDown(extendSelection);
            }
        }
        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if (event->key() == Qt::Key_PageUp && (event->modifiers() == 0 || event->modifiers() == Qt::ShiftModifier)) {
        _detachedScrolling = false;
        if (_blockSelect && event->modifiers() == Qt::ShiftModifier) {
            if (_blockSelectEndLine->line() > getVisibleLines()) {
                _blockSelectEndLine->setLine(_blockSelectEndLine->line() - getVisibleLines());
            } else {
                _blockSelectEndLine->setLine(0);
            }
        } else {
            if (_blockSelect) {
                disableBlockSelection();
            }

            // Shift+PageUp/Down does not work with xterm's default settings.
            const bool extendSelection = event->modifiers() == Qt::ShiftModifier || _selectMode;
            const int amount = getVisibleLines();
            for (int i = 0; i < amount; i++) {
                _cursor.moveUp(extendSelection);
            }
        }
        adjustScrollPosition();
        _doc->clearCollapseUndoStep();
    } else if(event->key() == Qt::Key_Enter && (event->modifiers() & ~Qt::KeypadModifier) == 0) {
        _detachedScrolling = false;
        setSelectMode(false);
        if (_blockSelect) {
            delAndClearSelection();
        }
        // Inserting might adjust the scroll position, so save it here and restore it later.
        const int line = _scrollPositionY.line();
        _cursor.insertText("\n");
        _scrollPositionY.setLine(line);
        updateCommands();
        adjustScrollPosition();
    } else if(event->key() == Qt::Key_Tab && event->modifiers() == 0) {
        _detachedScrolling = false;
        if (_blockSelect) {
            if (hasBlockSelection()) {
                blockSelectRemoveSelectedAndConvertToMultiInsert();
            }

            multiInsertForEachCursor(mi_add_spaces, [&](Tui::ZDocumentCursor &cur) {
                addTabAt(cur);
            });
        } else if (_cursor.hasSelection()) {
            // Add one level of indent to the selected lines.

            const auto [firstLine, lastLine] = getSelectLinesSort();
            const auto [startLine, endLine] = getSelectLines();

            const bool savedSelectMode = getSelectMode();
            resetSelect();

            Tui::ZDocumentCursor cur = createCursor();

            for (int line = firstLine; line <= lastLine; line++) {
                if (_doc->lineCodeUnits(line) > 0) {
                    if (getTabOption()) {
                        cur.setPosition({0, line});
                        cur.insertText(QString("\t"));
                    } else {
                        cur.setPosition({0, line});
                        cur.insertText(QString(" ").repeated(getTabsize()));
                    }
                }
            }

            selectLines(startLine, endLine);
            setSelectMode(savedSelectMode);
            adjustScrollPosition();
        } else {
            if (_eatSpaceBeforeTabs && !_tabOption) {
                // If spaces in front of a tab
                const auto [cursorCodeUnit, cursorLine] = _cursor.position();
                int leadingSpace = 0;
                for (; leadingSpace < _doc->lineCodeUnits(cursorLine) && _doc->line(cursorLine)[leadingSpace] == ' '; leadingSpace++);
                if (leadingSpace > cursorCodeUnit) {
                    setCursorPosition({leadingSpace, cursorLine});
                }
            }
            // a normal tab
            addTabAt(_cursor);
            updateCommands();
            adjustScrollPosition();
            update();
        }
    } else if(event->key() == Qt::Key_Tab && event->modifiers() == Qt::ShiftModifier) {
        _detachedScrolling = false;
        // returns current line if no selection is active
        const auto [firstLine, lastLine] = getSelectLinesSort();
        const auto [startLine, endLine] = getSelectLines();
        const auto [cursorCodeUnit, cursorLine] = _cursor.position();

        const bool savedSelectMode = getSelectMode();
        const bool reselect = hasMultiInsert() || hasBlockSelection() || _cursor.hasSelection();
        resetSelect();

        int cursorAdjust = 0;

        for (int line = firstLine; line <= lastLine; line++) {
            int codeUnitsToRemove = 0;
            if (_doc->lineCodeUnits(line) && _doc->line(line)[0] == '\t') {
                codeUnitsToRemove = 1;
            } else {
                while (true) {
                    if (codeUnitsToRemove < _doc->lineCodeUnits(line)
                        && codeUnitsToRemove < getTabsize()
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
    } else if ((event->text() == "c" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Insert && event->modifiers() == Qt::ControlModifier) ) {
        _detachedScrolling = false;
        //STRG + C // Strg+Einfg
        copy();
    } else if ((event->text() == "v" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Insert && event->modifiers() == Qt::ShiftModifier)) {
        _detachedScrolling = false;
        //STRG + V // Umschalt+Einfg
        paste();
    } else if ((event->text() == "x" && event->modifiers() == Qt::ControlModifier) ||
               (event->key() == Qt::Key_Delete && event->modifiers() == Qt::ShiftModifier)) {
        _detachedScrolling = false;
        //STRG + X // Umschalt+Entf
        cut();
    } else if (event->text() == "z" && event->modifiers() == Qt::ControlModifier) {
        _detachedScrolling = false;
        undoGroup.closeGroup();
        undo();
    } else if (event->text() == "y" && event->modifiers() == Qt::ControlModifier) {
        _detachedScrolling = false;
        undoGroup.closeGroup();
        redo();
    } else if (event->text() == "a" && event->modifiers() == Qt::ControlModifier) {
        //STRG + a
        selectAll();
        _doc->clearCollapseUndoStep();
    } else if (event->text() == "k" && event->modifiers() == Qt::ControlModifier) {
        _detachedScrolling = false;
        //STRG + k //cut and copy line
        setSelectMode(false);
        cutline();
    } else if (event->text() == "d" && event->modifiers() == Qt::ControlModifier) {
        _detachedScrolling = false;
        //STRG + d //delete single line
        deleteLine();
    } else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Up) {
        // Fenster hoch Scrolen
        if (_scrollFineLine > 0) {
            _detachedScrolling = true;
            _scrollFineLine -= 1;
            update();
        } else if (_scrollPositionY.line() > 0) {
            _detachedScrolling = true;
            _scrollPositionY.setLine(_scrollPositionY.line() - 1);
            if (getWrapOption() != Tui::ZTextOption::WrapMode::NoWrap) {
                Tui::ZTextLayout lay = getTextLayoutForLine(getTextOption(false), _scrollPositionY.line());
                _scrollFineLine = lay.lineCount() - 1;
            } else {
                _scrollFineLine = 0;
            }
        }
    } else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Down) {
        // Fenster runter Scrolen
        bool adjustedFineScrolling = false;
        if (getWrapOption() != Tui::ZTextOption::WrapMode::NoWrap) {
            Tui::ZTextLayout lay = getTextLayoutForLine(getTextOption(false), _scrollPositionY.line());
            if (lay.lineCount() - 1 > _scrollFineLine) {
                _scrollFineLine += 1;
                adjustedFineScrolling = true;
                update();
            }
        }

        if (!adjustedFineScrolling && _doc->lineCount() - 1 > _scrollPositionY.line()) {
            _detachedScrolling = true;
            _scrollPositionY.setLine(_scrollPositionY.line() + 1);
            _scrollFineLine = 0;
        }
    } else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier) && event->key() == Qt::Key_Up) {
        _detachedScrolling = false;
        // returns current line if no selection is active
        const auto [firstLine, lastLine] = getSelectLinesSort();

        if (firstLine > 0) {
            const auto scrollPositionYSave = _scrollPositionY.line();
            const auto [startLine, endLine] = getSelectLines();
            const auto [cursorCodeUnit, cursorLine] = _cursor.position();

            const bool savedSelectMode = getSelectMode();
            const bool reselect = hasMultiInsert() || hasBlockSelection() || _cursor.hasSelection();
            resetSelect();

            // move lines up
            _doc->moveLine(firstLine - 1, endLine, &_cursor);

            // Update cursor / recreate selection
            if (!reselect) {
                setCursorPosition({cursorCodeUnit, cursorLine - 1});
            } else {
                selectLines(startLine - 1, endLine - 1);
            }
            setSelectMode(savedSelectMode);
            _scrollPositionY.setLine(scrollPositionYSave);
            adjustScrollPosition();
        }
    } else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier) && event->key() == Qt::Key_Down) {
        _detachedScrolling = false;
        // returns current line if no selection is active
        const auto [firstLine, lastLine] = getSelectLinesSort();

        if (lastLine < _doc->lineCount() - 1) {
            const auto scrollPositionYSave = _scrollPositionY.line();
            const auto [startLine, endLine] = getSelectLines();
            const auto [cursorCodeUnit, cursorLine] = _cursor.position();

            const bool savedSelectMode = getSelectMode();
            const bool reselect = hasMultiInsert() || hasBlockSelection() || _cursor.hasSelection();
            resetSelect();

            // Move lines down
            _doc->moveLine(lastLine + 1, firstLine, &_cursor);

            // Update cursor / recreate selection
            if (!reselect) {
                setCursorPosition({cursorCodeUnit, cursorLine + 1});
            } else {
                selectLines(startLine + 1, endLine + 1);
            }
            setSelectMode(savedSelectMode);
            _scrollPositionY.setLine(scrollPositionYSave);
            adjustScrollPosition();
        }
    } else if (event->key() == Qt::Key_Escape && event->modifiers() == 0) {
        _detachedScrolling = false;
        setSearchText("");
        if (_blockSelect) {
            disableBlockSelection();
        }
        adjustScrollPosition();
    } else if (event->key() == Qt::Key_Insert && event->modifiers() == 0) {
        _detachedScrolling = false;
        toggleOverwrite();
    } else if (event->key() == Qt::Key_F4 && event->modifiers() == 0) {
        _detachedScrolling = false;
        toggleSelectMode();
    } else {
        undoGroup.closeGroup();
        Tui::ZWidget::keyEvent(event);
    }
}

std::tuple<int, int, int> File::cursorPositionOrBlockSelectionEnd() {
    if (_blockSelect) {
        const int cursorLine = _blockSelectEndLine->line();
        const int cursorColumn = _blockSelectEndColumn;
        Tui::ZTextLayout layNoWrap = getTextLayoutForLineWithoutWrapping(cursorLine);
        const int cursorCodeUnit = layNoWrap.lineAt(0).xToCursor(cursorColumn);
        return std::make_tuple(cursorCodeUnit, cursorLine, cursorColumn);
    } else {
        const auto [cursorCodeUnit, cursorLine] = _cursor.position();
        Tui::ZTextLayout layNoWrap = getTextLayoutForLineWithoutWrapping(cursorLine);
        int cursorColumn = layNoWrap.lineAt(0).cursorToX(cursorCodeUnit, Tui::ZTextLayout::Leading);

        return std::make_tuple(cursorCodeUnit, cursorLine, cursorColumn);
    }
}

void File::adjustScrollPosition() {

    if(geometry().width() <= 0 && geometry().height() <= 0) {
        return;
    }

    if (_detachedScrolling) {
        if (_scrollPositionY.line() >= _doc->lineCount()) {
            _scrollPositionY.setLine(_doc->lineCount() - 1);
        }

        update();
        return;
    }

    const auto [cursorCodeUnit, cursorLine, cursorColumn] = cursorPositionOrBlockSelectionEnd();

    int viewWidth = geometry().width() - shiftLinenumber();
    // horizontal scroll position
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

    // vertical scroll position
    if (!_wrapOption) {
        if (cursorLine >= 0) {
            if (cursorLine - _scrollPositionY.line() < 1) {
                _scrollPositionY.setLine(cursorLine);
                _scrollFineLine = 0;
            }
        }

        if (cursorLine - _scrollPositionY.line() >= geometry().height() - 1) {
            _scrollPositionY.setLine(cursorLine - geometry().height() + 2);
        }

        if (_doc->lineCount() - _scrollPositionY.line() < geometry().height() - 1) {
            _scrollPositionY.setLine(std::max(0, _doc->lineCount() - geometry().height() + 1));
        }
    } else {
        //TODO: #193 scrollup with Crl+Up and wraped lines.
        Tui::ZTextOption option = getTextOption(false);

        const int availableLinesAbove = geometry().height() - 2;

        Tui::ZTextLayout layCursorLayout = getTextLayoutForLine(option, cursorLine);
        int linesAbove = layCursorLayout.lineForTextPosition(cursorCodeUnit).lineNumber();

        if (linesAbove >= availableLinesAbove) {
            if (_scrollPositionY.line() < cursorLine) {
                _scrollPositionY.setLine(cursorLine);
                _scrollFineLine = linesAbove - availableLinesAbove;
            } if (_scrollPositionY.line() == cursorLine) {
                if (_scrollFineLine < linesAbove - availableLinesAbove) {
                    _scrollFineLine = linesAbove - availableLinesAbove;
                }
            }
        } else {
            for (int line = cursorLine - 1; line >= 0; line--) {
                Tui::ZTextLayout lay = getTextLayoutForLine(option, line);
                if (linesAbove + lay.lineCount() >= availableLinesAbove) {
                    if (_scrollPositionY.line() < line) {
                        _scrollPositionY.setLine(line);
                        _scrollFineLine = (linesAbove + lay.lineCount()) - availableLinesAbove;
                    } if (_scrollPositionY.line() == line) {
                        if (_scrollFineLine < (linesAbove + lay.lineCount()) - availableLinesAbove) {
                            _scrollFineLine = (linesAbove + lay.lineCount()) - availableLinesAbove;
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

        if (_scrollPositionY.line() == cursorLine) {
            if (linesAbove < _scrollFineLine) {
                _scrollFineLine = linesAbove;
            }
        } else if (_scrollPositionY.line() > cursorLine) {
            _scrollPositionY.setLine(cursorLine);
            _scrollFineLine = linesAbove;
        }

        // scroll when window is larger than the document shown (unless scrolled to top)
        if (_scrollPositionY.line() && _scrollPositionY.line() + (geometry().height() - 1) > _doc->lineCount()) {
            int linesCounted = 0;
            QVector<int> sizes;

            for (int line = _doc->lineCount() - 1; line >= 0; line--) {
                Tui::ZTextLayout lay = getTextLayoutForLine(option, line);
                sizes.append(lay.lineCount());
                linesCounted += lay.lineCount();
                if (linesCounted >= geometry().height() - 1) {
                    if (_scrollPositionY.line() > line) {
                        _scrollPositionY.setLine(line);
                        _scrollFineLine = linesCounted - (geometry().height() - 1);
                    } else if (_scrollPositionY.line() == line &&
                               _scrollFineLine > linesCounted - (geometry().height() - 1)) {
                        _scrollFineLine = linesCounted - (geometry().height() - 1);
                    }
                    break;
                }
            }
        }
    }

    scrollPositionChanged(_scrollPositionX, _scrollPositionY.line());

    int max=0;
    for (int i = _scrollPositionY.line(); i < _doc->lineCount() && i < _scrollPositionY.line() + geometry().height(); i++) {
        if(max < _doc->lineCodeUnits(i)) {
            max = _doc->lineCodeUnits(i);
        }
    }
    textMax(max - viewWidth, _doc->lineCount() - geometry().height());

    update();
}

void File::updateCommands() {
    _cmdCopy->setEnabled(hasBlockSelection() || _cursor.hasSelection());
    _cmdCut->setEnabled(hasBlockSelection() || _cursor.hasSelection());
    _cmdPaste->setEnabled(isInsertable());
}


