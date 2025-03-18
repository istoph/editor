// SPDX-License-Identifier: BSL-1.0

#include "markermanager.h"
#include <algorithm>
#include <vector>

MarkerManager::MarkerManager() {

}

void MarkerManager::addMarker(Tui::ZDocument *doc, int line) {
    markers.emplace_back(doc, line);
}

void MarkerManager::removeMarker(int line) {
    markers.erase(std::remove_if(markers.begin(), markers.end(),
                                 [line](const Tui::ZDocumentLineMarker& marker) {
                                     return marker.line() == line;
                                 }),
                  markers.end());
}

bool MarkerManager::hasMarker() {
    return !markers.empty();
}

bool MarkerManager::hasMarker(int line) {
    return std::any_of(markers.begin(), markers.end(),
                       [line](const Tui::ZDocumentLineMarker& marker) {
                           return marker.line() == line;
                       });
}

std::vector<int> MarkerManager::listMarkerIntern() {
    std::vector<int> list;
    list.reserve(markers.size());
    for (const auto& marker : markers) {
        list.push_back(marker.line());
    }
    std::sort(list.begin(), list.end());
    return list;
}
QList<int> MarkerManager::listMarker() {
    std::vector<int> vec = listMarkerIntern();
    QList<int> list;
    for (int value : vec) {
        list.append(value);
    }
    return list;
}

int MarkerManager::nextMarker(int line) {
    std::vector<int> list = listMarkerIntern();
    auto it = std::upper_bound(list.begin(), list.end(), line);
    if (it != list.end()) {
        return *it;
    } else {
        auto it = std::min_element(list.begin(), list.end());
        if (it != list.end()) {
            return *it;
        }
    }
    return -1;
}

int MarkerManager::previousMarker(int line) {
    std::vector<int> list = listMarkerIntern();
    auto it = std::lower_bound(list.begin(), list.end(), line);
    if (it != list.begin()) {
        if (*(it - 1) < line) {
            return *(it - 1);
        } else {
            if(!list.empty()) {
                return list.back();
            }
        }
    } else {
        if (!list.empty()) {
            return list.back();
        }
    }
    return -1;
}

void MarkerManager::clearMarkers() {
    markers.clear();
}

MarkerManager::~MarkerManager() {
    clearMarkers();
}
