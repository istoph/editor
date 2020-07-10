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
    Button *_cancleButton = nullptr;
    Button *_okButton = nullptr;
    QDir _dir;

    void refreshFolder();
};

#endif // SAVEDIALOG_H
