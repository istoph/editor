#ifndef WRAPDIALOG_H
#define WRAPDIALOG_H

#include <Tui/ZRadioButton.h>
#include <Tui/ZButton.h>

#include <file.h>

#include <testtui_lib.h>

class WrapDialog : public Dialog {
    Q_OBJECT
public:
    WrapDialog(Tui::ZWidget *parent, File *file = nullptr);

private:
    Tui::ZRadioButton *_noWrapRadioButton = nullptr;
    Tui::ZRadioButton *_wordWrapRadioButton = nullptr;
    Tui::ZRadioButton *_hardWrapRadioButton = nullptr;
};

#endif // WRAPDIALOG_H
