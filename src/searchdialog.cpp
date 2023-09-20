// SPDX-License-Identifier: BSL-1.0

#include "searchdialog.h"

#include <Tui/ZButton.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWindowLayout.h>

#include "clipboard.h"
#include "groupbox.h"


class MyInputBox : public Tui::ZInputBox {
public:
    MyInputBox(Tui::ZWidget *parent) : Tui::ZInputBox(parent) {};
protected:
    void keyEvent(Tui::ZKeyEvent *event) override {
        QString text = event->text();
        Clipboard *clipboard = findFacet<Clipboard>();

        if(event->text() == "v" && event->modifiers() == Qt::Modifier::CTRL) {
            if (clipboard->getClipboard().size()) {
                insertAtCursorPosition(clipboard->getClipboard());
            }
        } else {
            Tui::ZInputBox::keyEvent(event);
        }
    }
};

SearchDialog::SearchDialog(Tui::ZWidget *parent, bool replace) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setDefaultPlacement(Qt::AlignBottom | Qt::AlignHCenter, {0, -2});
    _replace = replace;
    setVisible(false);
    setContentsMargins({ 1, 1, 2, 1});

    Tui::ZWindowLayout *wl = new Tui::ZWindowLayout();
    setLayout(wl);

    if(_replace) {
        setWindowTitle("Replace");
    } else {
        setWindowTitle("Search");
    }

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    Tui::ZLabel *labelFind;
    vbox->setSpacing(1);
    {
        Tui::ZHBoxLayout *hbox = new Tui::ZHBoxLayout();
        hbox->setSpacing(2);

        labelFind = new Tui::ZLabel(Tui::withMarkup, "Find", this);
        hbox->addWidget(labelFind);

        _searchText = new MyInputBox(this);
        labelFind->setBuddy(_searchText);
        hbox->addWidget(_searchText);

        vbox->add(hbox);
    }

    if (_replace) {
        Tui::ZHBoxLayout *hbox = new Tui::ZHBoxLayout();
        hbox->setSpacing(2);

        Tui::ZLabel *labelReplace = new Tui::ZLabel(Tui::withMarkup, "Replace", this);
        hbox->addWidget(labelReplace);

        _replaceText = new MyInputBox(this);
        labelReplace->setBuddy(_replaceText);
        hbox->addWidget(_replaceText);
        labelFind->setMinimumSize(labelReplace->sizeHint());
        vbox->add(hbox);
    }

    {
        Tui::ZHBoxLayout *hbox = new Tui::ZHBoxLayout();
        hbox->setSpacing(3);

        {
            ZWidget *gbox1 = new ZWidget(this);
            Tui::ZVBoxLayout *nbox = new Tui::ZVBoxLayout();
            gbox1->setLayout(nbox);

            _caseMatchBox = new Tui::ZCheckBox(Tui::withMarkup, "<m>M</m>atch case sensitive", gbox1);
            nbox->addWidget(_caseMatchBox);

            _plainTextRadio = new Tui::ZRadioButton(Tui::withMarkup, "Plain <m>t</m>ext", gbox1);
            _plainTextRadio->setChecked(true);
            nbox->addWidget(_plainTextRadio);

            _wordMatchRadio = new Tui::ZRadioButton(Tui::withMarkup, "Match <m>e</m>ntire word only", gbox1);
            nbox->addWidget(_wordMatchRadio);
            _wordMatchRadio->setEnabled(false);

            _regexMatchRadio = new Tui::ZRadioButton(Tui::withMarkup, "Re<m>g</m>ular expression", gbox1);
            nbox->addWidget(_regexMatchRadio);

            _escapeSequenceRadio = new Tui::ZRadioButton(Tui::withMarkup, "escape sequence", gbox1);
            _escapeSequenceRadio->setEnabled(false);
            nbox->addWidget(_escapeSequenceRadio);

            hbox->addWidget(gbox1);
        }

        {
            ZWidget *gbox2 = new ZWidget(this);
            Tui::ZVBoxLayout *nbox = new Tui::ZVBoxLayout();
            gbox2->setLayout(nbox);

            _forwardRadio = new Tui::ZRadioButton(Tui::withMarkup, "Forward", gbox2);
            _forwardRadio->setChecked(true);
            nbox->addWidget(_forwardRadio);
            _backwardRadio = new Tui::ZRadioButton(Tui::withMarkup, "<m>B</m>ackward", gbox2);
            nbox->addWidget(_backwardRadio);

            nbox->addSpacing(1);

            _liveSearchBox = new Tui::ZCheckBox(Tui::withMarkup, "<m>L</m>ive search", gbox2);
            _liveSearchBox->setCheckState(Qt::Checked);
            nbox->addWidget(_liveSearchBox);

            _wrapBox = new Tui::ZCheckBox(Tui::withMarkup, "Wrap around", gbox2);
            _wrapBox->setCheckState(Qt::Checked);
            nbox->addWidget(_wrapBox);

            hbox->addWidget(gbox2);
        }

        vbox->add(hbox);
    }

    vbox->addStretch();

    {
        Tui::ZHBoxLayout *hbox = new Tui::ZHBoxLayout();
        hbox->setSpacing(1);
        hbox->addStretch();

        _findNextBtn = new Tui::ZButton(Tui::withMarkup, "<m>N</m>ext", this);
        _findNextBtn->setDefault(true);
        hbox->addWidget(_findNextBtn);

        if(_replace) {
            _replaceBtn = new Tui::ZButton(Tui::withMarkup, "<m>R</m>eplace", this);

            _replaceAllBtn = new Tui::ZButton(Tui::withMarkup, "<m>A</m>ll", this);

            hbox->addWidget(_replaceBtn);
            hbox->addWidget(_replaceAllBtn);
        }

        _cancelBtn = new Tui::ZButton(Tui::withMarkup, "<m>C</m>lose", this);
        hbox->addWidget(_cancelBtn);

        vbox->add(hbox);
    }

    wl->setCentral(vbox);

    if(_replace) {
        setGeometry({ 0, 0, 55, 14});
    } else {
        setGeometry({ 0, 0, 55, 12});
    }

    QObject::connect(_searchText, &Tui::ZInputBox::textChanged, this, [this] (const QString& newText) {
        _findNextBtn->setEnabled(newText.size());
        if (_replaceBtn) {
            _replaceBtn->setEnabled(newText.size());
        }
        if (_replaceAllBtn) {
            _replaceAllBtn->setEnabled(newText.size());
        }
        if(_liveSearchBox->checkState() == Qt::Checked) {
            emitAllConditions();
            Q_EMIT liveSearch(_searchText->text(), _forwardRadio->checked());
        }
    });

    QObject::connect(_findNextBtn, &Tui::ZButton::clicked, [this]{
        emitAllConditions();
        Q_EMIT searchFindNext(_searchText->text(), _forwardRadio->checked());
    });

    if (_replaceBtn) {
        QObject::connect(_replaceBtn, &Tui::ZButton::clicked, [this]{
            emitAllConditions();
            Q_EMIT searchReplace(_searchText->text(), _replaceText->text(), _forwardRadio->checked());
        });
    }

    if (_replaceAllBtn) {
        QObject::connect(_replaceAllBtn, &Tui::ZButton::clicked, [=]{
            emitAllConditions();
            Q_EMIT searchReplaceAll(_searchText->text(), _replaceText->text());
        });
    }

    QObject::connect(_cancelBtn, &Tui::ZButton::clicked, [=]{
        Q_EMIT searchCanceled();
        setVisible(false);
    });

    QObject::connect(_caseMatchBox, &Tui::ZCheckBox::stateChanged, [=]{
        emitAllConditions();
    });
    QObject::connect(_regexMatchRadio, &Tui::ZRadioButton::toggled, [=]{
        emitAllConditions();
    });
    QObject::connect(_wrapBox, &Tui::ZCheckBox::stateChanged, [=]{
        emitAllConditions();
    });
    QObject::connect(_forwardRadio, &Tui::ZRadioButton::toggled, [=]{
        emitAllConditions();
    });
}


void SearchDialog::setSearchText(QString text) {
    text = text.replace('\n', "\\n");
    text = text.replace('\t', "\\t");
    _searchText->setText(text);
    _searchText->textChanged(text);
}

void SearchDialog::setReplace(bool replace) {
    _replace = replace;
}

void SearchDialog::emitAllConditions() {
    Q_EMIT searchCaseSensitiveChanged(_caseMatchBox->checkState() == Tui::CheckState::Checked);
    Q_EMIT searchRegexChanged(_regexMatchRadio->checked());
    Q_EMIT searchDirectionChanged(_forwardRadio->checked());
    Q_EMIT searchWrapChanged(_wrapBox->checkState()  == Tui::CheckState::Checked);
}

void SearchDialog::open() {
    setVisible(true);
    raise();
    placeFocus()->setFocus();
}
