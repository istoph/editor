#ifndef TABDIALOG_H
#define TABDIALOG_H

#include "file.h"
#include <testtui_lib.h>

class TabDialog : public Dialog {
public:
    TabDialog(File *file, Tui::ZWidget *parent);
};

#endif // TABDIALOG_H
