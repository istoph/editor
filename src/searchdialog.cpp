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

            caseMatch = new CheckBox(withMarkup, "<m>M</m>atch case", gbox);
            nbox->addWidget(caseMatch);
            CheckBox *wordMatch = new CheckBox(withMarkup, "Match <m>e</m>ntire word only", gbox);
            nbox->addWidget(wordMatch);
            wordMatch->setEnabled(false);
            CheckBox *regexMatch = new CheckBox(withMarkup, "Re<m>g</m>ular expression", gbox);
            nbox->addWidget(regexMatch);
            regexMatch->setEnabled(false);

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

            _wrap = new CheckBox(withMarkup, "Wra<m>p</m> around", gbox);
            _wrap->setCheckState(Qt::Checked);
            nbox->addWidget(_wrap);

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

        cancelBtn = new Button(withMarkup, "<m>C</m>lose", this);
        hbox->addWidget(cancelBtn);

        hbox->addSpacing(3);

        vbox->add(hbox);
    }

    wl->addCentral(vbox);

    setGeometry({ 12, 5, 55, 13});

    QObject::connect(searchText, &InputBox::textChanged, this, [this,file] (const QString& newText) {
        okBtn->setEnabled(newText.size());
        file->setSearchText(searchText->text());
    });

    QObject::connect(okBtn, &Button::clicked, [=]{
        if (_wrap->checkState() == Qt::Checked) {
            file->setSearchWrap(true);
        } else {
            file->setSearchWrap(false);
        }

        _searchText = searchText->text();
        file->setSearchText(_searchText);
        file->searchNext();
        setVisible(false);
        update();
    });

    QObject::connect(cancelBtn, &Button::clicked, [=]{
        _searchText = "";
        file->setSearchText(_searchText);
        setVisible(false);
    });

    QObject::connect(caseMatch, &CheckBox::stateChanged, [=]{
        caseSensitive = !caseSensitive;
        if(caseSensitive) {
            file->searchCaseSensitivity = Qt::CaseInsensitive;
        } else {
            file->searchCaseSensitivity = Qt::CaseSensitive;
        }
        update();
    });
}

void SearchDialog::setSearchText(QString text) {
    searchText->setText(text);
    searchText->textChanged(text);
}

void SearchDialog::open() {
    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    r.moveBottom(terminal()->mainWidget()->geometry().height()-3);
    setGeometry(r);
    setVisible(true);
    placeFocus()->setFocus();
}
