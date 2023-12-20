// SPDX-License-Identifier: BSL-1.0

#ifndef FILE_H
#define FILE_H

#include <memory>
#include <mutex>
#include <optional>
#include <variant>

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
#include <Tui/ZTextEdit.h>
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
    unsigned lineRevision = -1;
};
struct Updates {
    QList<std::shared_ptr<ExtraData>> data;
    QList<int> lines;
    unsigned documentRevision = 0;
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

class File : public Tui::ZTextEdit {
    Q_OBJECT

public:
    explicit File(Tui::ZTextMetrics textMetrics, Tui::ZWidget *parent);
    ~File();
    bool setFilename(QString _filename);
    QString getFilename();
    bool saveText();
    bool openText(QString filename);
    void cutline();
    void deleteLine();
    void copy() override;
    void paste() override;
    void gotoLine(QString pos);
    void setEatSpaceBeforeTabs(bool eat);
    bool eatSpaceBeforeTabs();
    bool formattingCharacters() const;
    void setFormattingCharacters(bool formattingCharacters);
    bool colorTabs() const;
    void setColorTabs(bool colorTabs);
    bool colorTrailingSpaces();
    void setColorTrailingSpaces(bool colorTrailingSpaces);
    QString selectedText();
    bool hasSelection();
    bool removeSelectedText();
    void appendLine(const QString &line);
    void insertText(const QString &str);
    void setSearchText(QString searchText);
    void setSearchCaseSensitivity(Qt::CaseSensitivity searchCaseSensitivity);
    void setSearchVisible(bool visible);
    bool searchVisible();
    void setReplaceText(QString replaceText);
    bool isSearchMatchSelected();
    void replaceSelected();
    void setHighlightBracket(bool hb);
    bool highlightBracket();
    bool writeAttributes();
    void setAttributesFile(QString attributesFile);
    QString attributesFile();
    int convertTabsToSpaces();
    int replaceAll(QString searchText, QString replaceText);
    void setRightMarginHint(int hint);
    int rightMarginHint() const;
    bool isNewFile();
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
    void toggleShowLineNumbers();
    void setSaveAs(bool state);
    bool isSaveAs();
    bool newText(QString filename);
    bool stdinText();
    void sortSelecedLines();

    bool event(QEvent *event) override;
    bool followStandardInput();

public slots:
    void setFollowStandardInput(bool follow);

signals:
    void followStandardInputChanged(bool follow);
    void writableChanged(bool rw);
    void searchCountChanged(int sc);
    void searchTextChanged(QString searchText);
    void searchVisibleChanged(bool visible);
    void syntaxHighlightingLanguageChanged(QString language);
    void syntaxHighlightingEnabledChanged(bool enable);

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;
    void pasteEvent(Tui::ZPasteEvent *event) override;
    void focusInEvent(Tui::ZFocusEvent *event) override;

    void clearAdvancedSelection() override;

    bool canCut() override;
    bool canCopy() override;

private:
    bool initText();
    void adjustScrollPosition() override;
    void emitCursorPostionChanged() override;

    bool hasBlockSelection() const;
    bool hasMultiInsert() const;

    Tui::ZTextOption textOption() const override;

    bool highlightBracketFind();
    void searchSelect(int line, int found, int length, bool direction);
    int pageNavigationLineCount() const override;
    void checkWritable();

    QPair<int, int> getSelectedLinesSort();
    QPair<int, int> getSelectedLines();
    void selectLines(int startY, int endY);

    bool readAttributes();
    Tui::ZDocumentCursor::Position getAttributes();

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

    // block selection
    bool _blockSelect = false;
    std::optional<Tui::ZDocumentLineMarker> _blockSelectStartLine;
    std::optional<Tui::ZDocumentLineMarker> _blockSelectEndLine;
    int _blockSelectStartColumn = -1;
    int _blockSelectEndColumn = -1;

    bool _eatSpaceBeforeTabs = true;
    QString _searchText;
    Qt::CaseSensitivity _searchCaseSensitivity = Qt::CaseSensitivity::CaseSensitive;
    QString _replaceText;
    std::optional<std::variant<std::monostate, Tui::ZDocumentFindAsyncResult, Tui::ZDocumentFindResult>> _currentSearchMatch;
    bool _searchWrap = true;
    bool _searchRegex = false;
    bool _searchDirectionForward = true;
    bool _searchVisible = false;
    std::shared_ptr<std::atomic<int>> searchGeneration = std::make_shared<std::atomic<int>>();
    std::optional<QFuture<Tui::ZDocumentFindAsyncResult>> _searchNextFuture;
    bool _followMode = false;
    bool _stdin = false;
    Position _bracketPosition;
    bool _bracket = false;
    QJsonObject _attributeObject;
    QString _attributesFile;
    bool _saveAs = true;
    bool _formattingCharacters = true;
    int _rightMarginHint = 0;
    bool _colorTabs = true;
    bool _colorTrailingSpaces = true;

    Tui::ZCommandNotifier *_cmdSearchNext = nullptr;
    Tui::ZCommandNotifier *_cmdSearchPrevious = nullptr;

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
