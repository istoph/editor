// SPDX-License-Identifier: BSL-1.0

#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include <QList>

#include "Tui/ZDocumentCursor.h"
#include "qjsonobject.h"
#include <Tui/ZDocumentCursor.h>

class Attributes
{
public:
    Attributes();
    Attributes(QString attributesFile);

    bool readAttributes();
    Tui::ZDocumentCursor::Position getAttributesCursorPosition(QString filename);
    bool writeAttributes(QString filename, Tui::ZDocumentCursor::Position cursorPosition, int scrollPositionColumn, int scrollPositionLine, int scrollPositionFineLine, QList<int> lineMarkers);
    void setAttributesFile(QString attributesFile);
    QString attributesFile();

    int getAttributesScrollCol(QString filename);
    int getAttributesScrollLine(QString filename);
    int getAttributesScrollFine(QString filename);
    QList<int> getAttributesLineMarker(QString filename);

private:
    bool _loaded = false;
    QJsonObject _attributeObject;
    QString _attributesFile;
};

#endif // ATTRIBUTES_H
