#ifndef COMMANDLINEWIDGET_H
#define COMMANDLINEWIDGET_H

#include <QObject>

#include <testtui_lib.h>

class CommandLineWidget : public Tui::ZWidget {
    Q_OBJECT
public:
    CommandLineWidget(Tui::ZWidget *parent);

public:
    QSize sizeHint() const override;

signals:
    void dismissed();
    void execute(QString command);

protected:
    void paintEvent(Tui::ZPaintEvent *event);
    void keyEvent(Tui::ZKeyEvent *event) override;
    void pasteEvent(Tui::ZPasteEvent *event) override;
    void resizeEvent(Tui::ZResizeEvent *event) override;

private:
    InputBox cmdEntry;
};

#endif
