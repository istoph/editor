#ifndef ALERT_H
#define ALERT_H

#include <QObject>

#include <Tui/ZColor.h>
#include <Tui/ZTextStyle.h>
#include <Tui/ZStyledTextLine.h>
#include <Tui/ZPalette.h>
#include <Tui/ZWindow.h>

class Alert : public Tui::ZWindow {
    Q_OBJECT

public:
    explicit Alert(Tui::ZWidget *parent);

public:
    QString markup() const;
    void setMarkup(QString m);

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;

private:
    Tui::ZStyledTextLine _styledText;
};

#endif // ALERT_H
