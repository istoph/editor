#include "searchdialog.h"

SearchDialog::SearchDialog(Tui::ZWidget *parent, File *file) : Dialog(parent) {
    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    WindowLayout *wl = new WindowLayout();
    setLayout(wl);

    setWindowTitle("Search");

    _searchText = new InputBox(this);
    VBoxLayout *vbox = new VBoxLayout();
    vbox->setSpacing(1);
    {
        HBoxLayout* hbox = new HBoxLayout();

        hbox->setSpacing(2);
        Label *l = new Label(withMarkup, "F<m>i</m>nd", this);
        hbox->addWidget(l);
        l->setBuddy(_searchText);
        hbox->addWidget(_searchText);
        vbox->add(hbox);
    }

    {
        HBoxLayout* hbox = new HBoxLayout();
        hbox->setSpacing(3);

        {
            GroupBox *gbox = new GroupBox("Options", this);
            VBoxLayout *nbox = new VBoxLayout();
            gbox->setLayout(nbox);

            _caseMatchBox = new CheckBox(withMarkup, "<m>M</m>atch case", gbox);
            nbox->addWidget(_caseMatchBox);
            CheckBox *wordMatchBox = new CheckBox(withMarkup, "Match <m>e</m>ntire word only", gbox);
            nbox->addWidget(wordMatchBox);
            wordMatchBox->setEnabled(false);
            CheckBox *regexMatchBox = new CheckBox(withMarkup, "Re<m>g</m>ular expression", gbox);
            nbox->addWidget(regexMatchBox);
            regexMatchBox->setEnabled(false);
            _liveSearchBox = new CheckBox(withMarkup, "<m>L</m>ive search", gbox);
            _liveSearchBox->setCheckState(Qt::Checked);
            nbox->addWidget(_liveSearchBox);

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

            _wrapBox = new CheckBox(withMarkup, "Wra<m>p</m> around", gbox);
            if(file->getSearchWrap()) {
                _wrapBox->setCheckState(Qt::Checked);
            } else {
                _wrapBox->setCheckState(Qt::Unchecked);
            }
            nbox->addWidget(_wrapBox);

            hbox->addWidget(gbox);
        }

        vbox->add(hbox);
    }

    vbox->addStretch();

    {
        HBoxLayout* hbox = new HBoxLayout();
        hbox->setSpacing(1);
        hbox->addStretch();

        _findNextBtn = new Button(withMarkup, "<m>N</m>ext", this);
        _findNextBtn->setDefault(true);
        hbox->addWidget(_findNextBtn);

        _findPreviousBtn = new Button(withMarkup, "<m>P</m>revious", this);
        hbox->addWidget(_findPreviousBtn);

        _cancelBtn = new Button(withMarkup, "<m>C</m>lose", this);
        hbox->addWidget(_cancelBtn);

        hbox->addSpacing(3);

        vbox->add(hbox);
    }

    wl->addCentral(vbox);

    setGeometry({ 12, 5, 55, 13});

    QObject::connect(_searchText, &InputBox::textChanged, this, [this,file] (const QString& newText) {
        _findNextBtn->setEnabled(newText.size());
        _findPreviousBtn->setEnabled(newText.size());
        file->setSearchText(_searchText->text());
        if(_searchText->text() != "" && _liveSearchBox->checkState() == Qt::Checked) {
            file->searchNext(0);
        }
    });

    QObject::connect(_findNextBtn, &Button::clicked, [=]{
        file->setSearchText(_searchText->text());
        file->searchNext();
    });

    QObject::connect(_findPreviousBtn, &Button::clicked, [=]{
        file->setSearchText(_searchText->text());
        file->searchPrevious();
    });

    QObject::connect(_cancelBtn, &Button::clicked, [=]{
        file->setSearchText("");
        file->resetSelect();
        setVisible(false);
    });

    QObject::connect(_caseMatchBox, &CheckBox::stateChanged, [=]{
        _caseSensitive = !_caseSensitive;
        if(_caseSensitive) {
            file->searchCaseSensitivity = Qt::CaseInsensitive;
        } else {
            file->searchCaseSensitivity = Qt::CaseSensitive;
        }
        update();
    });

    QObject::connect(_wrapBox, &CheckBox::stateChanged, [=]{
        if (_wrapBox->checkState() == Qt::Checked) {
            file->setSearchWrap(true);
        } else {
            file->setSearchWrap(false);
        }
    });
}

void SearchDialog::setSearchText(QString text) {
    _searchText->setText(text);
    _searchText->textChanged(text);
}

void SearchDialog::open() {
    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    r.moveBottom(terminal()->mainWidget()->geometry().height()-3);
    setGeometry(r);
    setVisible(true);
    placeFocus()->setFocus();
}
