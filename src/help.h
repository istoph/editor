// SPDX-License-Identifier: BSL-1.0

#ifndef HELP_H
#define HELP_H

#include <QObject>

#include <Tui/ZDialog.h>
#include <Tui/ZTextEdit.h>


class Help : public Tui::ZDialog {
    Q_OBJECT

public:
    explicit Help(Tui::ZWidget *parent);

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;

private:
    Tui::ZTextEdit *_text = nullptr;
};

#endif // HELP_H
