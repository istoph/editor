// SPDX-License-Identifier: BSL-1.0

#ifndef EDIT_H
#define EDIT_H

#include <Tui/ZMenuItem.h>
#include <Tui/ZRoot.h>

#include "commandlinewidget.h"
#include "file.h"
#include "filewindow.h"
#include "help.h"
#include "mdilayout.h"
#include "searchdialog.h"
#include "statemux.h"
#include "statusbar.h"
#include "syntaxhighlightdialog.h"
#include "tabdialog.h"
#include "themedialog.h"


class Settings {
public:
    int tabSize = 4;
    bool tabOption = false;
    bool eatSpaceBeforeTabs = true;
    bool formattingCharacters = false;
    bool colorSpaceEnd = false;
    bool colorTabs = false;
    Tui::ZTextOption::WrapMode wrap = Tui::ZTextOption::NoWrap;
    bool highlightBracket = true;
    bool showLineNumber = false;
    QString attributesFile;
    int rightMarginHint = 0;
    QString syntaxHighlightingTheme;
    bool disableSyntaxHighlighting = false;
};

class Editor : public Tui::ZRoot {
    Q_OBJECT

public:
    enum class Theme {
        classic,
        dark
    };

public:
    Editor();
    ~Editor();

public:
    void setTheme(Theme theme);
    void setInitialFileSettings(const Settings &initial);
    void windowTitle(QString filename);
    FileWindow* openFile(QString fileName);
    void openFileDialog(QString path = "");
    FileWindow* newFile(QString filename = "");
    void openFileMenu();

    void gotoLineInCurrentFile(QString lineInfo);
    void followInCurrentFile(bool follow=true);

    void watchPipe();

    void setStartActions(std::vector<std::function<void()>> actions);

public slots:
    void showCommandLine();

private slots:
    void commandLineDismissed();
    void commandLineExecute(QString cmd);

protected:
    void terminalChanged() override;

private:
    void setupUi();
    void setupSearchDialogs();
    void ensureWindowCommands(int count);
    void enableFileCommands(bool enable);
    FileWindow *createFileWindow();
    QVector<Tui::ZMenuItem> createWindowMenu();
    void quit();
    void quitImpl(int i);
    void searchDialog();
    void replaceDialog();

private:
    File *_file = nullptr;
    FileWindow *_win = nullptr;
    MdiLayout *_mdiLayout = nullptr;
    StateMux _mux;
    QMap<QString, FileWindow*> _nameToWindow;
    QVector<FileWindow*> _allWindows;
    Tui::ZCommandNotifier *_cmdSearch = nullptr;
    SearchDialog *_searchDialog = nullptr;
    Tui::ZCommandNotifier *_cmdReplace = nullptr;
    SearchDialog *_replaceDialog = nullptr;
    ThemeDialog *_themeDialog = nullptr;
    QPointer<TabDialog> _tabDialog = nullptr;
    Tui::ZCommandNotifier *_cmdSyntaxHighlight = nullptr;
    QPointer<SyntaxHighlightDialog> _syntaxHighlightDialog = nullptr;
    StatusBar *_statusBar = nullptr;
    CommandLineWidget *_commandLineWidget = nullptr;
    Theme _theme = Theme::classic;
    Settings _initialFileSettings;
    QPointer<Help> _helpDialog;

    Tui::ZCommandNotifier *_cmdTab = nullptr;
    Tui::ZWindow *_optionTab = nullptr;
    Tui::ZWindow *_fileOpen = nullptr;
    Tui::ZWindow *_fileGotoLine = nullptr;
    Tui::ZCommandNotifier *_cmdInsertCharacter = nullptr;
    Tui::ZCommandNotifier *_cmdGotoLine = nullptr;
    Tui::ZCommandNotifier *_cmdSortSelectedLines = nullptr;
    Tui::ZCommandNotifier *_cmdLineNumbers = nullptr;
    Tui::ZCommandNotifier *_cmdFormatting = nullptr;
    Tui::ZCommandNotifier *_cmdBrackets = nullptr;
    Tui::ZCommandNotifier *_cmdTileVert = nullptr;
    Tui::ZCommandNotifier *_cmdTileHorz = nullptr;
    Tui::ZCommandNotifier *_cmdTileFull = nullptr;
    Tui::ZCommandNotifier *_cmdNextWindow = nullptr;
    Tui::ZCommandNotifier *_cmdPreviousWindow = nullptr;

    int _tab = 8;
    Tui::ZWindow *_pendingKeySequence = nullptr;
    QTimer _pendingKeySequenceTimer;
    std::vector<std::function<void()>> _startActions;
    int _windowCommandsCreated = 0;
};

#endif // EDIT_H
