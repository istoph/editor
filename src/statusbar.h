#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <testtui_lib.h>

class StatusBar : public Tui::ZWidget {
    Q_OBJECT
public:
    StatusBar(Tui::ZWidget *parent);

public slots:
    void cursorPosition(int x, int y);
    void scrollPosition(int x, int y);

protected:
    void paintEvent(Tui::ZPaintEvent *event);
private:
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
};

#endif // STATUSBAR_H
