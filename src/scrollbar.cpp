#include "scrollbar.h"

ScrollBar::ScrollBar(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
    QObject::connect(&_autoHide, &QTimer::timeout, this, &ScrollBar::autoHideExpired);
}
void ScrollBar::paintEvent(Tui::ZPaintEvent *event) {

    auto *painter = event->painter();
    Tui::ZColor controlfg = getColor("scrollbar.control.fg");
    Tui::ZColor controlbg = getColor("scrollbar.control.bg");
    Tui::ZColor fg = getColor("scrollbar.fg");
    Tui::ZColor bg = getColor("scrollbar.bg");
    Tui::ZColor fgbehindText = {0xaa, 0xaa, 0xaa};

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
        if (!_transparent) {
            painter->clear(fg, bg);
        } else {
            for (int i = 0; i < geometry().height(); i++) {
                painter->setBackground(0,i,{ 0, 0x00, 0x80});
                painter->setForeground(0,i,fg);
            }
        }
        int y = 1 + trackBarPosition;
        painter->writeWithColors(0, 0, "↑", controlfg, controlbg);
        for (int i = 0; i < thumbHeight; i++) {
            if (!_transparent) {
                painter->writeWithColors(0, y, "▓", controlfg, controlbg);
            } else {
                painter->setBackground(0,y,{ 0, 0, 0xd9});
                painter->setForeground(0,y,fgbehindText);
            }
            y+=1;
        }
        painter->writeWithColors(0, trackBarSize + 1, "↓", controlfg, controlbg);
    } else {
        if (!_transparent) {
            painter->clear(fg, bg);
        } else {
            for (int i = 0; i < geometry().width(); i++) {
                painter->setBackground(i,0,{ 0, 0x00, 0x80});
                painter->setForeground(i,0,fgbehindText);
            }
        }
        int x = 1 + trackBarPosition;
        painter->writeWithColors(0, 0, "←", controlfg, controlbg);
        for (int i = 0; i < thumbHeight; i++) {
            if (!_transparent) {
                painter->writeWithColors(x, 0, "▓", controlfg, controlbg);
            } else {
                painter->setBackground(x,0,{ 0, 0, 0xd9});
                painter->setForeground(x,0,fgbehindText);
            }
            x+=1;
        }
        painter->writeWithColors(trackBarSize + 1, 0, "→", controlfg, controlbg);
    }
}

void ScrollBar::autoHideExpired() {
    setVisible(false);
}

bool ScrollBar::transparent() const {
    return _transparent;
}

void ScrollBar::setTransparent(bool transparent) {
    _transparent = transparent;
}

void ScrollBar::scrollPosition(int x, int y)
{
    if (_autoHideEnabled && _scrollPositionY != y) {
        setVisible(true);
        _autoHide.start();
    }
    _scrollPositionX = x;
    _scrollPositionY = y;
}

void ScrollBar::positonMax(int x, int y)
{
    _positionMaxX = x;
    _positionMaxY = y;
}

void ScrollBar::setAutoHide(bool val) {
    if (_autoHideEnabled == val) {
        return;
    }
    _autoHideEnabled = val;
    if (_autoHideEnabled) {
        _autoHide.setInterval(1000);
        _autoHide.setSingleShot(true);
        _autoHide.start();
    } else {
        _autoHide.stop();
        setVisible(true);
    }
}
