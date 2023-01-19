#ifndef OPENDIALOG_H
#define OPENDIALOG_H

#include <QDir>

#include <Tui/ZButton.h>
#include <Tui/ZCheckBox.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>

#include "file.h"
#include "dlgfilemodel.h"


class OpenDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    OpenDialog(Tui::ZWidget *parent, QString path = "");

public slots:
    void rejected();

signals:
    void fileSelected(QString filename);

private:
    void filenameChanged(QString filename);
    void userInput(QString filename);
    void refreshFolder();

private:
    Tui::ZLabel *_curentPath = nullptr;
    Tui::ZInputBox *_filenameText = nullptr;
    Tui::ZListView *_folder = nullptr;
    Tui::ZCheckBox *_hiddenCheckBox = nullptr;
    Tui::ZButton *_okButton = nullptr;
    Tui::ZButton *_cancelButton = nullptr;
    QDir _dir;
    std::unique_ptr<DlgFileModel> _model;
};

#endif // OPENDIALOG_H
