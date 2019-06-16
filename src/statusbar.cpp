#include "statusbar.h"

StatusBar::StatusBar(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
    setMaximumSize(Tui::tuiMaxSize, 1);
    setSizePolicyH(Tui::SizePolicy::Expanding);
    setSizePolicyV(Tui::SizePolicy::Fixed);
}

QSize StatusBar::sizeHint() const {
    return { 20, 1 };
}

void StatusBar::cursorPosition(int x, int utf8x, int y) {
    _cursorPositionX = x;
    _utf8PositionX = utf8x;
    _cursorPositionY = y;
}

void StatusBar::scrollPosition(int x, int y) {
    _scrollPositionX = x;
    _scrollPositionY = y;
}

void StatusBar::setModified(bool save) {
    this->modified = save;
}

void StatusBar::paintEvent(Tui::ZPaintEvent *event) {

    auto *painter = event->painter();
    painter->clear({0, 0, 0}, {0, 0xaa, 0xaa});

    QString text;
    if(this->modified) {
        text += "";
    }
    text += " | UTF-8 " + QString::number(_cursorPositionY +1) + ":" + QString::number(_utf8PositionX +1);
    if (_utf8PositionX != _cursorPositionX) {
        text += "-" + QString::number(_cursorPositionX +1);
    }
    text += " | " + QString::number(_scrollPositionY +1)  + ":" + QString::number(_scrollPositionX +1);

    painter->writeWithColors(50, 0, text.toUtf8(), {0, 0, 0}, {0, 0xaa, 0xaa});
}
