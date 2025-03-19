// SPDX-License-Identifier: BSL-1.0

#include "catchwrapper.h"

#include "../attributes.h"

#include <Tui/ZTerminal.h>
#include <Tui/ZDocumentCursor.h>

#include <QList>
#include <QFile>
#include <QTemporaryDir>

const QString attributesFileName = "attributes.file";
void cleanAttributesFileName() {
    QFile *file = new QFile(attributesFileName);
    file->remove();
}

TEST_CASE("attributes") {
    SECTION("empty") {
        Attributes *a = new Attributes("");
        CHECK(a->attributesFile() == "");
    }

    SECTION("attributes-in-out") {
        QTemporaryDir dir;
        QString file;

        QList<int> list;
        list.append(0);
        list.append(1);
        list.append(65535);

        struct TestCase { QString filename;
            int cursorPositionX;
            int cursorPositionY;
            int scrollPositionColumn;
            int scrollPositionLine;
            int scrollPositionFineLine; };
        auto testCase = GENERATE( TestCase{"/file1", 0, 0, 0, 0, 0},
                                  TestCase{"/file2", 1, 2, 3, 4, 5},
                                  TestCase{"/file3", 65535, 65534, 65533, 65532, 65531},
                                  TestCase{"/file1", 65535, 65535, 65535, 65535, 65535}
                                 );

        file = dir.path() + testCase.filename;
        QFile datei(file);
        if (datei.open(QIODevice::WriteOnly)) {
            datei.close();
        }

        Attributes *a = new Attributes(attributesFileName);
        a->writeAttributes(file,
                           {testCase.cursorPositionX, testCase.cursorPositionY},
                           testCase.scrollPositionColumn,
                           testCase.scrollPositionLine,
                           testCase.scrollPositionFineLine,
                           list);

        CHECK(a->getAttributesCursorPosition(file).codeUnit == testCase.cursorPositionX);
        CHECK(a->getAttributesCursorPosition(file).line == testCase.cursorPositionY);
        CHECK(a->getAttributesScrollCol(file) == testCase.scrollPositionColumn);
        CHECK(a->getAttributesScrollLine(file) == testCase.scrollPositionLine);
        CHECK(a->getAttributesScrollFine(file) == testCase.scrollPositionFineLine);
        CHECK(a->getAttributesLineMarker(file) == list);

        cleanAttributesFileName();
    }
}
