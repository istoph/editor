#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include <QVector>

class File;

class Document {
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
