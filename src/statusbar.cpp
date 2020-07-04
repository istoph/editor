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

void StatusBar::readFromStandardInput(bool activ) {
    _stdin = activ;
}
void StatusBar::followStandardInput(bool follow) {
    _stdin_follow = follow;
}

void StatusBar::setWritable(bool rw) {
    _readwrite = rw;
}

void StatusBar::msdosMode(bool msdos) {
    _msdosMode = msdos;
}

void StatusBar::paintEvent(Tui::ZPaintEvent *event) {
    Tui::ZColor bg = {0, 0xaa, 0xaa};
    auto *painter = event->painter();


    QString text;
    if(this->modified) {
        text += " ";
    }
    if(_stdin) {
        text += " | STDIN";
        if (_stdin_follow) {
            //TODO: Blinking
            text += " FOLOW";
        }
    } else {
        if(_readwrite) {
            text += " | RW";
        } else {
            text += " | RO";
            bg = {0xff,0,0};
        }
    }
    text += " | UTF-8 ";
    if (_msdosMode) {
        text += "DOS ";
    }
    text += QString::number(_cursorPositionY +1) + ":" + QString::number(_utf8PositionX +1);
    if (_utf8PositionX != _cursorPositionX) {
        text += "-" + QString::number(_cursorPositionX +1);
    }
    text += " | " + QString::number(_scrollPositionY +1)  + ":" + QString::number(_scrollPositionX +1);

    painter->clear({0, 0, 0}, bg);
    painter->writeWithColors(80 - text.size() -2, 0, text.toUtf8(), {0, 0, 0}, bg);
}
