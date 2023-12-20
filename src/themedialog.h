// SPDX-License-Identifier: BSL-1.0

#ifndef THEMEDIALOG_H
#define THEMEDIALOG_H

#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZListView.h>


class Editor;

class ThemeDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    ThemeDialog(Editor *edit);

public slots:
    void open();
    void close();

private:
    Tui::ZListView *_lv = nullptr;
    Tui::ZButton *_cancelButton = nullptr;
    Tui::ZButton *_okButton = nullptr;
};

#endif // THEMEDIALOG_H
