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
            GroupBox *gbox = new GroupBox("Options", this);
            Tui::ZVBoxLayout *nbox = new Tui::ZVBoxLayout();
            gbox->setLayout(nbox);

            _caseMatchBox = new Tui::ZCheckBox(Tui::withMarkup, "<m>M</m>atch case", gbox);
            nbox->addWidget(_caseMatchBox);
            Tui::ZCheckBox *wordMatchBox = new Tui::ZCheckBox(Tui::withMarkup, "Match <m>e</m>ntire word only", gbox);
            nbox->addWidget(wordMatchBox);
            wordMatchBox->setEnabled(false);
            _regexMatchBox = new Tui::ZCheckBox(Tui::withMarkup, "Re<m>g</m>ular expression", gbox);
            nbox->addWidget(_regexMatchBox);
            _liveSearchBox = new Tui::ZCheckBox(Tui::withMarkup, "<m>L</m>ive search", gbox);
            _liveSearchBox->setCheckState(Qt::Checked);
            nbox->addWidget(_liveSearchBox);

            hbox->addWidget(gbox);
        }

        {
            GroupBox *gbox = new GroupBox("Direction", this);
            Tui::ZVBoxLayout *nbox = new Tui::ZVBoxLayout();
            gbox->setLayout(nbox);

            _forward = new Tui::ZRadioButton(Tui::withMarkup, "Forward", gbox);
            _forward->setChecked(true);
            nbox->addWidget(_forward);
            _backward = new Tui::ZRadioButton(Tui::withMarkup, "<m>B</m>ackward", gbox);
            nbox->addWidget(_backward);

            _parseBox = new Tui::ZCheckBox(Tui::withMarkup, "escape sequence", gbox);
            _parseBox->setCheckState(Qt::Checked);
            _parseBox->setEnabled(false);
            nbox->addWidget(_parseBox);

            _wrapBox = new Tui::ZCheckBox(Tui::withMarkup, "Wrap around", gbox);
            _wrapBox->setCheckState(Qt::Checked);
            nbox->addWidget(_wrapBox);

            hbox->addWidget(gbox);
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

        hbox->addSpacing(3);

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
        if(_searchText->text() != "" && _liveSearchBox->checkState() == Qt::Checked) {
            Q_EMIT liveSearch(_searchText->text(), _forward->checked());
        }
    });

    QObject::connect(_findNextBtn, &Tui::ZButton::clicked, [this]{
        Q_EMIT findNext(_searchText->text(), _forward->checked());
    });

    if (_replaceBtn) {
        QObject::connect(_replaceBtn, &Tui::ZButton::clicked, [this]{
            Q_EMIT replace1(_searchText->text(), _replaceText->text(), _forward->checked());
        });
    }

    if (_replaceAllBtn) {
        QObject::connect(_replaceAllBtn, &Tui::ZButton::clicked, [=]{
            Q_EMIT replaceAll(_searchText->text(), _replaceText->text());
        });
    }

    QObject::connect(_regexMatchBox, &Tui::ZCheckBox::stateChanged, [this]{
        Q_EMIT regex(_regexMatchBox->checkState());
    });

    QObject::connect(_cancelBtn, &Tui::ZButton::clicked, [=]{
        Q_EMIT canceled();
        setVisible(false);
    });

    QObject::connect(_caseMatchBox, &Tui::ZCheckBox::stateChanged, [this]{
        _caseSensitive = !_caseSensitive;
        Q_EMIT caseSensitiveChanged(_caseSensitive);
        update();
    });

    QObject::connect(_wrapBox, &Tui::ZCheckBox::stateChanged, [=]{
        Q_EMIT wrapChanged(_wrapBox->checkState());
    });

    QObject::connect(_forward, &Tui::ZRadioButton::toggled, [=](bool state){
        Q_EMIT forwardChanged(state);
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
    setVisible(true);
    raise();
    placeFocus()->setFocus();
}
