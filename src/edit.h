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
#include "filewindow.h"
#include "opendialog.h"
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

class Editor : public Tui::ZRoot {
    Q_OBJECT

public:
    Editor();

    FileWindow *win;
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
    void setTheme(Theme theme);
    void windowTitle(QString filename);
    void openFileDialog(QString path = "");
    void newFileMenue();
    void openFileMenue();
    QObject *facet(const QMetaObject metaObject);

public slots:
    void showCommandLine();

private slots:
    void commandLineDismissed();
    void commandLineExecute(QString cmd);

private:
    void quit();
    void searchDialog();
    void replaceDialog();

private:
    SearchDialog *_searchDialog = nullptr;
    SearchDialog *_replaceDialog = nullptr;
    ThemeDialog *_themeDialog = nullptr;
    StatusBar *_statusBar = nullptr;
    CommandLineWidget *_commandLineWidget = nullptr;
    Clipboard _clipboard;
    Theme _theme = Theme::classic;
};

#endif // EDIT_H
