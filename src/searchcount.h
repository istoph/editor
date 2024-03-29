// SPDX-License-Identifier: BSL-1.0

#ifndef SEARCHCOUNT_H
#define SEARCHCOUNT_H

#include <memory>

#include <QObject>
#include <QVector>

#include <Tui/ZDocument.h>

class SearchCount : public QObject {
    Q_OBJECT

public:
    explicit SearchCount();
    void run(Tui::ZDocumentSnapshot snap, QString searchText, Qt::CaseSensitivity caseSensitivity, int gen, std::shared_ptr<std::atomic<int>> searchGen);
signals:
    void searchCount(int sc);
};

class SearchCountSignalForwarder : public QObject {
    Q_OBJECT
signals:
    void searchCount(int count);
};

#endif // SEARCHCOUNT_H
