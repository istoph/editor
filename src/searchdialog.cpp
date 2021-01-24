#include "searchdialog.h"

class MyInputBox : public InputBox {
public:
    MyInputBox(Tui::ZWidget *parent) : InputBox(parent) {};
protected:
    void keyEvent(Tui::ZKeyEvent *event) override {
        QString text = event->text();
        Clipboard *clipboard = findFacet<Clipboard>();

        if(event->text() == "v" && event->modifiers() == Qt::Modifier::CTRL) {
            if(!clipboard->getClipboard().empty()) {
                insertAtCursorPosition(clipboard->getClipboard()[0]);
            }
        } else {
            InputBox::keyEvent(event);
        }
    }
};

SearchDialog::SearchDialog(Tui::ZWidget *parent, File *file, bool replace) : Dialog(parent) {
    _replace = replace;
    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    WindowLayout *wl = new WindowLayout();
    setLayout(wl);

    if(_replace) {
        setWindowTitle("Replace");
    } else {
        setWindowTitle("Search");
    }

    VBoxLayout *vbox = new VBoxLayout();
    vbox->setSpacing(1);
    {
        HBoxLayout* hbox = new HBoxLayout();
        hbox->setSpacing(2);

        Label *l = new Label(withMarkup, "F<m>i</m>nd   ", this);
        hbox->addWidget(l);

        _searchText = new MyInputBox(this);
        l->setBuddy(_searchText);
        hbox->addWidget(_searchText);

        vbox->add(hbox);
    }

    {
        HBoxLayout* hbox = new HBoxLayout();
        hbox->setSpacing(2);

        Label *l = new Label(withMarkup, "Replace", this);
        hbox->addWidget(l);

        _replaceText = new MyInputBox(this);
        l->setBuddy(_replaceText);
        hbox->addWidget(_replaceText);
        if(_replace) {
            vbox->add(hbox);
        }
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
            _regexMatchBox = new CheckBox(withMarkup, "Re<m>g</m>ular expression", gbox);
            nbox->addWidget(_regexMatchBox);
            _liveSearchBox = new CheckBox(withMarkup, "<m>L</m>ive search", gbox);
            _liveSearchBox->setCheckState(Qt::Checked);
            nbox->addWidget(_liveSearchBox);

            hbox->addWidget(gbox);
        }

        {
            GroupBox *gbox = new GroupBox("Direction", this);
            VBoxLayout *nbox = new VBoxLayout();
            gbox->setLayout(nbox);

            _forward = new RadioButton(withMarkup, "<m>F</m>orward", gbox);
            _forward->setChecked(true);
            nbox->addWidget(_forward);
            _backward = new RadioButton(withMarkup, "<m>B</m>ackward", gbox);
            nbox->addWidget(_backward);

            _parseBox = new CheckBox(withMarkup, "escape sequence", gbox);
            _parseBox->setCheckState(Qt::Checked);
            _parseBox->setEnabled(false);
            nbox->addWidget(_parseBox);

            _wrapBox = new CheckBox(withMarkup, "<m>W</m>rap around", gbox);
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

        _replaceBtn = new Button(withMarkup, "<m>R</m>eplace", this);

        _replaceAllBtn = new Button(withMarkup, "<m>A</m>ll", this);

        if(_replace) {
            hbox->addWidget(_replaceBtn);
            hbox->addWidget(_replaceAllBtn);
        }

        _cancelBtn = new Button(withMarkup, "<m>C</m>lose", this);
        hbox->addWidget(_cancelBtn);

        hbox->addSpacing(3);

        vbox->add(hbox);
    }

    wl->addCentral(vbox);

    if(_replace) {
        setGeometry({ 12, 5, 55, 14});
    } else {
        setGeometry({ 12, 5, 55, 12});
    }

    QObject::connect(_searchText, &InputBox::textChanged, this, [this,file] (const QString& newText) {
        _findNextBtn->setEnabled(newText.size());
        _replaceBtn->setEnabled(newText.size());
        _replaceAllBtn->setEnabled(newText.size());
        if(_searchText->text() != "" && _liveSearchBox->checkState() == Qt::Checked) {
            file->setSearchText(_searchText->text());
            file->setSearchDirection(_forward->checked());
            if(_forward->checked()) {
                file->setCursorPosition({file->getCursorPosition().x() - _searchText->text().size(), file->getCursorPosition().y()});
            } else {
                file->setCursorPosition({file->getCursorPosition().x() + _searchText->text().size(), file->getCursorPosition().y()});
            }
            file->runSearch(false);
        }
    });

    QObject::connect(_findNextBtn, &Button::clicked, [=]{
        file->setSearchText(_searchText->text());
        file->setRegex(_regexMatchBox->checkState());
        file->setSearchDirection(_forward->checked());
        file->runSearch(false);
    });

    QObject::connect(_replaceBtn, &Button::clicked, [=]{
        file->setSearchText(_searchText->text());
        file->setReplaceText(_replaceText->text());
        file->setRegex(_regexMatchBox->checkState());
        file->setReplaceSelected();
        file->setSearchDirection(_forward->checked());
        file->runSearch(false);
    });

     QObject::connect(_replaceAllBtn, &Button::clicked, [=]{
         file->setRegex(_regexMatchBox->checkState());
         file->replaceAll(_searchText->text(), _replaceText->text());
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

    QObject::connect(_forward, &RadioButton::toggled, [=](bool state){
        file->setSearchDirection(state);
    });
}

void SearchDialog::setSearchText(QString text) {
    _searchText->setText(text);
    _searchText->textChanged(text);
}

void SearchDialog::setReplace(bool replace) {
    _replace = replace;
}

void SearchDialog::open() {
    QRect r = geometry();
    r.moveCenter(terminal()->mainWidget()->geometry().center());
    r.moveBottom(terminal()->mainWidget()->geometry().height()-3);
    setGeometry(r);
    setVisible(true);
    raise();
    placeFocus()->setFocus();
}
