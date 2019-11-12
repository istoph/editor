#include "searchdialog.h"

SearchDialog::SearchDialog(Tui::ZWidget *parent, File *file) : Dialog(parent) {
    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    WindowLayout *wl = new WindowLayout();
    setLayout(wl);

    setWindowTitle("Search");

    searchText = new InputBox(this);
    VBoxLayout *vbox = new VBoxLayout();
    vbox->setSpacing(1);
    {
        HBoxLayout* hbox = new HBoxLayout();

        hbox->setSpacing(2);
        Label *l = new Label(withMarkup, "F<m>i</m>nd", this);
        hbox->addWidget(l);
        l->setBuddy(searchText);
        hbox->addWidget(searchText);
        vbox->add(hbox);
    }

    {
        HBoxLayout* hbox = new HBoxLayout();
        hbox->setSpacing(3);

        {
            GroupBox *gbox = new GroupBox("Options", this);
            VBoxLayout *nbox = new VBoxLayout();
            gbox->setLayout(nbox);

            CheckBox *caseMatch = new CheckBox(withMarkup, "<m>M</m>atch case", gbox);
            nbox->addWidget(caseMatch);
            CheckBox *wordMatch = new CheckBox(withMarkup, "Match <m>e</m>ntire word only", gbox);
            nbox->addWidget(wordMatch);
            CheckBox *regexMatch = new CheckBox(withMarkup, "Re<m>g</m>ular expression", gbox);
            nbox->addWidget(regexMatch);

            hbox->addWidget(gbox);
        }

        {
            GroupBox *gbox = new GroupBox("Direction", this);
            VBoxLayout *nbox = new VBoxLayout();
            gbox->setLayout(nbox);

            RadioButton *forward = new RadioButton(withMarkup, "<m>F</m>orward", gbox);
            forward->setChecked(true);
            nbox->addWidget(forward);
            RadioButton *backward = new RadioButton(withMarkup, "<m>B</m>ackward", gbox);
            nbox->addWidget(backward);

            nbox->addSpacing(1);

            CheckBox *wrap = new CheckBox(withMarkup, "Wra<m>p</m> around", gbox);
            nbox->addWidget(wrap);

            hbox->addWidget(gbox);
        }

        vbox->add(hbox);
    }

    vbox->addStretch();

    {
        HBoxLayout* hbox = new HBoxLayout();
        hbox->setSpacing(1);
        hbox->addStretch();
        okBtn = new Button(withMarkup, "<m>F</m>ind", this);
        okBtn->setDefault(true);
        okBtn->setEnabled(false);
        hbox->addWidget(okBtn);

        Button *cancelBtn = new Button(withMarkup, "<m>C</m>lose", this);
        hbox->addWidget(cancelBtn);

        hbox->addSpacing(3);

        vbox->add(hbox);
    }

    wl->addCentral(vbox);

    setGeometry({ 12, 5, 55, 13});

    connect(searchText, &InputBox::textChanged, this, [this] (const QString& newText) {
        okBtn->setEnabled(newText.size());
    });
}

void SearchDialog::open()
{
    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    setGeometry(r);
    setVisible(true);
    placeFocus()->setFocus();
}
