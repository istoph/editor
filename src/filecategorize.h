#ifndef FILECATEGORIZE_H
#define FILECATEGORIZE_H

#include <QString>

enum class FileCategory { stdin, open_file, new_file, dir, invalid_dir_not_exist, invalid_dir_not_writable, invalid_filetype, invalid_error };

FileCategory fileCategorize(QString input);

#endif // FILECATEGORIZE_H
