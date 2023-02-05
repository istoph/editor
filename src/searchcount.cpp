// SPDX-License-Identifier: BSL-1.0

#include "searchcount.h"


SearchCount::SearchCount() {

}

void SearchCount::run(QVector<QString> text, QString searchText, Qt::CaseSensitivity caseSensitivity, int gen, std::shared_ptr<std::atomic<int>> searchGen) {
    int found = 0;
    for (int line = 0; line < text.size(); line++) {
        if (gen != *searchGen) {
            return;
        }
        found += text[line].count(searchText, caseSensitivity);
        searchCount(found);
    }
}
