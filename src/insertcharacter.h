// SPDX-License-Identifier: BSL-1.0

#ifndef INSERTCHARACTER_H
#define INSERTCHARACTER_H

#include <Tui/ZDialog.h>


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
    bool _check = false;
};

#endif // INSERTCHARACTER_H
