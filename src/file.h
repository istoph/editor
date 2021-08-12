#ifndef FILE_H
#define FILE_H

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTextStream>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QPair>

#include <testtui_lib.h>
#include <Tui/ZCommandNotifier.h>
#include "searchcount.h"
#include "clipboard.h"

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

struct SearchLine;
struct SearchParameter;

class File : public Tui::ZWidget {
    Q_OBJECT

public:
    struct Position {
        int x = 0;
        int y = 0;

        Position(int x, int y) : x(x), y(y) {}
        friend bool operator<(const Position& a, const Position& b);
        friend bool operator>(const Position& a, const Position& b);
        friend bool operator==(const Position& a, const Position& b);
        friend void swap(Position& a, Position& b);
    };

public:
    File();
    explicit File(Tui::ZWidget *parent);
    bool setFilename(QString _filename);
    QString getFilename();
    QString emptyFilename();
    bool saveText();
    bool openText(QString filename);
    void cut();
    void cutline();
    void copy();
    void paste();
    bool isInsertable();
    void insertLinebreak();
    QPoint insertLinebreakAtPosition(QPoint cursor);
    void gotoline(QString pos);
    bool setTabsize(int tab);
    int getTabsize();
    void setTabOption(bool tab);
    bool getTabOption();
    bool setFormattingCharacters(bool fb);
    bool getformattingCharacters();
    void setWrapOption(bool wrap);
    bool getWrapOption();
    void select(int x, int y);
    void blockSelect(int x, int y);
    bool blockSelectEdit(int x);
    QPair<int, int> getSelectLines();
    void selectLines(int startY, int endY);
    void resetSelect();
    QString getSelectText();
    bool isSelect(int x, int y);
    bool isSelect();
    void selectAll();
    bool delSelect();
    void toggleOverwrite();
    bool isOverwrite();
    void undo();
    void redo();
    void setGroupUndo(bool onoff);
    int getGroupUndo();
    void deletePreviousCharacterOrWord(TextLayout::CursorMode mode);
    QPoint deletePreviousCharacterOrWordAt(TextLayout::CursorMode mode, int x, int y);
    void deleteNextCharacterOrWord(TextLayout::CursorMode mode);
    QPoint deleteNextCharacterOrWordAt(TextLayout::CursorMode mode, int x, int y);
    QPoint addTabAt(QPoint cursor);
    int getVisibleLines();
    void appendLine(const QString &line);
    void insertAtCursorPosition(QVector<QString> str);
    QPoint insertAtPosition(QVector<QString> str, QPoint cursor);
    bool isModified() const;
    void setSearchText(QString searchText);
    void setReplaceText(QString replaceText);
    void setReplaceSelected();
    Qt::CaseSensitivity searchCaseSensitivity;
    void setHighlightBracket(bool hb);
    bool getHighlightBracket();
    bool readAttributes();
    void getAttributes();
    bool writeAttributes();
    void setAttributesfile(QString attributesfile);
    bool getMsDosMode();
    void setMsDosMode(bool msdos);
    int tabToSpace();
    int replaceAll(QString searchText, QString replaceText);
    QPoint getCursorPosition();
    void setCursorPosition(QPoint position);
    void toggleSelectMode();
    bool select_cursor_position_x0;

public:
    void setSearchWrap(bool wrap);
    bool getSearchWrap();
    void setRegex(bool reg);
    void checkWritable();
    bool getWritable();
    void runSearch(bool direction);
    void setSearchDirection(bool searchDirection);
    void searchSelect(int found = -1);
    void setLineNumber(bool linenumber);
    void toggleLineNumber();
    void setSaveAs(bool state);
    bool isSaveAs();
    bool newText(QString filename);
    bool stdinText();
public slots:
    void followStandardInput(bool follow);

signals:
    void cursorPositionChanged(int x, int utf8x, int y);
    void scrollPositionChanged(int x, int y);
    void textMax(int x, int y);
    void modifiedChanged(bool modified);
    void setWritable(bool rw);
    void msdosMode(bool msdos);
    void modifiedSelectMode(bool f4);
    void emitSearchCount(int sc);
    void emitSearchText(QString searchText);
    void emitOverwrite(bool overwrite);

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;
    void pasteEvent(Tui::ZPasteEvent *event) override;
    void resizeEvent(Tui::ZResizeEvent *event) override;

private:
    bool initText();
    void adjustScrollPosition();
    void safeCursorPosition();
    void saveUndoStep(bool collapsable=false);
    void checkUndo();
    struct UndoStep {
        QVector<QString> text;
        int cursorPositionX;
        int cursorPositionY;
    };

    ZTextOption getTextOption();
    TextLayout getTextLayoutForLine(const ZTextOption &option, int line);
    bool highlightBracket();
    void searchSelect(int line, int found, int length, bool direction);
    SearchLine searchNext(QVector<QString> text, SearchParameter search);
    //void searchSelectPrevious(int line, int found);
    Range getBlockSelectedLines();
    void selectCursorPosition(Qt::KeyboardModifiers modifiers);
    void setSelectMode(bool f4);
    bool getSelectMode();

private:
    QString _filename;
    QVector<QString> _text;
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
    int _lastCursorPositionX = -1;
    int _tabsize = 8;
    bool _tabOption = true;
    bool _wrapOption = false;
    bool _formatting_characters = true;
    int _startSelectX = -1;
    int _startSelectY = -1;
    int _endSelectX = -1;
    int _endSelectY = -1;
    bool _overwrite = false;
    QVector<UndoStep> _undoSteps;
    int _currentUndoStep = -1;
    bool _collapseUndoStep = false;
    int _savedUndoStep = -1;
    QString _searchText;
    QString _replaceText;
    bool _searchWrap = true;
    bool _searchReg = false;
    bool _searchDirection = true;
    std::shared_ptr<std::atomic<int>> searchGeneration = std::make_shared<std::atomic<int>>();
    std::shared_ptr<std::atomic<int>> searchNextGeneration = std::make_shared<std::atomic<int>>();
    bool _follow = false;
    bool _nonewline = false;
    int _saveCursorPositionX = 0;
    int _bracketX = -1;
    int _bracketY = -1;
    bool _bracket = false;
    int _groupUndo = 0;
    QJsonObject _jo;
    bool _noattr = false;
    QString _attributesfile;
    bool _msdos = false;
    bool _linenumber = false;
    bool _saveAs = true;

    Tui::ZCommandNotifier *_cmdCopy = nullptr;
    Tui::ZCommandNotifier *_cmdCut = nullptr;
    Tui::ZCommandNotifier *_cmdPaste = nullptr;
    Tui::ZCommandNotifier *_cmdUndo = nullptr;
    Tui::ZCommandNotifier *_cmdRedo = nullptr;
    Tui::ZCommandNotifier *_cmdSearchNext = nullptr;
    Tui::ZCommandNotifier *_cmdSearchPrevious = nullptr;
    bool _selectMode = false;
    bool _blockSelect = false;


};

#endif // FILE_H
