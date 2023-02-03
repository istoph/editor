// SPDX-License-Identifier: BSL-1.0

#ifndef QUITDIALOG_H
#define QUITDIALOG_H

#include <Tui/ZDialog.h>

#include "file.h"

class ConfirmSave : public Tui::ZDialog
{
public:
    enum Type { Reload, Close, Quit };
    Q_OBJECT
public:
    explicit ConfirmSave(Tui::ZWidget *parent, QString filename, Type type, bool saveable);

signals:
    void saveSelected();
    void exitSelected();
};

#endif // QUITDIALOG_H
