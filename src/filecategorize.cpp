#include "filecategorize.h"
#include <QFileInfo>
#include <QDir>

FileCategory fileCategorize(QString input) {

    //STDIN
    if(input == "-") {
        return FileCategory::stdin;
    }

    //DIR
    QFileInfo datei(input);
    if(datei.isDir()) {
        return FileCategory::dir;
    }
    /*
    if (datei.isSymLink()) {
        return fileCategorize(datei.symLinkTarget());
    }
    */

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
