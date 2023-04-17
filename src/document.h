// SPDX-License-Identifier: BSL-1.0

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <functional>

#include <QIODevice>
#include <QObject>
#include <QString>
#include <QVector>

#include <Tui/ZTerminal.h>
#include <Tui/ZTextLayout.h>

class Document;

class RangeIterator {
public:
    int i;
    int operator *(){ return i; };
    bool operator !=(const RangeIterator & b) {return i!=b.i; }
    int operator ++() { return i++; }
};

class Range {
public:
    int start;
    int stop;
    RangeIterator begin() { return RangeIterator {start};}
    RangeIterator end() { return RangeIterator {stop};}
};

class TextCursor {
public:
    struct Position {
        int codeUnit = 0; // in line
        int line = 0;

        Position(int codeUnit, int line) : codeUnit(codeUnit), line(line) {}
        friend bool operator<(const Position& a, const Position& b);
        friend bool operator>(const Position& a, const Position& b);
        friend bool operator==(const Position& a, const Position& b);
    };


public:
    TextCursor(Document *doc, Tui::ZWidget *widget, std::function<Tui::ZTextLayout(int line, bool wrappingAllowed)> createTextLayout);

    void insertText(const QString &text);
    void removeSelectedText();
    void clearSelection();
    QString selectedText() const;

    void deleteCharacter();
    void deletePreviousCharacter();
    void deleteWord();
    void deletePreviousWord();
    void deleteLine();

    void moveCharacterLeft(bool extendSelection = false);
    void moveCharacterRight(bool extendSelection = false);
    void moveWordLeft(bool extendSelection = false);
    void moveWordRight(bool extendSelection = false);

    void moveUp(bool extendSelection = false);
    void moveDown(bool extendSelection = false);

    void moveToStartOfLine(bool extendSelection = false);
    void moveToStartIndentedText(bool extendSelection = false);
    void moveToEndOfLine(bool extendSelection = false);

    void moveToStartOfDocument(bool extendSelection = false);
    void moveToEndOfDocument(bool extendSelection = false);

    Position position();
    void setPosition(Position pos, bool extendSelection = false);
    void setPositionPreservingVerticalMovementColumn(Position pos, bool extendSelection = false);

    Position anchor();
    void setAnchorPosition(Position pos);

    int verticalMovementColumn();
    void setVerticalMovementColumn(int column);

    // for hasSelection() == true
    Position selectionStartPos() const;
    Position selectionEndPos() const;

    void selectAll();

    bool hasSelection() const;

    bool atStart() const;
    bool atEnd() const;
    bool atLineStart() const;
    bool atLineEnd() const;

public: // temporary hacks
    void tmp_ensureInRange();

private:
    //TextLayout createTextLayout(int line, bool wrappingAllowed);
    void updateVerticalMovementColumn(const Tui::ZTextLayout &layoutForCursorLine);

private:
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;
    int _anchorPositionX = 0;
    int _anchorPositionY = 0;
    int _saveCursorPositionX = 0;

    Document *_doc;
    Tui::ZWidget *_widget;
    std::function<Tui::ZTextLayout(int line, bool wrappingAllowed)> _createTextLayout;
};

class Document : public QObject {
    Q_OBJECT
    friend class TextCursor;

public:
    class UndoGroup;

public:
    Document(QObject *parent=nullptr);

public:
    void reset();
    void writeTo(QIODevice *file, bool crLfMode = false);
    bool readFrom(QIODevice *file);

    int lineCount() const;
    QString line(int line) const;
    int lineCodeUnits(int line) const;
    QVector<QString> getLines() const;

    bool isModified() const;

    void setNoNewline(bool value);
    bool noNewLine() const;

    QString filename() const;
    void setFilename(const QString &filename);

    void clearCollapseUndoStep();

    void tmp_sortLines(int first, int last, TextCursor *cursorForUndoStep);
    void tmp_moveLine(int from, int to, TextCursor *cursorForUndoStep);

    void undo(TextCursor *cursor);
    void redo(TextCursor *cursor);
    UndoGroup startUndoGroup(TextCursor *cursor);

    void initalUndoStep(TextCursor *cursor);

signals:
    void modificationChanged(bool changed);
    void redoAvailable(bool available);
    void undoAvailable(bool available);

public:
    class UndoGroup {
    public:
        UndoGroup(const UndoGroup&) = delete;
        UndoGroup &operator=(const UndoGroup&) = delete;

        ~UndoGroup();

    public:
        void closeGroup();

    private:
        friend class Document;
        UndoGroup(Document *doc, TextCursor *cursor);

        Document *_doc = nullptr;
        TextCursor *_cursor = nullptr;
        bool _closed = false;
    };

private: // TextCursor interface
    void removeFromLine(int line, int codeUnitStart, int codeUnits);
    void insertIntoLine(int line, int codeUnitStart, const QString &data);
    void appendToLine(int line, const QString &data);
    void removeLines(int start, int count);
    void insertLine(int before, const QString &data);
    void splitLine(TextCursor::Position pos);
    void saveUndoStep(TextCursor *cursor, bool collapsable=false);

private: // UndoGroup interface
    void closeUndoGroup(TextCursor *cursor);

private:
    void emitModifedSignals();

private:
    struct UndoStep {
        QVector<QString> text;
        int cursorPositionX;
        int cursorPositionY;
    };

private:
    QString _filename;
    QVector<QString> _text;
    bool _nonewline = false;

    QVector<UndoStep> _undoSteps;
    int _currentUndoStep = -1;
    int _savedUndoStep = -1;

    bool _collapseUndoStep = false;
    int _groupUndo = 0;
    bool _undoStepCreationDeferred = false;
};

#endif // DOCUMENT_H
