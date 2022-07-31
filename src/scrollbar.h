#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <QTimer>

#include <Tui/ZWidget.h>
#include <testtui_lib.h>

class ScrollBar : public Tui::ZWidget {
    Q_OBJECT
public:
    ScrollBar(Tui::ZWidget *parent);

    bool transparent() const;

public slots:
    void scrollPosition(int x, int y);
    void positonMax(int x, int y);
    void setAutoHide(bool val);
    void setTransparent(bool transparent);

protected:
    void paintEvent(Tui::ZPaintEvent *event);

private:
    void autoHideExpired();

private:
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
    int _positionMaxX = 0;
    int _positionMaxY = 0;

    QTimer _autoHide;
    bool _autoHideEnabled = false;
    bool _transparent = false;
};

#endif // SCROLLBAR_H
