// SPDX-License-Identifier: BSL-1.0

#include "alert.h"

#include <Tui/ZSymbol.h>


Alert::Alert(Tui::v0::ZWidget *parent)
    : Tui::ZWindow(parent)
{

}

QString Alert::markup() const {
    return _styledText.markup();
}

void Alert::setMarkup(QString m) {
    _styledText.setMarkup(m);
}

void Alert::paintEvent(Tui::v0::ZPaintEvent *event) {
    Tui::ZWindow::paintEvent(event);
    _styledText.setBaseStyle({getColor("control.fg"), getColor("control.bg")});
    //_styledText.setWidth(geometry().width() - 4);
    auto *painter = event->painter();
    //_styledText.write(painter, 2, 2, geometry().width() - 4, geometry().height() - 4);
    _styledText.write(painter, 2, 2, 47);
}

void Alert::keyEvent(Tui::v0::ZKeyEvent *event) {
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Enter) {
        deleteLater();
    } else {
        Tui::ZWidget::keyEvent(event);
    }
}
