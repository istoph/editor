// SPDX-License-Identifier: BSL-1.0

#include "filelistparser.h"


QVector<FileListEntry> parseFileList(QStringList args)
{
    QVector<FileListEntry> res;
    QString pos = "";
    while (!args.empty()) {
        if (args.first().startsWith("+")) {
            if (pos != "") {
                return {};
            }
            pos = args.first();
            args.removeFirst();
            continue;
        }
        res.append({args.first(), pos});
        pos = "";
        args.removeFirst();
    }
    if (pos != "" && !res.empty()) {
        if (res.last().pos != "") {
            return {};
        }
        res.last().pos = pos;
    }
    return res;
}
