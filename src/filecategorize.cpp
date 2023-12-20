// SPDX-License-Identifier: BSL-1.0

#include "filecategorize.h"

#include <QDir>
#include <QFileInfo>


FileCategory fileCategorize(QString input, int maxrecursion) {

    //STDIN
    if (input == "-") {
        return FileCategory::stdin_file;
    }

    //DIR
    QFileInfo fileInfo(input);
    if (fileInfo.isDir()) {
        return FileCategory::dir;
    }

    if (fileInfo.isSymLink()) {
        if (maxrecursion > 10) {
            return FileCategory::invalid_error;
        }
        return fileCategorize(fileInfo.symLinkTarget(), ++maxrecursion);
    }

    //FILE
    if (fileInfo.exists() && fileInfo.isFile()) {
        if (fileInfo.isReadable())
            return FileCategory::open_file;
        else {
            return FileCategory::invalid_file_not_readable;
        }
    } else if (fileInfo.exists()) {
        return FileCategory::invalid_filetype;
    }

    if (fileInfo.absoluteDir().exists()) {
        QFileInfo dir(fileInfo.absoluteDir().absolutePath());
        if (dir.isWritable()) {
            return FileCategory::new_file;
        } else {
            return FileCategory::invalid_dir_not_writable;
        }
    } else {
        return FileCategory::invalid_dir_not_exist;
    }

    //return FileCategory::invalid_error;
}
