// SPDX-License-Identifier: BSL-1.0

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QObject>
#include <QVector>


class Clipboard : public QObject
{
    Q_OBJECT
public:
    Clipboard();
    void clear();
    QString getClipboard();
    void setClipboard(const QString &clipboard);

private:
    QString _clipboard;
};

#endif // CLIPBOARD_H
