// SPDX-License-Identifier: BSL-1.0

#include "commandlinewidget.h"

#include <QRect>
#include <QSize>

#include <Tui/ZColor.h>
#include <Tui/ZPainter.h>
#include <Tui/ZPalette.h>

CommandLineWidget::CommandLineWidget(Tui::v0::ZWidget *parent) : Tui::ZWidget(parent) {
    setMaximumSize(Tui::tuiMaxSize, 1);
    setSizePolicyH(Tui::SizePolicy::Expanding);
    setSizePolicyV(Tui::SizePolicy::Fixed);

    cmdEntry.setParent(this);
    auto pal = cmdEntry.palette();
    pal.setColors({
        { "lineedit.focused.bg", Tui::Colors::black},
        { "lineedit.focused.fg", Tui::Colors::lightGray},
        { "lineedit.bg", Tui::Colors::black},
        { "lineedit.fg", Tui::Colors::lightGray}});
    cmdEntry.setPalette(pal);
}

void CommandLineWidget::setCmdEntryText(QString text) {
    cmdEntry.setText(text);
}

QSize CommandLineWidget::sizeHint() const {
    return { 0, 1 };
}

void CommandLineWidget::paintEvent(Tui::ZPaintEvent *event) {
    Tui::ZColor bg = Tui::Colors::black;
    Tui::ZColor fg = Tui::Colors::lightGray;

    auto *painter = event->painter();
    painter->clear(fg, bg);
    painter->writeWithColors(0, 0, "> ", fg, bg);
}

void CommandLineWidget::keyEvent(Tui::ZKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        releaseKeyboard();
        cmdEntry.setText("");
        dismissed();
    } else if (event->key() == Qt::Key_Enter && event->modifiers() == 0) {
        releaseKeyboard();
        QString cmd = cmdEntry.text();
        cmdEntry.setText("");
        execute(cmd);
    } else {
        cmdEntry.event(event);
    }
}

void CommandLineWidget::pasteEvent(Tui::ZPasteEvent *event) {
    cmdEntry.event(event);
}

void CommandLineWidget::resizeEvent(Tui::ZResizeEvent *event) {
    cmdEntry.setGeometry({2, 0, event->size().width() - 2, 1});
    ZWidget::resizeEvent(event);
}
