#ifndef FILELISTPARSER_H
#define FILELISTPARSER_H

#include <QString>
#include <QStringList>
#include <QVector>

struct FileListEntry {
    QString fileName;
    QString pos;
};

QVector<FileListEntry> parseFileList(QStringList args);

#endif // FILELISTPARSER_H
