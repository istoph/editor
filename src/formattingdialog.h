// SPDX-License-Identifier: BSL-1.0

#ifndef FORMATTINGDIALOG_H
#define FORMATTINGDIALOG_H

#include <Tui/ZCheckBox.h>
#include <Tui/ZDialog.h>


class FormattingDialog : public Tui::ZDialog {
    Q_OBJECT
public:
    FormattingDialog(Tui::ZWidget *parent);
    void updateSettings(bool formattingCharacters, bool colorTabs, bool colorSpaceEnd);
signals:
    void settingsChanged(bool formattingCharacters, bool colorTabs, bool colorSpaceEnd);
private:
    Tui::ZCheckBox *_formattingCharacters = nullptr;
    Tui::ZCheckBox *_colorTabs = nullptr;
    Tui::ZCheckBox *_colorSpaceEnd = nullptr;
};

#endif // FORMATTINGDIALOG_H
