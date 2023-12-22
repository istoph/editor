// SPDX-License-Identifier: BSL-1.0

#include "help.h"

#include <QRect>

#include <Tui/ZBasicWindowFacet.h>
#include <Tui/ZPainter.h>
#include <Tui/ZPalette.h>
#include <Tui/ZSymbol.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZWindowLayout.h>

#include "scrollbar.h"

static const QString helpText = R"(The important operations can be invoked from the menu. The menu
can be opened with F10 or with Alt together with the
highlighted letter of the menu item (e.g. Alt + f).

In dialogs Tab is used to navigate between the elements. Use F6
to navigate between windows/dialogs.

Text can be marked in most terminals with Shift + arrow key.
Ctrl + c is used for copying. Paste with Ctrl + v.

Changes can be saved with Ctrl + s and the editor can be
exited with Ctrl + q.
)";

Help::Help(Tui::v0::ZWidget *parent)
    : Tui::ZDialog(parent)
{
    setFocusPolicy(Tui::StrongFocus);
    setDefaultPlacement(Qt::AlignBottom | Qt::AlignHCenter, {0, -2});
    setGeometry({0, 0, 65, 14});
    setPaletteClass({"window", "cyan"});
    setWindowTitle("Quick Start");

    auto winLayout = new Tui::ZWindowLayout();
    setLayout(winLayout);

    auto scrollbar = new ScrollBar(this);
    winLayout->setRightBorderWidget(scrollbar);

    _text = new Tui::ZTextEdit(terminal()->textMetrics(), this);
    winLayout->setCentralWidget(_text);
    _text->setReadOnly(true);
    QObject::connect(_text, &Tui::ZTextEdit::scrollRangeChanged, scrollbar, &ScrollBar::positonMax);
    QObject::connect(_text, &Tui::ZTextEdit::scrollPositionChanged, scrollbar, &ScrollBar::scrollPosition);
    _text->setText(helpText);
    _text->setWordWrapMode(Tui::ZTextOption::WrapMode::WordWrap);
    _text->setFocusPolicy(Tui::NoFocus);
    auto pal = _text->palette();
    // Change colors to look like e.g. a label.
    pal.addRules({
                     {{}, {{Tui::ZPalette::Local, "textedit.bg", "control.bg"}}},
                     {{}, {{Tui::ZPalette::Local, "textedit.fg", "control.fg"}}}
                 });
    _text->setPalette(pal);

    setFocus();
    dynamic_cast<Tui::ZBasicWindowFacet*>(facet(Tui::ZWindowFacet::staticMetaObject))->setExtendViewport(true);
}

void Help::paintEvent(Tui::v0::ZPaintEvent *event) {
    Tui::ZDialog::paintEvent(event);
    auto *painter = event->painter();

    painter->writeWithColors(3, geometry().height() - 1,
                             " Press ESC or Enter to dismiss ",
                             getColor("control.fg"), getColor("control.bg"));
}

void Help::keyEvent(Tui::v0::ZKeyEvent *event) {
    if (event->key() == Tui::Key_Escape || event->key() == Tui::Key_Enter) {
        deleteLater();
    } else if (event->key() == Tui::Key_Up) {
        _text->detachedScrollUp();
    } else if (event->key() == Tui::Key_Down) {
        _text->detachedScrollDown();
    } else {
        Tui::ZDialog::keyEvent(event);
    }
}
