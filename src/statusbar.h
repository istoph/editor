#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <testtui_lib.h>

class StatusBar : public Tui::ZWidget {
    Q_OBJECT
public:
    StatusBar(Tui::ZWidget *parent);

public:
    QSize sizeHint() const override;

public slots:
    void cursorPosition(int x, int utf8x, int y);
    void scrollPosition(int x, int y);
    void setModified(bool modified);
    void readFromStandardInput(bool activ);
    void followStandardInput(bool follow);
    void setWritable(bool rw);
    void msdosMode(bool msdos);

protected:
    void paintEvent(Tui::ZPaintEvent *event);
private:
    bool modified = false;
    int _cursorPositionX = 0;
    int _utf8PositionX = 0;
    int _cursorPositionY = 0;
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
    bool _stdin = false;
    bool _stdin_follow = false;
    bool _readwrite = true;
    bool _msdosMode = false;
};

#endif // STATUSBAR_H
