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
    void userInput(QString filename);

private:
    InputBox *filenameText = nullptr;
    ListView *folder = nullptr;
    Button *okButton = nullptr;
    QDir dir;

    void refreshFolder();
};

#endif // SAVEDIALOG_H
