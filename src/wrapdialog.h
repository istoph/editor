// SPDX-License-Identifier: BSL-1.0

#ifndef WRAPDIALOG_H
#define WRAPDIALOG_H

#include <Tui/ZDialog.h>
#include <Tui/ZRadioButton.h>

#include <file.h>


class WrapDialog : public Tui::ZDialog {
    Q_OBJECT
public:
    WrapDialog(Tui::ZWidget *parent, File *file = nullptr);

private:
    Tui::ZRadioButton *_noWrapRadioButton = nullptr;
    Tui::ZRadioButton *_wordWrapRadioButton = nullptr;
    Tui::ZRadioButton *_wrapAnywhereRadioButton = nullptr;
};

#endif // WRAPDIALOG_H
