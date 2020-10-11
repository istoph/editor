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
    update();
}

void StatusBar::scrollPosition(int x, int y) {
    _scrollPositionX = x;
    _scrollPositionY = y;
    update();
}

void StatusBar::setModified(bool mf) {
    _modifiedFile = mf;
    update();
}

void StatusBar::readFromStandardInput(bool activ) {
    _stdin = activ;
    update();
}

void StatusBar::followStandardInput(bool follow) {
    _follow = follow;
    update();
}

void StatusBar::setWritable(bool rw) {
    _readwrite = rw;
    update();
}

void StatusBar::searchCount(int sc) {
    _searchCount = sc;
    update();
}

void StatusBar::searchText(QString searchText) {
    _searchText = searchText;
    update();
}

void StatusBar::msdosMode(bool msdos) {
    _msdosMode = msdos;
    update();
}

void StatusBar::modifiedSelectMode(bool event) {
    _selectMode = event;
    update();
}

void StatusBar::fileHasBeenChanged(bool fileChanged) {
    _fileChanged = fileChanged;
    update();
}

void StatusBar::paintEvent(Tui::ZPaintEvent *event) {
    Tui::ZColor bg = {0, 0xaa, 0xaa};
    auto *painter = event->painter();

    QString search;
    search = _searchText +": "+ QString::number(_searchCount);

    QString text;
    if(_fileChanged) {
        text += " FILE CHANGED";
        bg = {0xFF,0xDD,0};
    }
    if(_selectMode) {
        text += " SELECT MODE";
    }
    if(_modifiedFile) {
        text += " ";
    }
    if(_stdin) {
        text += " | STDIN";
        if (_follow) {
            text += " FOLLOW";
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
    if(_searchText != "") {
        painter->writeWithColors(0, 0, search.toUtf8(), {0, 0, 0}, {0xff,0xdd,00});
    }
}
