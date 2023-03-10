// SPDX-License-Identifier: BSL-1.0

#ifndef SEARCHCOUNT_H
#define SEARCHCOUNT_H

#include <memory>

#include <QObject>
#include <QVector>


class SearchCount : public QObject {
    Q_OBJECT

public:
    explicit SearchCount();
    void run(QVector<QString> text, QString searchText, Qt::CaseSensitivity caseSensitivity, int gen, std::shared_ptr<std::atomic<int>> searchGen);
signals:
    void searchCount(int sc);
};

#endif // SEARCHCOUNT_H
