#ifndef TABDIALOG_H
#define TABDIALOG_H

#include <Tui/ZRadioButton.h>

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
    Tui::ZRadioButton *_tabRadioButton = nullptr;
    Tui::ZRadioButton *_blankRadioButton = nullptr;
    InputBox *_tabsizeInputBox = nullptr;
};

#endif // TABDIALOG_H
