// SPDX-License-Identifier: BSL-1.0

#ifndef ALERT_H
#define ALERT_H

#include <QObject>

#include <Tui/ZStyledTextLine.h>
#include <Tui/ZDialog.h>


class Alert : public Tui::ZDialog {
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
