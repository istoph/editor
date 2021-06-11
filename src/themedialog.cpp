#include "themedialog.h"
#include "edit.h"

ThemeDialog::ThemeDialog(Editor *edit) : Dialog(edit) {
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

        _cancelButton = new Button(withMarkup, "<m>C</m>ancel", this);
        hbox->addWidget(_cancelButton);

        _okButton = new Button(withMarkup, "<m>O</m>K", this);
        _okButton->setDefault(true);
        hbox->addWidget(_okButton);
        vbox->add(hbox);
    }
    wl->addCentral(vbox);
    setGeometry({ 6, 5, 30, 8});

    QObject::connect(_cancelButton, &Button::clicked, [=]{
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

    QObject::connect(_okButton, &Button::clicked, [=]{
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
