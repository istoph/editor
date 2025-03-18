// SPDX-License-Identifier: BSL-1.0

#ifndef MARKERMANAGER_H
#define MARKERMANAGER_H

#include <QList>
#include <Tui/ZDocumentLineMarker.h>
#include <Tui/ZDocument.h>

class MarkerManager
{

public:
    explicit MarkerManager();
    void addMarker(Tui::ZDocument *doc, int line);
    void removeMarker(int line);
    bool hasMarker();
    bool hasMarker(int line);
    QList<int> listMarker();
    int nextMarker(int line);
    int previousMarker(int line);
    void clearMarkers();
    ~MarkerManager();

private:
    std::vector<int> listMarkerIntern();
    std::vector<Tui::ZDocumentLineMarker> markers;
};

#endif // MARKERMANAGER_H
