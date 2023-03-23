// SPDX-License-Identifier: BSL-1.0

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <functional>

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
        int x = 0; // TODO: -> codeUnit
        int y = 0; // TODO: -> line

        Position(int x, int y) : x(x), y(y) {}
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
    int _startSelectX = -1;
    int _startSelectY = -1;
    int _endSelectX = -1;
    int _endSelectY = -1;
    int _saveCursorPositionX = 0;

    Document *_doc;
    Tui::ZWidget *_widget;
    std::function<Tui::ZTextLayout(int line, bool wrappingAllowed)> _createTextLayout;
};

class Document : public QObject {
    Q_OBJECT
    friend class TextCursor;

public:
    Document(QObject *parent=nullptr);

public:
    int lineCount() const;
    QString line(int line) const;
    int lineCodeUnits(int line) const;

    bool isModified() const;

    void undo(TextCursor *cursor);
    void redo(TextCursor *cursor);
    void setGroupUndo(TextCursor *cursor, bool onoff);
    int getGroupUndo();

    void initalUndoStep(TextCursor *cursor);
    void saveUndoStep(TextCursor *cursor, bool collapsable=false);
    struct UndoStep {
        QVector<QString> text;
        int cursorPositionX;
        int cursorPositionY;
    };

signals:
    void modificationChanged(bool changed);
    void redoAvailable(bool available);
    void undoAvailable(bool available);

private: // TextCursor interface
    void removeFromLine(int line, int codeUnitStart, int codeUnits);
    void insertIntoLine(int line, int codeUnitStart, const QString &data);
    void appendToLine(int line, const QString &data);
    void removeLines(int start, int count);
    void insertLine(int before, const QString &data);
    void splitLine(TextCursor::Position pos);

private:
    void emitModifedSignals();

public:
    QString _filename;
    QVector<QString> _text;
    bool _nonewline = false;

    QVector<UndoStep> _undoSteps;
    int _currentUndoStep = -1;
    int _savedUndoStep = -1;

    bool _collapseUndoStep = false;
    int _groupUndo = 0;
};

#endif // DOCUMENT_H
