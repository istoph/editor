#ifndef QUITDIALOG_H
#define QUITDIALOG_H

#include <testtui_lib.h>
#include <file.h>

class QuitDialog : public Dialog
{
    Q_OBJECT
public:
    explicit QuitDialog(Tui::ZWidget *parent, QString filename);

signals:
    void saveSelected();
    void exitSelected();
};

#endif // QUITDIALOG_H
