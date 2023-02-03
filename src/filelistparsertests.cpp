// SPDX-License-Identifier: BSL-1.0

#include "catchwrapper.h"
#include <QStringList>

#include "filelistparser.h"

TEST_CASE("parseFileList") {
    SECTION("empty") {
        QStringList args = {""};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 1);
        CHECK(fle[0].fileName == "");
        CHECK(fle[0].pos == "");
    }
    SECTION("datei") {
        QStringList args = {"datei"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 1);
        CHECK(fle[0].fileName == "datei");
        CHECK(fle[0].pos == "");
    }
    SECTION("+pos datei") {
        QStringList args = {"+0","dateiA"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 1);
        CHECK(fle[0].fileName == "dateiA");
        CHECK(fle[0].pos == "+0");
    }
    SECTION("dateiA dateiB") {
        QStringList args = {"dateiA","dateiB"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 2);
        CHECK(fle[0].fileName == "dateiA");
        CHECK(fle[0].pos == "");
        CHECK(fle[1].fileName == "dateiB");
        CHECK(fle[1].pos == "");
    }
    SECTION("+pos dateiA +pos dateiB") {
        QStringList args = {"+0","dateiA","+55","dateiB"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 2);
        CHECK(fle[0].fileName == "dateiA");
        CHECK(fle[0].pos == "+0");
        CHECK(fle[1].fileName == "dateiB");
        CHECK(fle[1].pos == "+55");
    }
    SECTION("+pos dateiA dateiB") {
        QStringList args = {"+0","dateiA","dateiB"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 2);
        CHECK(fle[0].fileName == "dateiA");
        CHECK(fle[0].pos == "+0");
        CHECK(fle[1].fileName == "dateiB");
        CHECK(fle[1].pos == "");
    }
    SECTION("dateiA +pos dateiB") {
        QStringList args = {"dateiA","+0","dateiB"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 2);
        CHECK(fle[0].fileName == "dateiA");
        CHECK(fle[0].pos == "");
        CHECK(fle[1].fileName == "dateiB");
        CHECK(fle[1].pos == "+0");
    }
    SECTION("dateiA +pos") {
        QStringList args = {"dateiA","+0",};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 1);
        CHECK(fle[0].fileName == "dateiA");
        CHECK(fle[0].pos == "+0");
    }
    SECTION("dateiA dateiB +pos") {
        QStringList args = {"dateiA","dateiB","+0"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 2);
        CHECK(fle[0].fileName == "dateiA");
        CHECK(fle[0].pos == "");
        CHECK(fle[1].fileName == "dateiB");
        CHECK(fle[1].pos == "+0");
    }

    // ERROR
    SECTION("+pos") {
        QStringList args = {"+0"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 0);
    }
    SECTION("datei +pos datei +pos") {
        QStringList args = {"dateiA","+0","dateiB","+0"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 0);
    }
    SECTION("+pos +pos datei datei") {
        QStringList args = {"+0","+0","dateiA","dateiB"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 0);
    }
    SECTION("datei datei +pos +pos") {
        QStringList args = {"dateiA","dateiB","+0","+0"};
        QVector<FileListEntry> fle = parseFileList(args);
        REQUIRE(fle.size() == 0);
    }
}
