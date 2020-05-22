#ifndef FILE_H
#define FILE_H

#include <testtui_lib.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

class File : public Tui::ZWidget {
    Q_OBJECT

public:
    File();
    explicit File(Tui::ZWidget *parent);
    bool setFilename(QString filename);
    QString getFilename();
    bool newText();
    bool saveText();
    bool openText();
    void cut();
    void cutline();
    void copy();
    void paste();
    bool isInsertable();
    void insertLinebreak();
    void gotoline(int y=0, int x=0);
    bool setTabsize(int tab);
    int getTabsize();
    void setTabOption(bool tab);
    bool getTabOption();
    bool setFormattingCharacters(bool fb);
    bool getformattingCharacters();
    void setWrapOption(bool wrap);
    bool getWrapOption();
    void select(int x, int y);
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
    void deletePreviousCharacterOrWord(TextLayout::CursorMode mode);
    void deleteNextCharacterOrWord(TextLayout::CursorMode mode);
    int getVisibleLines();
    void appendLine(const QString &line);

    bool isModified() const;
    void setSearchText(QString searchText);
    Qt::CaseSensitivity searchCaseSensitivity;
    void setHighlightBracket(bool hb);
    bool getHighlightBracket();

public:
//    QString text() const;
//    void setText(const QString &t);
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;
    bool newfile = true;

    void searchNext(int line = -1);
    void searchPrevious(int line = -1);
    void setSearchWrap(bool wrap);
    void checkWritable();

public slots:
    void followStandardInput(bool follow);

signals:
    void cursorPositionChanged(int x, int utf8x, int y);
    void scrollPositionChanged(int x, int y);
    void textMax(int x, int y);
    void modifiedChanged(bool modified);
    void setWritable(bool rw);

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;

private:
    void adjustScrollPosition();
    void safeCursorPosition();
    void saveUndoStep(bool collapsable=false);
    QString filename;
    struct UndoStep {
        QVector<QString> text;
        int cursorPositionX;
        int cursorPositionY;
    };

    ZTextOption getTextOption();
    TextLayout getTextLayoutForLine(const ZTextOption &option, int line);
    bool highlightBracket();

private:
    QVector<QString> _text;
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
    int _lastCursorPositionX = -1;
    int _tabsize = 8;
    bool _tabOption = true;
    bool _wrapOption = false;
    bool _formatting_characters = true;
    int startSelectX = -1;
    int startSelectY = -1;
    int endSelectX = -1;
    int endSelectY = -1;
    QVector<QString> _clipboard;
    bool overwrite = false;
    QVector<UndoStep> _undoSteps;
    int _currentUndoStep;
    bool _collapseUndoStep;
    int _savedUndoStep;
    QString _searchText;
    bool _searchWrap;
    bool _follow = false;
    bool _nonewline = false;
    int _saveCursorPositionX = 0;
    int _bracketX = -1;
    int _bracketY = -1;
    bool _bracket = false;
};

#endif // FILE_H
