#ifndef EDIT_H
#define EDIT_H

#include <QCoreApplication>

#include <Tui/ZCommandNotifier.h>
#include <Tui/ZPalette.h>
#include <Tui/ZRoot.h>
#include <Tui/ZSymbol.h>

#include <testtui_lib.h>

#include <file.h>
#include <scrollbar.h>
#include <statusbar.h>

class Editor : public Tui::ZRoot {
    Q_OBJECT

public:
    Editor();

    WindowWidget *win;
    WindowWidget *option_tab;
    File *file;
    Label *file_name;
    int tab = 8;

};

#endif // EDIT_H
