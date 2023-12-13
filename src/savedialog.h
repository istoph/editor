// SPDX-License-Identifier: BSL-1.0

#ifndef SAVEDIALOG_H
#define SAVEDIALOG_H

#include <QDir>

#include <Tui/ZButton.h>
#include <Tui/ZCheckBox.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>

#include "dlgfilemodel.h"
#include "file.h"


class SaveDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    SaveDialog(Tui::ZWidget *parent, File *file = nullptr);

public slots:
    void saveFile();
    void rejected();

signals:
    void fileSelected(QString filename, bool crlfMode);

private:
    void filenameChanged(QString filename);
    bool _filenameChanged(QString filename);
    void userInput(QString filename);
    void refreshFolder();

private:
    Tui::ZLabel *_currentPath = nullptr;
    Tui::ZInputBox *_filenameText = nullptr;
    Tui::ZListView *_folder = nullptr;
    Tui::ZCheckBox *_hiddenCheckBox = nullptr;
    Tui::ZCheckBox *_crlf = nullptr;
    Tui::ZButton *_cancelButton = nullptr;
    Tui::ZButton *_saveButton = nullptr;
    QDir _dir;
    std::optional<QDir> _previewDir;
    std::unique_ptr<DlgFileModel> _model;

};

#endif // SAVEDIALOG_H
