#ifndef QUITDIALOG_H
#define QUITDIALOG_H

#include <testtui_lib.h>

#include "file.h"

class ConfirmSave : public Dialog
{
public:
    enum Type { New, Open, Quit };
    Q_OBJECT
public:
    explicit ConfirmSave(Tui::ZWidget *parent, QString filename, Type type);

signals:
    void saveSelected();
    void exitSelected();
};

#endif // QUITDIALOG_H
