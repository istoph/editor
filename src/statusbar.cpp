// SPDX-License-Identifier: BSL-1.0

#include "statusbar.h"

#include <QSize>

#include <Tui/ZPainter.h>
#include <Tui/ZSymbol.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTextMetrics.h>

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
QString StatusBar::viewCursorPosition() {
    QString text;
    text += QString::number(_cursorPositionY +1) + ":" + QString::number(_utf8PositionX +1);
    if (_utf8PositionX != _cursorPositionX) {
        text += "-" + QString::number(_cursorPositionX +1);
    }
    text += " | " + QString::number(_scrollPositionY +1)  + ":" + QString::number(_scrollPositionX +1);
    return text;
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
QString StatusBar::viewModifiedFile() {
    QString text;
    if(_modifiedFile) {
        text += "🖫";
    }
    return text;
}

void StatusBar::readFromStandardInput(bool activ) {
    _stdin = activ;
    update();
}

void StatusBar::followStandardInput(bool follow) {
    _follow = follow;
    update();
}

QString StatusBar::viewStandardInput() {
    QString text;
    if(_stdin) {
        text += "STDIN";
        if (_follow) {
            text += " FOLLOW";
        }
    }
    return text;
}

void StatusBar::setWritable(bool rw) {
    _readwrite = rw;
    update();
}

QString StatusBar::viewReadWriete() {
    QString text;
    if(_readwrite) {
        text += "RW";
    } else {
        text += "RO";
        _bg = {0xff,0,0};
    }
    return text;
}

void StatusBar::searchCount(int sc) {
    _searchCount = sc;
    update();
}

void StatusBar::searchText(QString searchText) {
    _searchText = searchText;
    update();
}

void StatusBar::crlfMode(bool crlf) {
    _crlfMode = crlf;
    update();
}

QString StatusBar::viewMode() {
    QString text;
    text += "UTF-8";
    if (_crlfMode) {
        text += " CRLF";
    }
    return text;
}

void StatusBar::modifiedSelectMode(bool event) {
    _selectMode = event;
    update();
}
QString StatusBar::viewSelectMode() {
    QString text;
    if(_selectMode) {
        text += "SELECT MODE (F4)";
    }
    return text;
}

void StatusBar::fileHasBeenChangedExternally(bool fileChanged) {
    _fileChanged = fileChanged;
    update();
}
QString StatusBar::viewFileChanged() {
    QString text;
    if(_fileChanged) {
        text += "FILE CHANGED";
        _bg = {0xFF,0xDD,0};
    }
    return text;
}

void StatusBar::overwrite(bool overwrite) {
    _overwrite = overwrite;
    update();
}

QString StatusBar::viewOverwrite() {
    QString text;
    if (_overwrite) {
        text += "OVR";
    } else {
        text += "INS";
    }
    return text;
}


QString StatusBar::viewLanguage() {
    if (!_syntaxHighlightingEnabled || _language == "None") {
        return "";
    } else {
        return _language;
    }
}

void StatusBar::language(QString language) {
    _language = language;
    update();
}

void StatusBar::syntaxHighlightingEnabled(bool enable) {
    _syntaxHighlightingEnabled = enable;
    update();
}


QString slash(QString text) {
    if (text != "") {
        return " | "+ text;
    }
    return text;
}

void StatusBar::paintEvent(Tui::ZPaintEvent *event) {
    _bg = getColor("chr.statusbarBg");
    auto *painter = event->painter();

    QString search;
    int cutColums = terminal()->textMetrics().splitByColumns(_searchText, 25).codeUnits;
    search = _searchText.left(cutColums) +": "+ QString::number(_searchCount);

    QString text;
    text += slash(viewLanguage());
    text += slash(viewFileChanged());
    text += slash(viewSelectMode());
    text += slash(viewModifiedFile());

    if(_stdin) {
        text += slash(viewStandardInput());
    } else {
        text += slash(viewReadWriete());
    }

    text += slash(viewOverwrite());
    text += slash(viewMode());
    text += slash(viewCursorPosition());

    painter->clear({0, 0, 0}, _bg);
    painter->writeWithColors(terminal()->width() - text.size() -2, 0, text.toUtf8(), {0, 0, 0}, _bg);
    if(_searchText != "") {
        painter->writeWithColors(0, 0, search.toUtf8(), {0, 0, 0}, {0xff,0xdd,00});
    }
}
