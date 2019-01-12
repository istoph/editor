#ifndef TABDIALOG_H
#define TABDIALOG_H

#include <testtui_lib.h>

#include "file.h"

class TabDialog : public Dialog {
public:
    TabDialog(File *file, Tui::ZWidget *parent);
};

#endif // TABDIALOG_H
