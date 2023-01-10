#ifndef GROUPBOX_H
#define GROUPBOX_H

#include <Tui/ZColor.h>
#include <Tui/ZLayout.h>
#include <Tui/ZPainter.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZStyledTextLine.h>
#include <Tui/ZSymbol.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZWidget.h>

#include <QSize>
#include <QRect>
#include <QObject>

class GroupBox : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit GroupBox(Tui::ZWidget *parent = nullptr);
    explicit GroupBox(const QString &text, Tui::ZWidget *parent = nullptr);
    explicit GroupBox(Tui::WithMarkupTag, const QString &markup, Tui::ZWidget *parent = nullptr);

public:
    QString text() const;
    void setText(const QString &t);

    QString markup() const;
    void setMarkup(QString m);

    QSize sizeHint() const override;
    QRect layoutArea() const override;

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;

private:
    Tui::ZStyledTextLine styledText;
};

#endif // GROUPBOX_H
