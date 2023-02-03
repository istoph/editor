// SPDX-License-Identifier: BSL-1.0

#ifndef FILECATEGORIZE_H
#define FILECATEGORIZE_H

#include <QString>

enum class FileCategory { stdin_file, open_file, new_file, dir, invalid_dir_not_exist, invalid_dir_not_writable, invalid_filetype, invalid_error };

FileCategory fileCategorize(QString input, int maxrecursion = 0);

#endif // FILECATEGORIZE_H
