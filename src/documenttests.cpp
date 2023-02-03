// SPDX-License-Identifier: BSL-1.0

#include "catchwrapper.h"
#include "document.h"
#include "file.h"
#include <Tui/ZRoot.h>
#include <Tui/ZTest.h>

#include <Tui/ZTerminal.h>

#include "documenttests.h"

TEST_CASE("Document") {
    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);
    DocumentTestHelper t;

    File file(&root);
    Document &doc = t.getDoc(&file);
    TextCursor cursor{&doc, &file};

    SECTION("insert nonewline") {
        doc._nonewline = true;

        cursor.insertText("test\ntest");

        REQUIRE(doc._text.size() == 2);
        CHECK(doc._text[0] == "test");
        CHECK(doc._text[1] == "test");
    }
    SECTION("insert and remove nonewline") {
        doc._nonewline = true;
        //cursor.insertText("\n");
        cursor.insertLineBreakAt({0, 0});
        REQUIRE(doc._text.size() == 1);
        CHECK(doc._nonewline == false);
    }
//    SECTION("insert tab-as-space") {
//        doc._nonewline = true;
//        file.setTabOption(true);
//        file.setTabsize(4);
//        file.setEatSpaceBeforeTabs(false);
//        cursor.insertText("\t");
//
//        REQUIRE(doc._text.size() == 1);
//        CHECK(doc._text[0] == "    ");
//        CHECK(file.getCursorPosition() == QPoint{4,0});
//    }
    SECTION("insert tab-as-tab") {
        doc._nonewline = true;
        file.setTabOption(false);
        file.setTabsize(4);
        file.setEatSpaceBeforeTabs(false);
        cursor.insertText("\t");

        REQUIRE(doc._text.size() == 1);
        CHECK(doc._text[0] == "\t");
        CHECK(file.getCursorPosition() == QPoint{1,0});
    }
//    SECTION("insert tab-no-eat-space") {
//        doc._nonewline = true;
//        file.setTabOption(true);
//        file.setTabsize(4);
//        file.setEatSpaceBeforeTabs(false);
//        cursor.insertText("  aa");
//        file.setCursorPosition({0,0});
//        cursor.insertText("\t");
//
//        REQUIRE(doc._text.size() == 1);
//        CHECK(doc._text[0] == "      aa");
//        CHECK(file.getCursorPosition() == QPoint{4,0});
//    }
//    SECTION("insert tab-eat-space") {
//        doc._nonewline = true;
//        file.setTabOption(true);
//        file.setTabsize(4);
//        file.setEatSpaceBeforeTabs(true);
//        cursor.insertText("  aa");
//        file.setCursorPosition({0,0});
//        cursor.insertText("\t");
//
//        REQUIRE(doc._text.size() == 1);
//        CHECK(doc._text[0] == "    aa");
//        CHECK(file.getCursorPosition() == QPoint{4,0});
//    }
}
