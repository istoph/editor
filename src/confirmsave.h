#ifndef QUITDIALOG_H
#define QUITDIALOG_H

#include <testtui_lib.h>

#include "file.h"

class ConfirmSave : public Dialog
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
