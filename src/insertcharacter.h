#ifndef INSERTCHARACTER_H
#define INSERTCHARACTER_H

#include <testtui_lib.h>

#include "file.h"

class InsertCharacter : public Dialog {
    Q_OBJECT

public:
    InsertCharacter(Tui::ZWidget *parent, File *file);
    void rejected();

private:
    QString intToChar(int i);
    int _codepoint = 0;
    bool _check;
};

#endif // INSERTCHARACTER_H