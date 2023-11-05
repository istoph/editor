// SPDX-License-Identifier: BSL-1.0

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <Tui/ZCommandNotifier.h>
#include <Tui/ZCommon.h>
#include <Tui/ZDocument.h>
#include <Tui/ZDocumentLineMarker.h>
#include <Tui/ZTextMetrics.h>
#include <Tui/ZTextOption.h>
#include <Tui/ZWidget.h>


class ZTextEdit : public Tui::ZWidget {
    Q_OBJECT

public:
    using Position = Tui::ZDocumentCursor::Position;
    using FindFlag = Tui::ZDocument::FindFlag;
    using FindFlags = Tui::ZDocument::FindFlags;

public:
    explicit ZTextEdit(Tui::ZTextMetrics textMetrics, Tui::ZWidget *parent);
    explicit ZTextEdit(Tui::ZTextMetrics textMetrics, Tui::ZDocument *document, Tui::ZWidget *parent);

public:
    void setCursorPosition(Position position, bool extendSelection=false);
    Position cursorPosition() const;
    void setAnchorPosition(Position position);
    Position anchorPosition() const;
    void setTextCursor(const Tui::ZDocumentCursor &cursor);
    Tui::ZDocumentCursor textCursor() const;
    void setSelection(Position anchor, Position position);

    Tui::ZDocument *document() const;

    int lineNumberBorderWidth() const;
    virtual int allBordersWidth() const;

    void setTabStopDistance(int tab);
    int tabStopDistance() const;
    void setShowLineNumbers(bool show);
    bool showLineNumbers() const;
    void setUseTabChar(bool tab);
    bool useTabChar() const;
    void setWordWrapMode(Tui::ZTextOption::WrapMode wrap);
    Tui::ZTextOption::WrapMode wordWrapMode() const;
    void setOverwriteMode(bool mode);
    void toggleOverwriteMode();
    bool overwriteMode() const;
    void setSelectMode(bool mode);
    void toggleSelectMode();
    bool selectMode() const;
    void setInsertCursorStyle(Tui::CursorStyle style);
    Tui::CursorStyle insertCursorStyle() const;
    void setOverwriteCursorStyle(Tui::CursorStyle style);
    Tui::CursorStyle overwriteCursorStyle() const;
    void setTabChangesFocus(bool enabled);
    bool tabChangesFocus() const;
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    void setUndoRedoEnabled(bool enabled);
    bool isUndoRedoEnabled() const;

    bool isModified() const;

    void insertText(const QString &str);
    void insertTabAt(Tui::ZDocumentCursor &cur);

    virtual void cut();
    virtual void copy();
    virtual void paste();

    virtual bool canPaste();
    virtual bool canCut();
    virtual bool canCopy();

    Tui::ZDocument::UndoGroup startUndoGroup();

    void removeSelectedText();
    void clearSelection();
    void selectAll();
    QString selectedText()  const;
    bool hasSelection() const;

    void undo();
    void redo();

    void enableDetachedScrolling();
    void disableDetachedScrolling();
    bool isDetachedScrolling() const;
    void detachedScrollUp();
    void detachedScrollDown();

    int scrollPositionLine() const;
    int scrollPositionColumn() const;
    int scrollPositionFineLine() const;
    void setScrollPosition(int column, int line, int fineLine);

    virtual int pageNavigationLineCount() const;

    Tui::ZDocumentCursor findSync(const QString &subString, FindFlags options = FindFlags{});
    Tui::ZDocumentCursor findSync(const QRegularExpression &regex, FindFlags options = FindFlags{});
    Tui::ZDocumentFindResult findSyncWithDetails(const QRegularExpression &regex, FindFlags options = FindFlags{});
    QFuture<Tui::ZDocumentFindAsyncResult> findAsync(const QString &subString, FindFlags options = FindFlags{});
    QFuture<Tui::ZDocumentFindAsyncResult> findAsync(const QRegularExpression &regex, FindFlags options = FindFlags{});
    QFuture<Tui::ZDocumentFindAsyncResult> findAsyncWithPool(QThreadPool *pool, int priority,
                                                             const QString &subString, FindFlags options = FindFlags{});
    QFuture<Tui::ZDocumentFindAsyncResult> findAsyncWithPool(QThreadPool *pool, int priority,
                                                             const QRegularExpression &regex,
                                                             FindFlags options = FindFlags{});

    void clear();

    void readFrom(QIODevice *file);
    void readFrom(QIODevice *file, Position initialPosition);
    void writeTo(QIODevice *file) const;

    void registerCommandNotifiers(Qt::ShortcutContext context);

    Tui::ZDocumentCursor makeCursor();

signals:
    void cursorPositionChanged(int x, int utf8CodeUnit, int line);
    void scrollPositionChanged(int x, int line, int fineLine);
    void scrollRangeChanged(int x, int y);
    void overwriteModeChanged(bool overwrite);
    void modifiedChanged(bool modified);
    void selectModeChanged(bool mode);

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;
    void pasteEvent(Tui::ZPasteEvent *event) override;
    void resizeEvent(Tui::ZResizeEvent *event) override;

protected:
    virtual Tui::ZTextOption textOption() const;
    Tui::ZTextLayout textLayoutForLine(const Tui::ZTextOption &option, int line) const;
    Tui::ZTextLayout textLayoutForLineWithoutWrapping(int line) const;
    const Tui::ZTextMetrics &textMetrics() const;

    virtual void adjustScrollPosition();
    virtual void emitCursorPostionChanged();

    virtual void updateCommands();

    virtual void clearAdvancedSelection();

private:
    void updatePasteCommandEnabled();

    QPair<int, int> getSelectedLinesSort();
    QPair<int, int> getSelectedLines();
    void selectLines(int startLine, int endLine);

    void connectAsyncFindWatcher(QFuture<Tui::ZDocumentFindAsyncResult> res, FindFlags options);

private:
    Tui::ZTextMetrics _textMetrics;
    Tui::ZDocument *_doc = nullptr;
    Tui::ZDocumentCursor _cursor;
    bool _selectMode = false;

    int _tabsize = 8;
    bool _useTabChar = false;
    Tui::ZTextOption::WrapMode _wrapMode = Tui::ZTextOption::NoWrap;
    bool _showLineNumbers = false;
    bool _overwriteMode = false;
    bool _tabChangesFocus = true;
    bool _readOnly = false;
    bool _undoRedoEnabled = true;
    Tui::CursorStyle _insertCursorStyle = Tui::CursorStyle::Bar;
    Tui::CursorStyle _overwriteCursorStyle = Tui::CursorStyle::Block;

    bool _detachedScrolling = false;
    int _scrollPositionColumn = 0;
    Tui::ZDocumentLineMarker _scrollPositionLine;
    int _scrollPositionFineLine = 0;

    Tui::ZCommandNotifier *_cmdCopy = nullptr;
    Tui::ZCommandNotifier *_cmdCut = nullptr;
    Tui::ZCommandNotifier *_cmdPaste = nullptr;
    Tui::ZCommandNotifier *_cmdUndo = nullptr;
    Tui::ZCommandNotifier *_cmdRedo = nullptr;
};

#endif // TEXTEDIT_H
