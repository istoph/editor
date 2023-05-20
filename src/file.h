// SPDX-License-Identifier: BSL-1.0

#ifndef FILE_H
#define FILE_H

#include <memory>
#include <optional>

#include <QJsonObject>
#include <QPair>

#include <Tui/ZCommandNotifier.h>
#include <Tui/ZTextLayout.h>
#include <Tui/ZTextOption.h>
#include <Tui/ZWidget.h>

#include "document.h"


struct SearchLine;
struct SearchParameter;

class File : public Tui::ZWidget {
    Q_OBJECT

    friend class DocumentTestHelper;
public:
    using Position = TextCursor::Position;

public:
    explicit File(Tui::ZWidget *parent);
    bool setFilename(QString _filename);
    QString getFilename();
    //QString emptyFilename();
    bool saveText();
    bool openText(QString filename);
    void cut();
    void cutline();
    void deleteLine();
    void copy();
    void paste();
    bool isInsertable();
    void insertLinebreak();
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
    QPair<int, int> getSelectLinesSort();
    QPair<int, int> getSelectLines();
    void selectLines(int startY, int endY);
    void resetSelect();
    QString getSelectText();
    bool isSelect();
    void selectAll();
    bool delSelect();
    void toggleOverwrite();
    bool isOverwrite();
    void addTabAt(TextCursor &cur);
    int getVisibleLines();
    void appendLine(const QString &line);
    void insertAtCursorPosition(const QString &str);
    bool isModified() const { return _doc->isModified(); };
    void setSearchText(QString searchText);
    void setSearchCaseSensitivity(Qt::CaseSensitivity searchCaseSensitivity);
    void setReplaceText(QString replaceText);
    void setReplaceSelected();
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
    void setCursorPosition(TextCursor::Position position);
    QPoint getScrollPosition();
    void setRightMarginHint(int hint);
    int rightMarginHint() const;
    void toggleSelectMode();
    bool isNewFile();
    void undo();
    void redo();

public:
    void setSearchWrap(bool wrap);
    bool getSearchWrap();
    void setRegex(bool reg);
    void checkWritable();
    bool getWritable();
    void runSearch(bool direction);
    void setSearchDirection(bool searchDirection);
    //void searchSelect(int found = -1);
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
    void focusInEvent(Tui::ZFocusEvent *event) override;

private:
    bool initText();
    void adjustScrollPosition();
    void updateCommands();

    bool hasBlockSelection() const;
    bool hasMultiInsert() const;

    Tui::ZTextOption getTextOption(bool lineWithCursor);
    Tui::ZTextLayout getTextLayoutForLine(const Tui::ZTextOption &option, int line);
    bool highlightBracket();
    void searchSelect(int line, int found, int length, bool direction);
    SearchLine searchNext(DocumentSnapshot snap, SearchParameter search);
    //void searchSelectPrevious(int line, int found);
    void setSelectMode(bool f4);
    bool getSelectMode();
    int shiftLinenumber();
    Tui::ZTextLayout getTextLayoutForLineWithoutWrapping(int line);
    TextCursor createCursor();
    std::tuple<int, int, int> cursorPositionOrBlockSelectionEnd();

    // block selection
    void activateBlockSelection();
    void disableBlockSelection();
    void blockSelectRemoveSelectedAndConvertToMultiInsert();
    static const int mi_add_spaces = 1;
    static const int mi_skip_short_lines = 2;
    template<typename F>
    void multiInsertForEachCursor(int flags, F f);
    void multiInsertDeletePreviousCharacter();
    void multiInsertDeletePreviousWord();
    void multiInsertDeleteCharacter();
    void multiInsertDeleteWord();
    void multiInsertInsert(const QString &text);

private:
    std::shared_ptr<Document> _doc;
    TextCursor _cursor;

    // block selection
    bool _blockSelect = false;
    std::optional<LineMarker> _blockSelectStartLine;
    std::optional<LineMarker> _blockSelectEndLine;
    int _blockSelectStartColumn = -1;
    int _blockSelectEndColumn = -1;

    int _scrollPositionX = 0;
    LineMarker _scrollPositionY;
    int _scrollFineLine = 0;
    int _tabsize = 8;
    bool _tabOption = false;
    bool _eatSpaceBeforeTabs = true;
    Tui::ZTextOption::WrapMode _wrapOption = Tui::ZTextOption::NoWrap;
    bool _overwrite = false;
    QString _searchText;
    Qt::CaseSensitivity _searchCaseSensitivity = Qt::CaseSensitivity::CaseSensitive;
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
    int _rightMarginHint = 0;
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
};

#endif // FILE_H
