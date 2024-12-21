// SPDX-License-Identifier: BSL-1.0

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QDateTime>

#include <Tui/ZColor.h>
#include <Tui/ZWidget.h>

class GlobalKeyPressListener;

class StatusBar : public Tui::ZWidget {
    Q_OBJECT
public:
    StatusBar(Tui::ZWidget *parent);
    QString viewCursorPosition();
    QString viewFileChanged();
    QString viewMode();
    QString viewModifiedFile();
    QString viewOverwrite();
    QString viewReadWrite();
    QString viewSelectMode();
    QString viewSelectCharsLines();
    QString viewStandardInput();
    QString viewLanguage();
    void switchToNormalDisplay();

public:
    QSize sizeHint() const override;

public slots:
    void cursorPosition(int x, int utf16CodeUnit, int utf8CodeUnit, int line);
    void scrollPosition(int x, int y);
    void setModified(bool modifiedFile);
    void readFromStandardInput(bool activ);
    void followStandardInput(bool follow);
    void setWritable(bool rw);
    void searchCount(int sc);
    void searchText(QString searchText);
    void searchVisible(bool visible);
    void crlfMode(bool msdos);
    void modifiedSelectMode(bool f4);
    void setSelectCharLines(int selectChar, int selectLines);
    void fileHasBeenChangedExternally(bool fileChanged = true);
    void overwrite(bool overwrite);
    void syntaxHighlightingEnabled(bool enable);
    void language(QString language);

public:
    static void notifyQtLog();

protected:
    void paintEvent(Tui::ZPaintEvent *event);

private:
    bool _showHelp = true;
    QDateTime _helpHoldOff = QDateTime::currentDateTimeUtc().addMSecs(100);
    GlobalKeyPressListener *_keyPressListener = nullptr;

    bool _modifiedFile = false;
    int _cursorPositionX = 0;
    int _utf8PositionX = 0;
    int _cursorPositionY = 0;
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
    bool _stdin = false;
    bool _follow = false;
    bool _readwrite = true;
    int _searchCount = -1;
    QString _searchText = "";
    bool _searchVisible = false;
    bool _crlfMode = false;
    bool _selectMode = false;
    int _selectLines = 0;
    int _selectChars = 0;
    bool _fileChanged = false;
    bool _overwrite = false;
    QString _language = "None";
    bool _syntaxHighlightingEnabled = false;
    Tui::ZColor _bg;

    static bool _qtMessage;
};

#endif // STATUSBAR_H
