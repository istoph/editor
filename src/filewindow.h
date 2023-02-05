// SPDX-License-Identifier: BSL-1.0

#ifndef FILEWINDOW_H
#define FILEWINDOW_H

#include <QSocketNotifier>

#include <Tui/ZWindow.h>
#include <Tui/ZWindowLayout.h>

#include "file.h"
#include "savedialog.h"
#include "scrollbar.h"
#include "wrapdialog.h"


class FileWindow : public Tui::ZWindow {
    Q_OBJECT
public:
    FileWindow(Tui::ZWidget *parent);

public:
    File *getFileWidget();
    void setWrap(Tui::ZTextOption::WrapMode wrap);
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

protected:
    void closeEvent(Tui::ZCloseEvent *event) override;
    void resizeEvent(Tui::ZResizeEvent *event) override;
    void moveEvent(Tui::ZMoveEvent *event) override;

private:
    void updateBorders();
    void closeRequested();
    SaveDialog *saveFileDialog();
    WrapDialog *wrapDialog();
    void reload();

    void watcherAdd();
    void watcherRemove();

    void inputPipeReadable(int socket);

private:
    File *_file = nullptr;
    ScrollBar *_scrollbarHorizontal = nullptr;
    ScrollBar *_scrollbarVertical = nullptr;
    Tui::ZWindowLayout *_winLayout = nullptr;
    QFileSystemWatcher *_watcher = nullptr;

    bool _follow = false;
    Tui::ZCommandNotifier *_cmdReload = nullptr;
    Tui::ZCommandNotifier *_cmdFollow = nullptr;
    Tui::ZCommandNotifier *_cmdInputPipe = nullptr;
    QSocketNotifier *_pipeSocketNotifier = nullptr;
    QByteArray _pipeLineBuffer;
};


#endif // FILEWINDOW_H
