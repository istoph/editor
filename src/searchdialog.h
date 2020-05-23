#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <testtui_lib.h>

#include "file.h"

class SearchDialog : public Dialog {
    Q_OBJECT

public:
    SearchDialog(Tui::ZWidget *parent, File *file);
    void setSearchText(QString text);

public slots:
    void open();

private:
    InputBox *_searchText = nullptr;
    Button *_findNextBtn = nullptr;
    Button *_findPreviousBtn = nullptr;
    Button *_cancelBtn = nullptr;
    bool _caseSensitive = true;
    CheckBox *_caseMatchBox = nullptr;
    CheckBox *_wrapBox = nullptr;
    CheckBox *_liveSearchBox = nullptr;
};

#endif // SEARCHDIALOG_H
