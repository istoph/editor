#ifndef DLGFILEMODEL_H
#define DLGFILEMODEL_H

#include <QDir>

#include <Tui/Misc/AbstractTableModelTrackBy.h>

#include "file.h"

class DlgFileModel : public Tui::Misc::AbstractTableModelTrackBy<QString> {
    Q_OBJECT
public:
    DlgFileModel(const QDir &dir);

public:
    void update();
    void setDisplayHidden(bool displayHidden);
    void setDirectory(const QDir &dir);

private:
    QDir _dir;
    QFileSystemWatcher _watcher;
    bool _displayHidden = false;
};

#endif // DLGFILEMODEL_H
