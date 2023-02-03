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
    void append(QString append);
    void append(QVector<QString> append);
    void clear();
    QVector<QString> getClipboard();
    void setClipboard(QVector<QString> clipboard);

private:
    QVector<QString> _clipboard;
};

#endif // CLIPBOARD_H
