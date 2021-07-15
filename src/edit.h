#ifndef EDIT_H
#define EDIT_H

#include <QCoreApplication>
#include <QSocketNotifier>

#include <Tui/ZCommandNotifier.h>
#include <Tui/ZPalette.h>
#include <Tui/ZRoot.h>
#include <Tui/ZSymbol.h>

#include <testtui_lib.h>

#include "file.h"
#include "opendialog.h"
#include "savedialog.h"
#include "confirmsave.h"
#include "scrollbar.h"
#include "searchdialog.h"
#include "statusbar.h"
#include "tabdialog.h"
#include "insertcharacter.h"
#include "gotoline.h"
#include "clipboard.h"
#include "commandlinewidget.h"
#include "themedialog.h"

#include <QDir>
#include <QSettings>
#include <QTextStream>
#include <QCommandLineParser>
#include <QFileSystemWatcher>

//Debug Sleep
#include <unistd.h>

class Editor : public Tui::ZRoot {
    Q_OBJECT

public:
    Editor();

    WindowWidget *win;
    WindowWidget *option_tab;
    WindowWidget *file_open;
    WindowWidget *file_goto_line;
    File *file;
    int tab = 8;
    enum class Theme {
        classic,
        dark
    };

public:
    void closePipe();
    void watchPipe();
    void setTheme(Theme theme);
    void setFollow(bool follow);
    bool getFollow();
    void windowTitle(QString filename);
    void openFileDialog(QString path = "");
    void newFileMenue();
    void openFileMenue();
    QObject *facet(const QMetaObject metaObject);

public slots:
    void newFile(QString filename = "dokument.txt");
    void openFile(QString filename);
    void saveFile(QString filename);
    void setWrap(bool wrap);
    void showCommandLine();

private slots:
    void inputPipeReadable(int socket);
    void commandLineDismissed();
    void commandLineExecute(QString cmd);

signals:
    void readFromStandadInput(bool activ);
    void followStandadInput(bool follow);
    //void emitFollowMenueEnable(bool enable);
    void fileChanged(bool fileChanged);

private:
    SaveDialog *saveFileDialog();
    SaveDialog *saveOrSaveas();
    void reload();
    void quit();
    void searchDialog();
    void replaceDialog();
    void watcherAdd();
    void watcherRemove();

private:
    ScrollBar *_scrollbarHorizontal = nullptr;
    WindowLayout *_winLayout = nullptr;
    SearchDialog *_searchDialog = nullptr;
    SearchDialog *_replaceDialog = nullptr;
    ThemeDialog *_themeDialog = nullptr;
    QSocketNotifier *_pipeSocketNotifier = nullptr;
    QByteArray _pipeLineBuffer;
    Tui::ZCommandNotifier *_cmdFollow = nullptr;
    Tui::ZCommandNotifier *_cmdInputPipe = nullptr;
    bool _follow = false;
    QFileSystemWatcher *_watcher = nullptr;
    StatusBar *_statusBar = nullptr;
    CommandLineWidget *_commandLineWidget = nullptr;
    VBoxLayout *_rootLayout = nullptr; // only needed to workaround missing updating in ZLayout
    Clipboard _clipboard;
    Theme _theme = Theme::classic;
};

#endif // EDIT_H
