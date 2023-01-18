#ifndef SAVEDIALOG_H
#define SAVEDIALOG_H

#include <QDir>

#include <Tui/ZButton.h>
#include <Tui/ZCheckBox.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>

#include "file.h"
#include "overwritedialog.h"
#include "dlgfilemodel.h"

class SaveDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    SaveDialog(Tui::ZWidget *parent, File *file = nullptr);

public slots:
    void saveFile();
    void rejected();

signals:
    void fileSelected(QString filename);

private:
    void filenameChanged(QString filename);
    bool _filenameChanged(QString filename, bool absolutePath = false);
    void userInput(QString filename);
    void refreshFolder();

private:
    Tui::ZLabel *_curentPath = nullptr;
    Tui::ZInputBox *_filenameText = nullptr;
    Tui::ZListView *_folder = nullptr;
    Tui::ZCheckBox *_hiddenCheckBox = nullptr;
    Tui::ZCheckBox *_dos = nullptr;
    Tui::ZButton *_cancelButton = nullptr;
    Tui::ZButton *_saveButton = nullptr;
    QDir _dir;
    std::unique_ptr<DlgFileModel> _model;

};

#endif // SAVEDIALOG_H
