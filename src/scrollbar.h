#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <testtui_lib.h>

class ScrollBar : public Tui::ZWidget {
    Q_OBJECT
public:
    ScrollBar(Tui::ZWidget *parent);

public slots:
    void cursorPosition(int x, int y);
    void scrollPosition(int x, int y);
    void textMax(int x, int y);
    void displayArea(int x, int y);

protected:
    void paintEvent(Tui::ZPaintEvent *event);

private:
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
    int _textMaxX = 0;
    int _textMaxY = 0;
    int _displayAreaWidth = 0;
    int _displayAreaHeight = 0;

};

#endif // SCROLLBAR_H
