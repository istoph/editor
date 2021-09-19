#ifndef OPENDIALOG_H
#define OPENDIALOG_H

#include <QDir>

#include <Tui/ZButton.h>
#include <Tui/ZCheckBox.h>
#include <Tui/ZLabel.h>

#include <testtui_lib.h>

#include "file.h"

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

private:
    Tui::ZLabel *_curentPath = nullptr;
    InputBox *_filenameText = nullptr;
    ListView *_folder = nullptr;
    Tui::ZCheckBox *_hiddenCheckBox = nullptr;
    Tui::ZButton *_okButton = nullptr;
    Tui::ZButton *_cancelButton = nullptr;
    QDir _dir;

    void refreshFolder();
};

#endif // OPENDIALOG_H
