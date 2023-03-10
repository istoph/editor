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
    void caseSensitiveChanged(bool value);
    void wrapChanged(bool value);
    void forwardChanged(bool value);
    void canceled();
    void liveSearch(QString text, bool forward);
    void findNext(QString text, bool regex, bool forward);
    void replace1(QString text, QString replacement, bool regex, bool forward);
    void replaceAll(QString text, QString replacement, bool regex);

public slots:
    void open();

private:
    bool _replace = false;
    Tui::ZInputBox *_searchText = nullptr;
    Tui::ZInputBox *_replaceText = nullptr;
    Tui::ZButton *_findNextBtn = nullptr;
    Tui::ZButton *_findPreviousBtn = nullptr;
    Tui::ZButton *_cancelBtn = nullptr;
    Tui::ZButton *_replaceBtn = nullptr;
    Tui::ZButton *_replaceAllBtn = nullptr;
    bool _caseSensitive = true;
    Tui::ZCheckBox *_caseMatchBox = nullptr;
    Tui::ZCheckBox *_parseBox = nullptr;
    Tui::ZCheckBox *_wrapBox = nullptr;
    Tui::ZCheckBox *_regexMatchBox = nullptr;
    Tui::ZCheckBox *_liveSearchBox = nullptr;
    Tui::ZRadioButton *_forward = nullptr;
    Tui::ZRadioButton *_backward = nullptr;
};

#endif // SEARCHDIALOG_H
