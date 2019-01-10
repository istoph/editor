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
    void cursorPosition(int x, int y);
    void scrollPosition(int x, int y);
    void setModified(bool modified);

protected:
    void paintEvent(Tui::ZPaintEvent *event);
private:
    bool modified = false;
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
};

#endif // STATUSBAR_H
