#ifndef SAVEDIALOG_H
#define SAVEDIALOG_H

#include <testtui_lib.h>
#include <QDir>
#include "file.h"

class SaveDialog : public Dialog {
    Q_OBJECT

public:
    SaveDialog(Tui::ZWidget *parent);

public slots:
    void saveFile();

signals:
    void fileSelected(QString filename);

private:
    void filenameChanged(QString filename);
    InputBox *filenameText = nullptr;
    Button *okButton = nullptr;

};

#endif // SAVEDIALOG_H
