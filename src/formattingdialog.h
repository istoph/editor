#ifndef FORMATTINGDIALOG_H
#define FORMATTINGDIALOG_H

#include <Tui/ZButton.h>
#include <Tui/ZLabel.h>
#include <Tui/ZCheckBox.h>

#include <testtui_lib.h>

class FormattingDialog : public Dialog {
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
