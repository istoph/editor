// SPDX-License-Identifier: BSL-1.0

#include "searchdialog.h"

#include <QRegularExpression>

#include <Tui/ZButton.h>
#include <Tui/ZClipboard.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWindowLayout.h>

#include "groupbox.h"


SearchDialog::SearchDialog(Tui::ZWidget *parent, bool replace) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::MoveOption | Tui::ZWindow::ResizeOption | Tui::ZWindow::AutomaticOption);
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

        _searchText = new Tui::ZInputBox(this);
        labelFind->setBuddy(_searchText);
        hbox->addWidget(_searchText);

        vbox->add(hbox);
    }

    if (_replace) {
        Tui::ZHBoxLayout *hbox = new Tui::ZHBoxLayout();
        hbox->setSpacing(2);

        Tui::ZLabel *labelReplace = new Tui::ZLabel(Tui::withMarkup, "Replace", this);
        hbox->addWidget(labelReplace);

        _replaceText = new Tui::ZInputBox(this);
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

            _escapeSequenceRadio = new Tui::ZRadioButton(Tui::withMarkup, "<m>T</m>ext with escapes", gbox1);
            _escapeSequenceRadio->setChecked(true);
            nbox->addWidget(_escapeSequenceRadio);

            _wordMatchRadio = new Tui::ZRadioButton(Tui::withMarkup, "Match entire wor<m>d</m> only", gbox1);
            nbox->addWidget(_wordMatchRadio);

            _regexMatchRadio = new Tui::ZRadioButton(Tui::withMarkup, "Re<m>g</m>ular expression", gbox1);
            nbox->addWidget(_regexMatchRadio);

            _plainTextRadio = new Tui::ZRadioButton(Tui::withMarkup, "Pla<m>i</m>n text", gbox1);
            nbox->addWidget(_plainTextRadio);

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

        hbox->addStretch();

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
        setMinimumSize(48, 14);
        setMaximumSize(Tui::tuiMaxSize, 14);
    } else {
        setGeometry({ 0, 0, 55, 12});
        setMinimumSize(48, 12);
        setMaximumSize(Tui::tuiMaxSize, 12);
    }

    QObject::connect(_searchText, &Tui::ZInputBox::textChanged, this, [this] (const QString& newText) {
        _findNextBtn->setEnabled(newText.size());
        if (_replaceBtn) {
            _replaceBtn->setEnabled(newText.size());
        }
        if (_replaceAllBtn) {
            _replaceAllBtn->setEnabled(newText.size());
        }
        if (_liveSearchBox->checkState() == Qt::Checked) {
            emitAllConditions();
            emitLiveSearch();
        }
    });

    QObject::connect(_findNextBtn, &Tui::ZButton::clicked, [this] {
        emitAllConditions();
        Q_EMIT searchFindNext(translateSearch(_searchText->text()), _forwardRadio->checked());
    });

    if (_replaceBtn) {
        QObject::connect(_replaceBtn, &Tui::ZButton::clicked, [this] {
            emitAllConditions();
            Q_EMIT searchReplace(translateSearch(_searchText->text()),
                                 translateReplace(_replaceText->text()), _forwardRadio->checked());
        });
    }

    if (_replaceAllBtn) {
        QObject::connect(_replaceAllBtn, &Tui::ZButton::clicked, [=] {
            emitAllConditions();
            Q_EMIT searchReplaceAll(translateSearch(_searchText->text()),
                                    translateReplace(_replaceText->text()));
        });
    }

    QObject::connect(_cancelBtn, &Tui::ZButton::clicked, [=] {
        Q_EMIT searchCanceled();
        setVisible(false);
    });

    QObject::connect(_caseMatchBox, &Tui::ZCheckBox::stateChanged, [=] {
        emitAllConditions();
    });

    QObject::connect(_escapeSequenceRadio, &Tui::ZRadioButton::toggled, [=] {
        if (!_escapeSequenceRadio->checked()) {
            return;
        }
        emitAllConditions();
        if (_liveSearchBox->checkState() == Qt::Checked) {
            emitLiveSearch();
        }
    });
    QObject::connect(_wordMatchRadio, &Tui::ZRadioButton::toggled, [=] {
        if (!_wordMatchRadio->checked()) {
            return;
        }
        emitAllConditions();
        if (_liveSearchBox->checkState() == Qt::Checked) {
            emitLiveSearch();
        }
    });
    QObject::connect(_regexMatchRadio, &Tui::ZRadioButton::toggled, [=] {
        if (!_regexMatchRadio->checked()) {
            return;
        }
        emitAllConditions();
        if (_liveSearchBox->checkState() == Qt::Checked) {
            emitLiveSearch();
        }
    });
    QObject::connect(_plainTextRadio, &Tui::ZRadioButton::toggled, [=] {
        if (!_plainTextRadio->checked()) {
            return;
        }
        emitAllConditions();
        if (_liveSearchBox->checkState() == Qt::Checked) {
            emitLiveSearch();
        }
    });

    QObject::connect(_wrapBox, &Tui::ZCheckBox::stateChanged, [=] {
        emitAllConditions();
    });
    QObject::connect(_forwardRadio, &Tui::ZRadioButton::toggled, [=] {
        emitAllConditions();
    });
}


void SearchDialog::setSearchText(QString text) {
    if (text != "") {
        if (_searchText->text() == "" || !_regexMatchRadio->checked()) {
            if (_escapeSequenceRadio->checked()) {
                text.replace('\n', "\\n");
                text.replace('\t', "\\t");
                text.replace('\\', "\\\\");
            }
            _searchText->setText(text);
            _searchText->textChanged(text);
        }
    }
}

void SearchDialog::setReplace(bool replace) {
    _replace = replace;
}

void SearchDialog::emitAllConditions() {
    Q_EMIT searchCaseSensitiveChanged(_caseMatchBox->checkState() == Tui::CheckState::Checked);
    Q_EMIT searchRegexChanged(_regexMatchRadio->checked()
                              || _wordMatchRadio->checked());
    Q_EMIT searchDirectionChanged(_forwardRadio->checked());
    Q_EMIT searchWrapChanged(_wrapBox->checkState()  == Tui::CheckState::Checked);
}

void SearchDialog::emitLiveSearch() {
    Q_EMIT liveSearch(translateSearch(_searchText->text()), _forwardRadio->checked());
}

QString SearchDialog::translateSearch(const QString &in) {
    QString text = in;
    if (_escapeSequenceRadio->checked()) {
        text = applyEscapeSequences(text);
    } else if (_wordMatchRadio->checked()) {
        text = "\\b" + QRegularExpression::escape(text) + "\\b";
    }

    return text;
}

QString SearchDialog::translateReplace(const QString &in) {
    QString result = in;
    if (_escapeSequenceRadio->checked() || _regexMatchRadio->checked()) {
        result = applyEscapeSequences(_replaceText->text());
    }

    return result;
}

QString SearchDialog::applyEscapeSequences(const QString &text) {
    QString result;
    result.reserve(text.size());

    auto isHexDigit = [&](int index) {
        QChar digitChar = text[index];
        return (digitChar >= '0' && digitChar <= '9')
                || (digitChar >= 'a' && digitChar <= 'f')
                || (digitChar >= 'A' && digitChar <= 'F');
    };

    for (int i = 0; i < text.size(); i++) {
        const QChar startChar = text[i];

        if (startChar == '\\' && i + 1 < text.size()) {
            const QChar nextChar = text[i + 1];
            if (nextChar == '0') {
                // octal escape sequence
                int value = 0;

                if (i + 2 >= text.size()) {
                    i += 1;
                    result += QChar(0);
                } else {
                    QChar digitChar = text[i + 2];
                    if (digitChar >= '0' && digitChar <= '3') {
                        value = digitChar.unicode() - '0';
                        if (i + 3 >= text.size()) {
                            i += 2;
                            result += QChar(value);
                        } else {
                            digitChar = text[i + 3];
                            if (digitChar >= '0' && digitChar <= '7') {
                                value = value * 8 + (digitChar.unicode() - '0');
                                if (i + 4 >= text.size()) {
                                    i += 3;
                                    result += QChar(value);
                                } else {
                                    digitChar = text[i + 4];
                                    if (digitChar >= '0' && digitChar <= '7') {
                                        value = value * 8 + (digitChar.unicode() - '0');

                                        i += 4;
                                        result += QChar(value);
                                    } else {
                                        i += 3;
                                        result += QChar(value);
                                    }
                                }
                            } else {
                                i += 2;
                                result += QChar(value);
                            }
                        }
                    } else {
                        i += 1;
                        result += QChar(0);
                    }
                }
            } else if (nextChar == 'x' && i + 3 < text.size()
                       && isHexDigit(i + 2) && isHexDigit(i + 3)) {
                // hex
                const int value = text.midRef(i + 2, 2).toInt(nullptr, 16);
                i += 3;
                result += QChar(value);
            } else if (nextChar == 'u' && i + 5 < text.size()
                       && isHexDigit(i + 2) && isHexDigit(i + 3) && isHexDigit(i + 4) && isHexDigit(i + 5)) {
                // unicode
                const int value = text.midRef(i + 2, 4).toInt(nullptr, 16);
                i += 5;
                result += QChar(value);
            } else if (nextChar == '\\') {
                result += '\\';
                i += 1;
            } else if (nextChar == 'a') {
                result += QChar(0x07);
                i += 1;
            } else if (nextChar == 'b') {
                result += QChar(0x08);
                i += 1;
            } else if (nextChar == 'f') {
                result += QChar(0x0C);
                i += 1;
            } else if (nextChar == 'n') {
                result += '\n';
                i += 1;
            } else if (nextChar == 'r') {
                result += '\r';
                i += 1;
            } else if (nextChar == 't') {
                result += '\t';
                i += 1;
            } else if (nextChar == 'v') {
                result += QChar(0x0b);
                i += 1;
            } else {
                result += nextChar;
                i += 1;
            }
        } else {
            result += startChar;
        }

    }
    return result;
}

void SearchDialog::open() {
    setVisible(true);
    raise();
    placeFocus()->setFocus();
}
