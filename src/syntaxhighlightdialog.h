// SPDX-License-Identifier: BSL-1.0

#ifndef SYNTAXHIGHLIGHTDIALOG_H
#define SYNTAXHIGHLIGHTDIALOG_H

#include <Tui/ZCheckBox.h>
#include <Tui/ZDialog.h>
#include <Tui/ZListView.h>

class SyntaxHighlightDialog : public Tui::ZDialog {
public:
    Q_OBJECT

public:
    explicit SyntaxHighlightDialog(Tui::ZWidget *root);
    void updateSettings(bool enable, QString lang);

signals:
    void settingsChanged(bool enable, QString lang);

private:
    Tui::ZCheckBox *_checkBoxOnOff = nullptr;
    Tui::ZListView *_lvLanguage = nullptr;
    bool _enabled = false;
    QString _lang;
    QStringList _availableLanguages;
};

#endif // SYNTAXHIGHLIGHTDIALOG_H
