#ifndef OPENDIALOG_H
#define OPENDIALOG_H

#include <testtui_lib.h>
#include <QDir>

#include "file.h"

class OpenDialog : public Dialog {
    Q_OBJECT

public:
    OpenDialog(Tui::ZWidget *parent);

public slots:
    void open();

signals:
    void fileSelected(QString filename);

private:
    void filenameChanged(QString filename);
    void userInput(QString filename);

private:
    InputBox *filenameText = nullptr;
    ListView *folder = nullptr;
    Button *okButton = nullptr;
    QDir dir;

    void refreshFolder();
};

#endif // OPENDIALOG_H
