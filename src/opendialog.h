#ifndef OPENDIALOG_H
#define OPENDIALOG_H

#include <testtui_lib.h>

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
    InputBox *filenameText = nullptr;
    Button *okButton = nullptr;



};

#endif // OPENDIALOG_H
