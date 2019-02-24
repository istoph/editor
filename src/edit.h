#ifndef EDIT_H
#define EDIT_H

#include <QCoreApplication>

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
#include "gotoline.h"

#include <QDir>
#include <QSettings>
#include <QTextStream>
#include <QCommandLineParser>

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

public slots:
    void newFile(QString filename = "dokument.txt");
    void openFile(QString filename);
    void saveFile(QString filename);

private:
    void openFileDialog();
    SaveDialog *saveFileDialog();
    SaveDialog *saveOrSaveas();
    void quit();
};

#endif // EDIT_H
