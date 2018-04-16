#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <testtui_lib.h>

#include "file.h"

class SearchDialog : public Dialog {
    Q_OBJECT

public:
    SearchDialog(Tui::ZWidget *parent, File* file);

public slots:
    void open();

private:
    InputBox *searchText = nullptr;
    Button *okBtn = nullptr;
};

#endif // SEARCHDIALOG_H
