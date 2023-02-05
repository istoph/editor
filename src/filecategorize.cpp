// SPDX-License-Identifier: BSL-1.0

#include "filecategorize.h"

#include <QDir>
#include <QFileInfo>


FileCategory fileCategorize(QString input, int maxrecursion) {

    //STDIN
    if(input == "-") {
        return FileCategory::stdin_file;
    }

    //DIR
    QFileInfo datei(input);
    if(datei.isDir()) {
        return FileCategory::dir;
    }

    if (datei.isSymLink()) {
        if (maxrecursion > 10) {
            return FileCategory::invalid_error;
        }
        return fileCategorize(datei.symLinkTarget(), ++maxrecursion);
    }

    //FILE
    if (datei.exists() && datei.isFile()) { // && datei.isReadable()
        return FileCategory::open_file;
    } else if (datei.exists()) {
        return FileCategory::invalid_filetype;
    }

    if (datei.absoluteDir().exists()) {
        QFileInfo dir(datei.absoluteDir().absolutePath());
        if(dir.isWritable()) {
            return FileCategory::new_file;
        } else {
            return FileCategory::invalid_dir_not_writable;
        }
    } else {
        return FileCategory::invalid_dir_not_exist;
    }

    //return FileCategory::invalid_error;
}
