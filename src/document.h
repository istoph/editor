#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include <QVector>

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
    TextCursor(Document *doc, File *file);

    void insertText(const QString &text);
    void removeSelectedText();
    void clearSelection();

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
    Document *_doc;
    File *_file;
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
