// SPDX-License-Identifier: BSL-1.0

#ifndef EDIT_H
#define EDIT_H

#include <Tui/ZMenuItem.h>
#include <Tui/ZRoot.h>

#include "clipboard.h"
#include "commandlinewidget.h"
#include "file.h"
#include "filewindow.h"
#include "mdilayout.h"
#include "searchdialog.h"
#include "statemux.h"
#include "statusbar.h"
#include "tabdialog.h"
#include "themedialog.h"


class Settings {
public:
    int tabSize = 4;
    bool tabOption = true;
    bool eatSpaceBeforeTabs = true;
    bool formattingCharacters = false;
    bool colorSpaceEnd = false;
    bool colorTabs = false;
    Tui::ZTextOption::WrapMode wrap = Tui::ZTextOption::NoWrap;
    bool highlightBracket = false;
    bool showLineNumber = false;
    QString attributesFile;
    bool select_cursor_position_x0 = true;
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
    void openFile(QString fileName);
    void openFileDialog(QString path = "");
    void newFile(QString filename = "");
    void openFileMenue();

    void gotoLineInCurrentFile(QString lineInfo);
    void followInCurrentFile();

    void watchPipe();

public:
    QObject *facet(const QMetaObject &metaObject) const override;

public slots:
    void showCommandLine();

private slots:
    void commandLineDismissed();
    void commandLineExecute(QString cmd);

protected:
    void terminalChanged() override;

private:
    void setupSearchDialogs();
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
    SearchDialog *_searchDialog = nullptr;
    SearchDialog *_replaceDialog = nullptr;
    ThemeDialog *_themeDialog = nullptr;
    QPointer<TabDialog> _tabDialog = nullptr;
    StatusBar *_statusBar = nullptr;
    CommandLineWidget *_commandLineWidget = nullptr;
    mutable Clipboard _clipboard;
    Theme _theme = Theme::classic;
    Settings initialFileSettings;

    Tui::ZWindow *_option_tab = nullptr;
    Tui::ZWindow *_file_open = nullptr;
    Tui::ZWindow *_file_goto_line = nullptr;
    int _tab = 8;
    Tui::ZWindow *pendingKeySequence = nullptr;
    QTimer pendingKeySequenceTimer;
};

#endif // EDIT_H
