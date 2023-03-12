// SPDX-License-Identifier: BSL-1.0

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <functional>

#include <QString>
#include <QVector>

#include <Tui/ZTerminal.h>
#include <Tui/ZTextLayout.h>

class File;
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
    TextCursor(Document *doc, File *file, std::function<Tui::ZTextLayout(int line, bool wrappingAllowed)> createTextLayout);

    void insertText(const QString &text);
    void removeSelectedText();
    void clearSelection();

    void deleteCharacter();
    void deletePreviousCharacter();
    void deleteWord();
    void deletePreviousWord();

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

    Position anchor();
    void setAnchorPosition(Position pos);

    // for hasSelection() == true
    Position selectionStartPos() const;
    Position selectionEndPos() const;

    // for hasBlockSelection() == true
    Position selectionBlockStartPos() const;
    Position selectionBlockEndPos() const;

    bool hasSelection() const;
    bool hasBlockSelection() const;
    bool hasAnySelection() const;

    bool hasMultiInsert() const;

    bool atStart() const;
    bool atEnd() const;
    bool atLineStart() const;
    bool atLineEnd() const;

    // rethink these later:
    Range getBlockSelectedLines();
    Position insertLineBreakAt(Position pos);

private:
    //TextLayout createTextLayout(int line, bool wrappingAllowed);
    void updateVerticalMovementColumn(const Tui::ZTextLayout &layoutForCursorLine);

private:
    Document *_doc;
    File *_file;
    std::function<Tui::ZTextLayout(int line, bool wrappingAllowed)> _createTextLayout;
};

class Document {
    friend class TextCursor;
public:
    bool isModified() const;

    void undo(File *file);
    void redo(File *file);
    void setGroupUndo(File *file, bool onoff);
    int getGroupUndo();

    void initalUndoStep(File *file);
    void saveUndoStep(File *file, bool collapsable=false);
    struct UndoStep {
        QVector<QString> text;
        int cursorPositionX;
        int cursorPositionY;
    };

private: // TextCursor interface
    void removeFromLine(int line, int codeUnitStart, int codeUnits);
    void insertIntoLine(int line, int codeUnitStart, const QString &data);
    void appendToLine(int line, const QString &data);
    void removeLines(int start, int count);
    void insertLine(int before, const QString &data);
    void splitLine(TextCursor::Position pos);

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
