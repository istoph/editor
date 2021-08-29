#ifndef FILEWINDOW_H
#define FILEWINDOW_H

#include <testtui_lib.h>

#include "file.h"
#include "savedialog.h"
#include "scrollbar.h"


class FileWindow : public WindowWidget {
    Q_OBJECT
public:
    FileWindow(Tui::ZWidget *parent);

public:
    File *getFileWidget();
    void setWrap(bool wrap);
    void saveFile(QString filename);
    void newFile(QString filename);
    void openFile(QString filename);

    void closePipe();
    void watchPipe();
    void setFollow(bool follow);
    bool getFollow();

    SaveDialog *saveOrSaveas();

signals:
    void readFromStandadInput(bool activ);
    void followStandadInput(bool follow);
    void fileChangedExternally(bool fileChangedExternally);
    void backingFileChanged(QString filename);

private:
    void closeRequested();
    SaveDialog *saveFileDialog();
    void reload();

    void watcherAdd();
    void watcherRemove();

    void inputPipeReadable(int socket);

private:
    File *_file = nullptr;
    ScrollBar *_scrollbarHorizontal = nullptr;
    WindowLayout *_winLayout = nullptr;
    QFileSystemWatcher *_watcher = nullptr;

    bool _follow = false;
    Tui::ZCommandNotifier *_cmdReload = nullptr;
    Tui::ZCommandNotifier *_cmdFollow = nullptr;
    Tui::ZCommandNotifier *_cmdInputPipe = nullptr;
    QSocketNotifier *_pipeSocketNotifier = nullptr;
    QByteArray _pipeLineBuffer;
};


#endif // FILEWINDOW_H
