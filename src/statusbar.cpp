#include "statusbar.h"

StatusBar::StatusBar(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
    setMaximumSize(-1, 1);
}

void StatusBar::cursorPosition(int x, int y) {
    _cursorPositionX = x;
    _cursorPositionY = y;
}

void StatusBar::scrollPosition(int x, int y)
{
    _scrollPositionX = x;
    _scrollPositionY = y;
}

void StatusBar::paintEvent(Tui::ZPaintEvent *event) {

    auto *painter = event->painter();
    painter->clear({0, 0, 0}, {0, 0xaa, 0xaa});

    QString text = "| " + QString::number(_cursorPositionY) +":"+ QString::number(_cursorPositionX) + " | "+
            QString::number(_scrollPositionY) +":"+ QString::number(_scrollPositionX);

    painter->writeWithColors(50, 0, text.toUtf8(), {0, 0, 0}, {0, 0xaa, 0xaa});
}
