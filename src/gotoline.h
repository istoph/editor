// SPDX-License-Identifier: BSL-1.0

#ifndef GOTOLINE_H
#define GOTOLINE_H

#include <Tui/ZDialog.h>

#include "file.h"

class GotoLine : public Tui::ZDialog
{
public:
    Q_OBJECT

public:
    explicit GotoLine(Tui::ZWidget *parent);

signals:
    void lineSelected(QString line);
};

#endif // GOTOLINE_H
