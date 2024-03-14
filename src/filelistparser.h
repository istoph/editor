// SPDX-License-Identifier: BSL-1.0

#ifndef FILELISTPARSER_H
#define FILELISTPARSER_H

#include <QString>
#include <QStringList>
#include <QVector>


struct FileListEntry {
    QString fileName;
    QString pos;
    QString search;
};

QVector<FileListEntry> parseFileList(QStringList args);

#endif // FILELISTPARSER_H
