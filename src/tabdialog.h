// SPDX-License-Identifier: BSL-1.0

#ifndef TABDIALOG_H
#define TABDIALOG_H

#include <Tui/ZCheckBox.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZRadioButton.h>


class TabDialog : public Tui::ZDialog {
    Q_OBJECT
public:
    TabDialog(Tui::ZWidget *parent);

public:
    void updateSettings(bool useTabs, int indentSize, bool eatSpaceBeforeTabs);

signals:
    void convert(bool useTabs, int indentSize);
    void settingsChanged(bool useTabs, int indentSize, bool eat);

private:
    Tui::ZRadioButton *_tabRadioButton = nullptr;
    Tui::ZRadioButton *_blankRadioButton = nullptr;
    Tui::ZCheckBox *_eatSpaceBeforeTabBox = nullptr;
    Tui::ZInputBox *_tabsizeInputBox = nullptr;
};

#endif // TABDIALOG_H
