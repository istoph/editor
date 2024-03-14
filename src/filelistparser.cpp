// SPDX-License-Identifier: BSL-1.0

#include "filelistparser.h"


QVector<FileListEntry> parseFileList(QStringList args)
{
    QVector<FileListEntry> res;
    QString pos = "", search = "";
    while (!args.empty()) {
        if (args.first().startsWith("+")) {
            if(args.first().startsWith("+/")) {
                if (search != "") {
                    return {};
                }
                search = args.first().right(args.first().length() - 2);
                args.removeFirst();
            } else {
                if (pos != "") {
                    return {};
                }
                pos = args.first(); //TODO: remove +
                args.removeFirst();
            }
            continue;
        }
        res.append({args.first(), pos, search});
        pos = "";
        search = "";
        args.removeFirst();
    }
    if (!res.empty()) {
        if (pos != "") {
            if (res.last().pos != "") {
                return {};
            }
            res.last().pos = pos;
        }
        if (search != "") {
            if (res.last().search != "") {
                return {};
            }
            res.last().search = search;
        }
    }
    return res;
}
