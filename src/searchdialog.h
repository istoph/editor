// SPDX-License-Identifier: BSL-1.0

#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <Tui/ZButton.h>
#include <Tui/ZCheckBox.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZRadioButton.h>


class SearchDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    SearchDialog(Tui::ZWidget *parent, bool replace = false);
    void setSearchText(QString text);
    void setReplace(bool replace);

signals:
    void searchCaseSensitiveChanged(bool value);
    void searchRegexChanged(bool searchRegexChanged);
    void searchDirectionChanged(bool value);
    void searchWrapChanged(bool value);
    void liveSearch(QString text, bool forward);
    void searchFindNext(QString text, bool forward);
    void searchReplace(QString text, QString replacement, bool forward);
    void searchReplaceAll(QString text, QString replacement);
    void searchCanceled();

public slots:
    void open();

private:
    bool _replace = false;
    Tui::ZInputBox *_searchText = nullptr;
    Tui::ZInputBox *_replaceText = nullptr;

    Tui::ZCheckBox *_caseMatchBox = nullptr;
    Tui::ZRadioButton *_plainTextRadio = nullptr;
    Tui::ZRadioButton *_wordMatchRadio = nullptr;
    Tui::ZRadioButton *_regexMatchRadio = nullptr;
    Tui::ZRadioButton *_escapeSequenceRadio = nullptr;

    Tui::ZCheckBox *_liveSearchBox = nullptr;
    Tui::ZCheckBox *_wrapBox = nullptr;

    Tui::ZRadioButton *_forwardRadio = nullptr;
    Tui::ZRadioButton *_backwardRadio = nullptr;

    Tui::ZButton *_findNextBtn = nullptr;
    Tui::ZButton *_findPreviousBtn = nullptr;
    Tui::ZButton *_replaceBtn = nullptr;
    Tui::ZButton *_replaceAllBtn = nullptr;
    Tui::ZButton *_cancelBtn = nullptr;
};

#endif // SEARCHDIALOG_H
