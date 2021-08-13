#ifndef TABDIALOG_H
#define TABDIALOG_H

#include <testtui_lib.h>

#include "file.h"

class TabDialog : public Dialog {
    Q_OBJECT
public:
    TabDialog(Tui::ZWidget *parent);

public:
    void updateSettings(bool useTabs, int indentSize);

signals:
    void convert(bool useTabs, int indentSize);
    void settingsChanged(bool useTabs, int indentSize);

private:
    RadioButton *_tabRadioButton = nullptr;
    RadioButton *_blankRadioButton = nullptr;
    InputBox *_tabsizeInputBox = nullptr;
};

#endif // TABDIALOG_H
