// SPDX-License-Identifier: BSL-1.0

#include "groupbox.h"

#include <QSize>

#include <Tui/ZLayout.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZSymbol.h>
#include <Tui/ZTerminal.h>


GroupBox::GroupBox(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
}

GroupBox::GroupBox(const QString &text, Tui::ZWidget *parent) : GroupBox(parent) {
    setText(text);
}

GroupBox::GroupBox(Tui::WithMarkupTag, const QString &markup, Tui::ZWidget *parent) : GroupBox(parent) {
    setMarkup(markup);
}

QString GroupBox::text() const {
    return styledText.text();
}

void GroupBox::setText(const QString &t) {
    styledText.setText(t);
    update();
}

QString GroupBox::markup() const {
    return styledText.markup();
}

void GroupBox::setMarkup(QString m) {
    styledText.setMarkup(m);
    if (styledText.mnemonic().size()) {
        for(Tui::ZShortcut *s : findChildren<Tui::ZShortcut*>("", Qt::FindDirectChildrenOnly)) {
            delete s;
        }
        Tui::ZShortcut *s = new Tui::ZShortcut(Tui::ZKeySequence::forMnemonic(styledText.mnemonic()), this);
        connect(s, &Tui::ZShortcut::activated, this, [this] {
            Tui::ZWidget *w = placeFocus();
            if (w) {
                w->setFocus(Qt::ShortcutFocusReason);
            }
        });
    }
    update();
}

QSize GroupBox::sizeHint() const {
    if (layout()) {
        QSize sh = layout()->sizeHint() + QSize(1, 1);
        QMargins cm = contentsMargins();
        sh.rwidth() += cm.left() + cm.right();
        sh.rheight() += cm.top() + cm.bottom();
        sh.rwidth() = std::max(sh.rwidth(), text().size() + 1);
        return sh.expandedTo(minimumSize()).boundedTo(maximumSize());
    }
    return {text().size() + 1, 1};
}

QRect GroupBox::layoutArea() const {
    QRect r = Tui::ZWidget::layoutArea();
    r.adjust(1, 1, 0, 0);
    return r;
}

void GroupBox::paintEvent(Tui::ZPaintEvent *event) {
    Tui::ZTextStyle baseStyle;
    Tui::ZTextStyle shortcut;

    auto *painter = event->painter();

    auto *term = terminal();
    if (!isEnabled()) {
        baseStyle = {getColor("control.disabled.fg"), getColor("control.bg")};
        shortcut = baseStyle;
    } else {
        if (term && isAncestorOf(term->focusWidget())) {
            baseStyle = {getColor("control.focused.fg"), getColor("control.focused.bg")};
            painter->writeWithColors(0, 0, "»", baseStyle.foregroundColor(), baseStyle.backgroundColor());
        } else {
            baseStyle = {getColor("control.fg"), getColor("control.bg")};
        }
        shortcut = {getColor("control.shortcut.fg"), getColor("control.shortcut.bg")};
    }
    styledText.setMnemonicStyle(baseStyle, shortcut);
    styledText.write(painter, 1, 0, geometry().width()-1);
}

void GroupBox::keyEvent(Tui::ZKeyEvent *event) {
    Tui::ZWidget::keyEvent(event);
}
