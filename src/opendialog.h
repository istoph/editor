#ifndef OPENDIALOG_H
#define OPENDIALOG_H

#include <QDir>

#include <Tui/ZButton.h>
#include <Tui/ZCheckBox.h>
#include <Tui/ZLabel.h>

#include <testtui_lib.h>

#include "file.h"
#include "dlgfilemodel.h"


class OpenDialog : public Dialog {
    Q_OBJECT

public:
    OpenDialog(Tui::ZWidget *parent, QString path = "");
    void pathSelected(QString path);

public slots:
    void open();
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
    Tui::ZButton *_okButton = nullptr;
    Tui::ZButton *_cancelButton = nullptr;
    QDir _dir;
    std::unique_ptr<DlgFileModel> _model;
};

#endif // OPENDIALOG_H
