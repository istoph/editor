#ifndef OPENDIALOG_H
#define OPENDIALOG_H

#include <testtui_lib.h>
#include <QDir>

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
    Label *_curentPath = nullptr;
    InputBox *_filenameText = nullptr;
    ListView *_folder = nullptr;
    Button *_okButton = nullptr;
    Button *_cancleButton = nullptr;
    QDir _dir;

    void refreshFolder();
};

#endif // OPENDIALOG_H
