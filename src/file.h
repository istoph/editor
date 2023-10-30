// SPDX-License-Identifier: BSL-1.0

#ifndef FILE_H
#define FILE_H

#include <memory>
#include <mutex>
#include <optional>

#include <QJsonObject>
#include <QPair>

#ifdef SYNTAX_HIGHLIGHTING
#include <KSyntaxHighlighting/AbstractHighlighter>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/State>
#include <KSyntaxHighlighting/Theme>
#endif

#include <Tui/ZCommandNotifier.h>
#include <Tui/ZDocument.h>
#include <Tui/ZDocumentLineMarker.h>
#include <Tui/ZDocumentSnapshot.h>
#include <Tui/ZTextLayout.h>
#include <Tui/ZTextMetrics.h>
#include <Tui/ZTextOption.h>
#include <Tui/ZWidget.h>


struct ExtraData : public Tui::ZDocumentLineUserData {
#ifdef SYNTAX_HIGHLIGHTING
    KSyntaxHighlighting::State stateBegin;
    KSyntaxHighlighting::State stateEnd;
#endif
    QVector<Tui::ZFormatRange> highlights;
    int lineRevision = -1;
};
struct Updates {
    QList<std::shared_ptr<ExtraData>> data;
    QList<int> lines;
    int documentRevision = 0;
};

Q_DECLARE_METATYPE(Updates);

class SyntaxHighlightingSignalForwarder : public QObject {
    Q_OBJECT
signals:
    void updates(Updates);
};

#ifdef SYNTAX_HIGHLIGHTING

class HighlightExporter : public KSyntaxHighlighting::AbstractHighlighter {
public:
    std::tuple<KSyntaxHighlighting::State, QVector<Tui::ZFormatRange>> highlightLineWrap(const QString &text, const KSyntaxHighlighting::State &state);

    Tui::ZColor defBg;
    Tui::ZColor defFg;

protected:
    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) override;

protected:
    // The whole object graph of the syntax highlighter is not safe to share across threads...
    // so we need to use a fine grained lock to avoid blocking the UI thread for too long.
    std::mutex mutex;

    QVector<Tui::ZFormatRange> highlights;
};

#endif

class File : public Tui::ZWidget {
    Q_OBJECT

    friend class DocumentTestHelper;
public:
    using Position = Tui::ZDocumentCursor::Position;

public:
    explicit File(Tui::ZTextMetrics textMetrics, Tui::ZWidget *parent);
    ~File();
    Tui::ZDocument *document() { return _doc.get(); }
    bool setFilename(QString _filename);
    QString getFilename();
    bool saveText();
    bool openText(QString filename);
    void cut();
    void cutline();
    void deleteLine();
    void copy();
    void paste();
    void gotoLine(QString pos);
    bool setTabStopDistance(int tab);
    int tabStopDistance();
    void setTabOption(bool tab);
    bool getTabOption();
    void setEatSpaceBeforeTabs(bool eat);
    bool eatSpaceBeforeTabs();
    bool formattingCharacters();
    void setFormattingCharacters(bool formattingCharacters);
    bool colorTabs();
    void setColorTabs(bool colorTabs);
    bool colorTrailingSpaces();
    void setColorTrailingSpaces(bool colorTrailingSpaces);
    void setWordWrapMode(Tui::ZTextOption::WrapMode wrap);
    Tui::ZTextOption::WrapMode wordWrapMode();
    void clearSelection();
    QString selectedText();
    bool hasSelection();
    void selectAll();
    bool removeSelectedText();
    void toggleOverwriteMode();
    void setOverwriteMode(bool mode);
    bool overwriteMode();
    void addTabAt(Tui::ZDocumentCursor &cur);
    void appendLine(const QString &line);
    void insertText(const QString &str);
    bool isModified() const { return _doc->isModified(); };
    void setSearchText(QString searchText);
    void setSearchCaseSensitivity(Qt::CaseSensitivity searchCaseSensitivity);
    void setReplaceText(QString replaceText);
    void setReplaceSelected();
    void setHighlightBracket(bool hb);
    bool highlightBracket();
    bool writeAttributes();
    void setAttributesFile(QString attributesFile);
    QString attributesFile();
    int convertTabsToSpaces();
    int replaceAll(QString searchText, QString replaceText);
    void setCursorPosition(Tui::ZDocumentCursor::Position position);
    Tui::ZDocumentCursor::Position cursorPosition();
    QPoint scrollPosition();
    void setRightMarginHint(int hint);
    int rightMarginHint() const;
    void toggleSelectMode();
    void setSelectMode(bool mode);
    bool selectMode();
    bool isNewFile();
    void undo();
    void redo();
    void setSyntaxHighlightingTheme(QString themeName);
    void setSyntaxHighlightingLanguage(QString language);
    QString syntaxHighlightingLanguage();
    void setSyntaxHighlightingActive(bool active);
    bool syntaxHighlightingActive();

public:
    void setSearchWrap(bool wrap);
    bool searchWrap();
    void setRegex(bool reg);
    bool getWritable();
    void runSearch(bool direction);
    void setSearchDirection(bool searchDirection);
    void setShowLineNumbers(bool show);
    bool showLineNumbers();
    void toggleShowLineNumbers();
    void setSaveAs(bool state);
    bool isSaveAs();
    bool newText(QString filename);
    bool stdinText();
    void sortSelecedLines();

    bool event(QEvent *event) override;

public slots:
    void followStandardInput(bool follow);

signals:
    void cursorPositionChanged(int x, int utf8x, int line);
    void scrollPositionChanged(int x, int y);
    void textMax(int x, int y);
    void modifiedChanged(bool modified);
    void writableChanged(bool rw);
    void selectModeChanged(bool mode);
    void searchCountChanged(int sc);
    void searchTextChanged(QString searchText);
    void overwriteModeChanged(bool overwrite);
    void syntaxHighlightingLanguageChanged(QString language);
    void syntaxHighlightingEnabledChanged(bool enable);

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
    bool highlightBracketFind();
    void searchSelect(int line, int found, int length, bool direction);
    int lineNumberBorderWidth();
    int getVisibleLines();
    void checkWritable();

    QPair<int, int> getSelectedLinesSort();
    QPair<int, int> getSelectedLines();
    void selectLines(int startY, int endY);

    bool readAttributes();
    Tui::ZDocumentCursor::Position getAttributes();

    Tui::ZTextLayout getTextLayoutForLineWithoutWrapping(int line);
    Tui::ZDocumentCursor createCursor();
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
#ifdef SYNTAX_HIGHLIGHTING
    void ingestSyntaxHighlightingUpdates(Updates);
    void updateSyntaxHighlighting(bool force);
    void syntaxHighlightDefinition();
#endif

private:
    Tui::ZTextMetrics _textMetrics;
    std::shared_ptr<Tui::ZDocument> _doc;
    Tui::ZDocumentCursor _cursor;

    // block selection
    bool _blockSelect = false;
    std::optional<Tui::ZDocumentLineMarker> _blockSelectStartLine;
    std::optional<Tui::ZDocumentLineMarker> _blockSelectEndLine;
    int _blockSelectStartColumn = -1;
    int _blockSelectEndColumn = -1;

    int _scrollPositionColumns = 0;
    Tui::ZDocumentLineMarker _scrollPositionLine;
    int _scrollPositionFineLine = 0;
    bool _detachedScrolling = false;
    int _tabsize = 8;
    bool _tabOption = false;
    bool _eatSpaceBeforeTabs = true;
    Tui::ZTextOption::WrapMode _wrapMode = Tui::ZTextOption::NoWrap;
    bool _overwriteMode = false;
    QString _searchText;
    Qt::CaseSensitivity _searchCaseSensitivity = Qt::CaseSensitivity::CaseSensitive;
    QString _replaceText;
    bool _searchWrap = true;
    bool _searchRegex = false;
    bool _searchDirectionForward = true;
    std::shared_ptr<std::atomic<int>> searchGeneration = std::make_shared<std::atomic<int>>();
    std::optional<QFuture<Tui::ZDocumentFindAsyncResult>> _searchNextFuture;
    bool _followMode = false;
    Position _bracketPosition;
    bool _bracket = false;
    QJsonObject _attributeObject;
    QString _attributesFile;
    bool _showLineNumbers = false;
    bool _saveAs = true;
    bool _formattingCharacters = true;
    int _rightMarginHint = 0;
    bool _colorTabs = true;
    bool _colorTrailingSpaces = true;

    Tui::ZCommandNotifier *_cmdCopy = nullptr;
    Tui::ZCommandNotifier *_cmdCut = nullptr;
    Tui::ZCommandNotifier *_cmdPaste = nullptr;
    Tui::ZCommandNotifier *_cmdUndo = nullptr;
    Tui::ZCommandNotifier *_cmdRedo = nullptr;
    Tui::ZCommandNotifier *_cmdSearchNext = nullptr;
    Tui::ZCommandNotifier *_cmdSearchPrevious = nullptr;
    bool _selectMode = false;

    // Syntax highlighting
    QString _syntaxHighlightingThemeName;
    QString _syntaxHighlightingLanguage = "None";
    bool _syntaxHighlightingActive = false;
#ifdef SYNTAX_HIGHLIGHTING
    KSyntaxHighlighting::Repository _syntaxHighlightRepo;
    KSyntaxHighlighting::Theme _syntaxHighlightingTheme;
    KSyntaxHighlighting::Definition _syntaxHighlightDefinition;
    HighlightExporter _syntaxHighlightExporter;
#endif
};

#endif // FILE_H
