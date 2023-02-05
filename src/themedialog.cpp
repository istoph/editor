// SPDX-License-Identifier: BSL-1.0

#include "themedialog.h"

#include <Tui/ZHBoxLayout.h>
#include <Tui/ZVBoxLayout.h>

#include "edit.h"


ThemeDialog::ThemeDialog(Editor *edit) : Tui::ZDialog(edit) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose
               | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setDefaultPlacement(Qt::AlignBottom | Qt::AlignHCenter, {0, -2});
    Tui::ZWindowLayout *wl = new Tui::ZWindowLayout();
    this->setWindowTitle("Theme Color");
    setLayout(wl);
    setContentsMargins({ 1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    {
        Tui::ZHBoxLayout* hbox = new Tui::ZHBoxLayout();
        _lv = new Tui::ZListView(this);
        QStringList qsl = {"Classic", "Dark"};
        _lv->setItems(qsl);
        _lv->setFocus();
        hbox->addWidget(_lv);
        vbox->add(hbox);
    }

    vbox->setSpacing(1);
    {
        Tui::ZHBoxLayout* hbox = new Tui::ZHBoxLayout();
        hbox->addStretch();

        _cancelButton = new Tui::ZButton(Tui::withMarkup, "<m>C</m>ancel", this);
        hbox->addWidget(_cancelButton);

        _okButton = new Tui::ZButton(Tui::withMarkup, "<m>O</m>K", this);
        _okButton->setDefault(true);
        hbox->addWidget(_okButton);
        vbox->add(hbox);
    }
    wl->setCentral(vbox);
    setGeometry({ 6, 5, 30, 8});

    QObject::connect(_cancelButton, &Tui::ZButton::clicked, [=]{
        close();
    });

    QObject::connect(_lv, &Tui::ZListView::enterPressed, [=]{
        if(_lv->currentItem() == "Dark") {
            edit->setTheme(Editor::Theme::dark);
        } else {
            edit->setTheme(Editor::Theme::classic);
        }
        close();
    });

    QObject::connect(_okButton, &Tui::ZButton::clicked, [=]{
        if(_lv->currentItem() == "Dark") {
            edit->setTheme(Editor::Theme::dark);
        } else {
            edit->setTheme(Editor::Theme::classic);
        }
        close();
    });
    open();
}

void ThemeDialog::close() {
    deleteLater();
}

void ThemeDialog::open() {
    raise();
}
