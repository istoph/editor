// SPDX-License-Identifier: BSL-1.0

#include "statusbar.h"

#include <QSize>

#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>
#include <Tui/ZSymbol.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTextLayout.h>
#include <Tui/ZTextMetrics.h>

bool StatusBar::_qtMessage = false;

class GlobalKeyPressListener : public QObject {
public:
    GlobalKeyPressListener(StatusBar *statusbar) : statusbar(statusbar) {
    }

    StatusBar *statusbar = nullptr;

    bool eventFilter(QObject *watched, QEvent *event) {
        (void)watched;
        if (event->type() == Tui::ZEventType::rawSequence()) {
            statusbar->switchToNormalDisplay();
        }
        return false;
    }
};


StatusBar::StatusBar(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
    setMaximumSize(Tui::tuiMaxSize, 1);
    setSizePolicyH(Tui::SizePolicy::Expanding);
    setSizePolicyV(Tui::SizePolicy::Fixed);

    _keyPressListener = new GlobalKeyPressListener(this);
    terminal()->installEventFilter(_keyPressListener);
}

QSize StatusBar::sizeHint() const {
    return { 20, 1 };
}

void StatusBar::cursorPosition(int x, int utf16CodeUnit, int utf8CodeUnit, int line) {
    (void)utf16CodeUnit;
    _cursorPositionX = x;
    _utf8PositionX = utf8CodeUnit;
    _cursorPositionY = line;
    update();
    switchToNormalDisplay();
}

QString StatusBar::viewCursorPosition() {
    QString text;
    text += QString::number(_cursorPositionY + 1) + ":" + QString::number(_utf8PositionX + 1);
    if (_utf8PositionX != _cursorPositionX) {
        text += "-" + QString::number(_cursorPositionX + 1);
    }
    text += " | " + QString::number(_scrollPositionY + 1)  + ":" + QString::number(_scrollPositionX + 1);
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
    if (_modifiedFile) {
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
    if (_stdin) {
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

QString StatusBar::viewReadWrite() {
    QString text;
    if (_readwrite) {
        text += "RW";
    } else {
        text += "RO";
        _bg = {0xff, 0, 0};
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

void StatusBar::searchVisible(bool visible) {
    _searchVisible = visible;
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
    if (_selectMode) {
        text += "SELECT MODE (F4)";
    }
    return text;
}

void StatusBar::setSelectCharLines(int selectChar, int selectLines) {
    _selectChars = selectChar;
    _selectLines = selectLines;
}

QString StatusBar::viewSelectCharsLines() {
    if (_selectChars > 0 || _selectLines > 0) {
        return QString::number(_selectChars) +"C "+ QString::number(_selectLines) +"L";
    }
    return "";
}

void StatusBar::fileHasBeenChangedExternally(bool fileChanged) {
    _fileChanged = fileChanged;
    update();
}

QString StatusBar::viewFileChanged() {
    QString text;
    if (_fileChanged) {
        text += "FILE CHANGED";
        _bg = {0xFF, 0xDD, 0};
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

void StatusBar::switchToNormalDisplay() {
    if (_showHelp) {
        if (_helpHoldOff < QDateTime::currentDateTimeUtc()) {
            _showHelp = false;
            update();
            _keyPressListener->deleteLater();
        }
    }
}

void StatusBar::language(QString language) {
    _language = language;
    update();
}

void StatusBar::notifyQtLog() {
    _qtMessage = true;
}

void StatusBar::syntaxHighlightingEnabled(bool enable) {
    _syntaxHighlightingEnabled = enable;
    update();
}


QString slash(QString text) {
    if (text != "") {
        return " | " + text;
    }
    return text;
}

static const QChar escapedNewLine = (QChar)(0xdc00 + (unsigned char)'\n');
static const QChar escapedTab = (QChar)(0xdc00 + (unsigned char)'\t');

void StatusBar::paintEvent(Tui::ZPaintEvent *event) {
    _bg = getColor("chr.statusbarBg");
    auto *painter = event->painter();

    QString search;
    int cutColums = terminal()->textMetrics().splitByColumns(_searchText, 25).codeUnits;
    search = _searchText.left(cutColums).replace(u'\n', escapedNewLine).replace(u'\t', escapedTab)
            + ": "+ QString::number(_searchCount);

    QString text;
    text += slash(viewSelectCharsLines());
    text += slash(viewLanguage());
    text += slash(viewFileChanged());
    text += slash(viewSelectMode());
    text += slash(viewModifiedFile());

    if (_stdin) {
        text += slash(viewStandardInput());
    } else {
        text += slash(viewReadWrite());
    }

    text += slash(viewOverwrite());
    text += slash(viewMode());
    text += slash(viewCursorPosition());

    painter->clear({0, 0, 0}, _bg);
    painter->writeWithColors(terminal()->width() - text.size() - 2, 0, text.toUtf8(), {0, 0, 0}, _bg);

    if (_searchVisible && _searchText != "" && _searchCount != -1) {
        Tui::ZTextLayout searchLayout(terminal()->textMetrics(), search);
        searchLayout.doLayout(25);
        searchLayout.draw(*painter, {0, 0}, Tui::ZTextStyle({0, 0, 0}, {0xff,0xdd,00}));
    }

    if (_showHelp) {
        QString text = "F1: Help, F10/Alt+O: Menu, Ctrl+Q: quit";
        painter->writeWithColors(0, 0, text.toUtf8(), {0, 0, 0}, _bg);
    }

    if (_qtMessage) {
        painter->writeWithAttributes(terminal()->width() - 2, 0, "!!", _bg, {0xff, 0, 0},
                                     Tui::ZTextAttribute::Bold | Tui::ZTextAttribute::Blink);
    }
}
