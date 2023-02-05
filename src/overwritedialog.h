// SPDX-License-Identifier: BSL-1.0

#ifndef OVERWRITEDIALOG_H
#define OVERWRITEDIALOG_H

#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>


class OverwriteDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    OverwriteDialog(Tui::ZWidget *parent, QString fileName);

signals:
    void confirm(bool confirm);

private:
    Tui::ZButton *_okButton = nullptr;
    Tui::ZButton *_cancelButton = nullptr;
};

#endif // OVERWRITEDIALOG_H
