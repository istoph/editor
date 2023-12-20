// SPDX-License-Identifier: BSL-1.0

#include "syntaxhighlightdialog.h"

#include <QModelIndex>
#include <QStringListModel>

#include <Tui/ZButton.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>

#ifdef SYNTAX_HIGHLIGHTING
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#endif

static QStringList getAvailableLanguages () {
    QStringList availableLanguages;

#ifdef SYNTAX_HIGHLIGHTING
    KSyntaxHighlighting::Repository repo;
    for (const auto &def : repo.definitions()) {
      availableLanguages.append(def.name());
    }
#endif

    return availableLanguages;
}

SyntaxHighlightDialog::SyntaxHighlightDialog(Tui::ZWidget *root) : Tui::ZDialog(root) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::DeleteOnClose
               | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption | Tui::ZWindow::ResizeOption);
    setWindowTitle("Syntax Highlighting");
    setContentsMargins({1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    Tui::ZHBoxLayout *hbox1 = new Tui::ZHBoxLayout();
    Tui::ZLabel *labelSyntaxHighighting = new Tui::ZLabel(this);
    labelSyntaxHighighting->setText("Syntax Highlighting: ");
    hbox1->addWidget(labelSyntaxHighighting);

    _checkBoxOnOff = new Tui::ZCheckBox(this);
    _checkBoxOnOff->setFocus();
    hbox1->addWidget(_checkBoxOnOff);
    vbox->add(hbox1);

    Tui::ZVBoxLayout *vbox1 = new Tui::ZVBoxLayout();
    Tui::ZLabel *labelLanguage = new Tui::ZLabel(this);
    _lvLanguage = new Tui::ZListView(this);
    labelLanguage->setMarkup("<m>L</m>anguage: ");
    labelLanguage->setBuddy(_lvLanguage);
    vbox1->addWidget(labelLanguage);

    _lvLanguage->setMinimumSize({30,11});
    _availableLanguages = getAvailableLanguages();
    _availableLanguages.sort();
    _lvLanguage->setItems(_availableLanguages);

    vbox1->addWidget(_lvLanguage);
    vbox->add(vbox1);

    Tui::ZHBoxLayout *hbox4 = new Tui::ZHBoxLayout();
    hbox4->addStretch();

    Tui::ZButton *buttonClose = new Tui::ZButton(this);
    buttonClose->setText("Close");
    buttonClose->setDefault(true);
    hbox4->addWidget(buttonClose);

    vbox->add(hbox4);

    QObject::connect(_checkBoxOnOff, &Tui::ZCheckBox::stateChanged, this, [this]{
        _enabled = (_checkBoxOnOff->checkState() == Tui::Checked);
        _lvLanguage->setEnabled(_enabled);
        settingsChanged(_enabled, _lang);
    });
    QObject::connect(_lvLanguage, &Tui::ZListView::enterPressed, this, [this]{
        _lang = _lvLanguage->currentItem();
        settingsChanged(_enabled, _lang);
    });

    QObject::connect(buttonClose, &Tui::ZButton::clicked, this, [this]{
        deleteLater();
    });
}

void SyntaxHighlightDialog::updateSettings(bool enable, QString lang) {
    _enabled = enable;
    _lang = lang;

    if (_enabled) {
        _checkBoxOnOff->setCheckState(Tui::Checked);
        _lvLanguage->setEnabled(true);
    } else {
        _checkBoxOnOff->setCheckState(Tui::Unchecked);
        _lvLanguage->setEnabled(false);
    }
    int i = _availableLanguages.indexOf(_lang);
    _lvLanguage->setCurrentIndex(_lvLanguage->model()->index(i, 0));
}
