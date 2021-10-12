#include "themedialog.h"
#include "edit.h"

ThemeDialog::ThemeDialog(Editor *edit) : Dialog(edit) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose | Tui::ZWindow::MoveOption);
    setDefaultPlacement(Qt::AlignBottom | Qt::AlignHCenter, {0, -2});
    WindowLayout *wl = new WindowLayout();
    this->setWindowTitle("Theme Color");
    setLayout(wl);
    setContentsMargins({ 1, 1, 1, 1});

    VBoxLayout *vbox = new VBoxLayout();
    {
        HBoxLayout* hbox = new HBoxLayout();
        _lv = new ListView(this);
        QStringList qsl = {"Classic", "Dark"};
        _lv->setItems(qsl);
        _lv->setFocus();
        hbox->addWidget(_lv);
        vbox->add(hbox);
    }

    vbox->setSpacing(1);
    {
        HBoxLayout* hbox = new HBoxLayout();
        hbox->addStretch();

        _cancelButton = new Tui::ZButton(Tui::withMarkup, "<m>C</m>ancel", this);
        hbox->addWidget(_cancelButton);

        _okButton = new Tui::ZButton(Tui::withMarkup, "<m>O</m>K", this);
        _okButton->setDefault(true);
        hbox->addWidget(_okButton);
        vbox->add(hbox);
    }
    wl->addCentral(vbox);
    setGeometry({ 6, 5, 30, 8});

    QObject::connect(_cancelButton, &Tui::ZButton::clicked, [=]{
        close();
    });

    QObject::connect(_lv, &ListView::enterPressed, [=]{
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
