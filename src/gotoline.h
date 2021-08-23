#ifndef GOTOLINE_H
#define GOTOLINE_H

#include <testtui_lib.h>

#include "file.h"

class GotoLine : public Dialog
{
public:
    Q_OBJECT

public:
    explicit GotoLine(Tui::ZWidget *parent);

signals:
    void lineSelected(QString line);
};

#endif // GOTOLINE_H
