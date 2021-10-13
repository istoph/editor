#ifndef SAVEDIALOG_H
#define SAVEDIALOG_H

#include <QDir>

#include <Tui/ZButton.h>
#include <Tui/ZCheckBox.h>
#include <Tui/ZLabel.h>

#include <testtui_lib.h>

#include "file.h"
#include "overwritedialog.h"
#include "dlgfilemodel.h"

class SaveDialog : public Dialog {
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
    void userInput(QString filename);
    void refreshFolder();

private:
    Tui::ZLabel *_curentPath = nullptr;
    InputBox *_filenameText = nullptr;
    ListView *_folder = nullptr;
    Tui::ZCheckBox *_hiddenCheckBox = nullptr;
    Tui::ZCheckBox *_dos = nullptr;
    Tui::ZButton *_cancelButton = nullptr;
    Tui::ZButton *_okButton = nullptr;
    QDir _dir;
    std::unique_ptr<DlgFileModel> _model;

};

#endif // SAVEDIALOG_H
