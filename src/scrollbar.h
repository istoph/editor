#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <testtui_lib.h>

class ScrollBar : public Tui::ZWidget {
    Q_OBJECT
public:
    ScrollBar(Tui::ZWidget *parent);

public slots:
    void scrollPosition(int x, int y);
    void positonMax(int x, int y);

protected:
    void paintEvent(Tui::ZPaintEvent *event);

private:
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
    int _positionMaxX = 0;
    int _positionMaxY = 0;
};

#endif // SCROLLBAR_H
