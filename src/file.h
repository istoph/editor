#ifndef FILE_H
#define FILE_H

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPair>
#include <QSaveFile>
#include <QTextStream>
#include <QtConcurrent>

#include <Tui/ZCommandNotifier.h>
#include <Tui/ZTextLayout.h>
#include <Tui/ZTextOption.h>

#include <testtui_lib.h>

#include "document.h"
#include "searchcount.h"
#include "clipboard.h"
#include "limits"

struct SearchLine;
struct SearchParameter;

class File : public Tui::ZWidget {
    Q_OBJECT

    friend class TextCursor;
    friend class DocumentTestHelper;
public:
    using Position = TextCursor::Position;

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
    void setEatSpaceBeforeTabs(bool eat);
    bool eatSpaceBeforeTabs();
    bool formattingCharacters();
    void setFormattingCharacters(bool formattingCharacters);
    bool colorTabs();
    void setColorTabs(bool colorTabs);
    bool colorSpaceEnd();
    void setColorSpaceEnd(bool colorSpaceEnd);
    void setWrapOption(Tui::ZTextOption::WrapMode wrap);
    Tui::ZTextOption::WrapMode getWrapOption();
    void select(int x, int y);
    void blockSelect(int x, int y);
    bool blockSelectEdit(int x);
    QPair<int, int> getSelectLinesSort();
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
    void deletePreviousCharacterOrWord(Tui::ZTextLayout::CursorMode mode);
    QPoint deletePreviousCharacterOrWordAt(Tui::ZTextLayout::CursorMode mode, int x, int y);
    void deleteNextCharacterOrWord(Tui::ZTextLayout::CursorMode mode);
    QPoint deleteNextCharacterOrWordAt(Tui::ZTextLayout::CursorMode mode, int x, int y);
    QPoint addTabAt(QPoint cursor);
    int getVisibleLines();
    void appendLine(const QString &line);
    void insertAtCursorPosition(QVector<QString> str);
    QPoint insertAtPosition(QVector<QString> str, QPoint cursor);
    bool isModified() const { return _doc.isModified(); };
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
    QString getAttributesfile();
    bool getMsDosMode();
    void setMsDosMode(bool msdos);
    int tabToSpace();
    int replaceAll(QString searchText, QString replaceText);
    QPoint getCursorPosition();
    void setCursorPosition(QPoint position);
    QPoint getScrollPosition();
    void toggleSelectMode();
    bool select_cursor_position_x0;
    bool isNewFile();

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
    bool getLineNumber();
    void toggleLineNumber();
    void setSaveAs(bool state);
    bool isSaveAs();
    bool newText(QString filename);
    bool stdinText();
    void sortSelecedLines();

    bool event(QEvent *event) override;

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
    void checkUndo();

    Tui::ZTextOption getTextOption(bool lineWithCursor);
    Tui::ZTextLayout getTextLayoutForLine(const Tui::ZTextOption &option, int line);
    bool highlightBracket();
    void searchSelect(int line, int found, int length, bool direction);
    SearchLine searchNext(QVector<QString> text, SearchParameter search);
    //void searchSelectPrevious(int line, int found);
    Range getBlockSelectedLines();
    void selectCursorPosition(Qt::KeyboardModifiers modifiers);
    void setSelectMode(bool f4);
    bool getSelectMode();
    int shiftLinenumber();

private:
    Document _doc;
    TextCursor _cursor;
    // this will move into TextCursor with time
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;
    int _startSelectX = -1;
    int _startSelectY = -1;
    int _endSelectX = -1;
    int _endSelectY = -1;
    int _saveCursorPositionX = 0;
    // </>

    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
    int _tabsize = 8;
    bool _tabOption = true;
    bool _eatSpaceBeforeTabs = true;
    Tui::ZTextOption::WrapMode _wrapOption = Tui::ZTextOption::NoWrap;
    bool _formatting_characters = true;
    bool _overwrite = false;
    QString _searchText;
    QString _replaceText;
    bool _searchWrap = true;
    bool _searchReg = false;
    bool _searchDirection = true;
    std::shared_ptr<std::atomic<int>> searchGeneration = std::make_shared<std::atomic<int>>();
    std::shared_ptr<std::atomic<int>> searchNextGeneration = std::make_shared<std::atomic<int>>();
    bool _follow = false;
    int _bracketX = -1;
    int _bracketY = -1;
    bool _bracket = false;
    QJsonObject _jo;
    bool _noattr = false;
    QString _attributesfile;
    bool _msdos = false;
    bool _linenumber = false;
    bool _saveAs = true;
    bool _formattingCharacters = true;
    bool _colorTabs = true;
    bool _colorSpaceEnd = true;

    Tui::ZCommandNotifier *_cmdCopy = nullptr;
    Tui::ZCommandNotifier *_cmdCut = nullptr;
    Tui::ZCommandNotifier *_cmdPaste = nullptr;
    Tui::ZCommandNotifier *_cmdUndo = nullptr;
    Tui::ZCommandNotifier *_cmdRedo = nullptr;
    Tui::ZCommandNotifier *_cmdSearchNext = nullptr;
    Tui::ZCommandNotifier *_cmdSearchPrevious = nullptr;
    bool _selectMode = false;
    bool _blockSelect = false;

    friend class Document;
};

#endif // FILE_H
