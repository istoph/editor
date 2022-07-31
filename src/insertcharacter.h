#ifndef INSERTCHARACTER_H
#define INSERTCHARACTER_H

#include <Tui/ZColor.h>
#include <Tui/ZDialog.h>
#include <Tui/ZPalette.h>

#include "file.h"

class InsertCharacter : public Tui::ZDialog {
    Q_OBJECT

public:
    InsertCharacter(Tui::ZWidget *parent);
    void rejected();

signals:
    void characterSelected(QString);

private:
    QString intToChar(int i);
    int _codepoint = 0;
    bool _check;
};

#endif // INSERTCHARACTER_H
