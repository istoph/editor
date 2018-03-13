#include "scrollbar.h"

ScrollBar::ScrollBar(Tui::ZWidget *parent) : Tui::ZWidget(parent) {

}
void ScrollBar::paintEvent(Tui::ZPaintEvent *event) {

    auto *painter = event->painter();
    Tui::ZColor controlfg = getColor("scrollbar.control.fg");
    Tui::ZColor controlbg = getColor("scrollbar.control.bg");
    Tui::ZColor fg = getColor("scrollbar.fg");
    Tui::ZColor bg = getColor("scrollbar.bg");

    painter->clear(fg, bg);

    int thumbHeight;
    int trackBarPosition = 0;
    int trackBarSize;
    int currentPosition;
    int maxPosition;

    if (geometry().width() == 1) {
        trackBarSize = this->geometry().height()-2;
        currentPosition = _scrollPositionY;
        maxPosition = _positionMaxY;
    } else {
        trackBarSize = this->geometry().width()-2;
        currentPosition = _scrollPositionX;
        maxPosition = _positionMaxX;
    }

    if (maxPosition <= 0) {
        thumbHeight = trackBarSize;
    } else if (maxPosition == 1) {
        thumbHeight = trackBarSize - 1;
    } else if (maxPosition == 2) {
        thumbHeight = trackBarSize - 2;
    } else {
        thumbHeight = (trackBarSize - 2) - (maxPosition - 2) / 10;
        if (thumbHeight < 1) {
            thumbHeight = 1;
        }
    }

    if (currentPosition == 0) {
        trackBarPosition = 0;
    } else if (currentPosition == maxPosition) {
        trackBarPosition = trackBarSize - thumbHeight;
    } else {
        trackBarPosition = 1 + ((double)(trackBarSize -1) - thumbHeight) * ((double)currentPosition / (maxPosition));
    }

    if (geometry().width() == 1) {
        int y = 1 + trackBarPosition;
        painter->writeWithColors(0, 0, "↑", controlfg, controlbg);
        for (int i=0; i<thumbHeight; i++) {
            painter->writeWithColors(0, y, "▓", controlfg, controlbg);
            y+=1;
        }
        painter->writeWithColors(0, trackBarSize + 1, "↓", controlfg, controlbg);
    } else {
        int x = 1 + trackBarPosition;
        painter->writeWithColors(0, 0, "←", controlfg, controlbg);
        for (int i=0; i<thumbHeight; i++) {
            painter->writeWithColors(x, 0, "▓", controlfg, controlbg);
            x+=1;
        }
        painter->writeWithColors(trackBarSize + 1, 0, "→", controlfg, controlbg);
    }
}

void ScrollBar::scrollPosition(int x, int y)
{
    _scrollPositionX = x;
    _scrollPositionY = y;
}

void ScrollBar::positonMax(int x, int y)
{
    _positionMaxX = x;
    _positionMaxY = y;
}
