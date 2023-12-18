// SPDX-License-Identifier: BSL-1.0

#ifndef CONFIRMSAVE_H
#define CONFIRMSAVE_H

#include <Tui/ZDialog.h>


class ConfirmSave : public Tui::ZDialog
{
public:
    enum Type { Reload, Close, CloseUnnamed, Quit, QuitUnnamed };
    Q_OBJECT
public:
    explicit ConfirmSave(Tui::ZWidget *parent, QString filename, Type type, bool saveable);

signals:
    void saveSelected();
    void discardSelected();
};

#endif // CONFIRMSAVE_H
