// SPDX-License-Identifier: BSL-1.0

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <Tui/ZDialog.h>

class AboutDialog : public Tui::ZDialog {
    Q_OBJECT
public:
    AboutDialog(Tui::ZWidget *parent);
};

#endif // ABOUTDIALOG_H
