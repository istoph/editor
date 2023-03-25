// SPDX-License-Identifier: BSL-1.0

#include "document.h"

#include "documenttests.h"

#include "catchwrapper.h"

#include <Tui/ZRoot.h>
#include <Tui/ZTest.h>

#include <Tui/ZTerminal.h>
#include <Tui/ZTextMetrics.h>


TEST_CASE("Document") {
    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);

    Document doc;

    TextCursor cursor{&doc, nullptr, [&terminal,&doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.getLines()[line]);
            lay.doLayout(65000);
            return lay;
        }
    };

    SECTION("insert nonewline") {
        doc.setNoNewline(true);

        cursor.insertText("test\ntest");

        REQUIRE(doc.getLines().size() == 2);
        CHECK(doc.getLines()[0] == "test");
        CHECK(doc.getLines()[1] == "test");
    }
    SECTION("insert and remove nonewline") {
        doc.setNoNewline(true);
        cursor.insertText("\n");
        REQUIRE(doc.getLines().size() == 1);
        CHECK(doc.noNewLine() == false);
    }

    SECTION("inser-empty") {
        cursor.insertText("");
        REQUIRE(doc.getLines().size() == 1);
        CHECK(cursor.position() == TextCursor::Position{0,0});
    }

    SECTION("inser-empty-line-and-text") {
        cursor.insertText("\ntest test");
        REQUIRE(doc.getLines().size() == 2);
        CHECK(cursor.position() == TextCursor::Position{9,1});
    }

    SECTION("inser-lines") {
        cursor.insertText("test test");
        REQUIRE(doc.getLines().size() == 1);
        CHECK(cursor.position() == TextCursor::Position{9,0});
        cursor.selectAll();

        cursor.insertText("test test\ntest test");
        REQUIRE(doc.getLines().size() == 2);
        CHECK(cursor.position() == TextCursor::Position{9,1});
        cursor.selectAll();

        cursor.insertText("test test\ntest test\ntest test\n");
        REQUIRE(doc.getLines().size() == 4);
        CHECK(cursor.position() == TextCursor::Position{0,3});
        cursor.selectAll();

        cursor.insertText("test test\ntest test\n");
        cursor.insertText("test test\ntest test");
        REQUIRE(doc.getLines().size() == 4);
        CHECK(cursor.position() == TextCursor::Position{9,3});
        cursor.selectAll();
    }

    SECTION("inser-and-selection") {
        cursor.insertText("test test");
        REQUIRE(doc.getLines().size() == 1);
        CHECK(cursor.position() == TextCursor::Position{9,0});
        cursor.moveWordLeft(true);
        CHECK(cursor.position() == TextCursor::Position{5,0});
        cursor.insertText("new new");

        QVector<QString> test;
        test.append("test new new");
        CHECK(cursor.position() == TextCursor::Position{12,0});
        CHECK(doc.getLines() == test);

        cursor.moveWordLeft(true);
        cursor.moveWordLeft(true);
        cursor.moveWordLeft(true);
        CHECK(cursor.position() == TextCursor::Position{0,0});
        cursor.insertText("\nold");
    }

    SECTION("TextCursor-Position-without-text") {

        CHECK(cursor.position() == TextCursor::Position{0,0});

        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        //check the original after delete any text
        bool afterDeletetText = GENERATE(false, true);
        CAPTURE(afterDeletetText);
        if (afterDeletetText) {
            cursor.insertText("test test\ntest test\ntest test\n");
            REQUIRE(doc.getLines().size() == 4);
            cursor.selectAll();
            cursor.removeSelectedText();
            REQUIRE(doc.getLines().size() == 1);
        }

        cursor.moveCharacterLeft(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});
        cursor.moveWordLeft(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});
        cursor.moveToStartOfLine(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});

        cursor.moveCharacterRight(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});
        cursor.moveWordRight(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});
        cursor.moveToEndOfLine(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});

        cursor.moveToStartOfDocument(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});
        cursor.moveToStartOfDocument(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});

        cursor.moveUp();
        CHECK(cursor.position() == TextCursor::Position{0,0});
        cursor.moveDown();
        CHECK(cursor.position() == TextCursor::Position{0,0});

        cursor.setPosition({3,2}, selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});
    }

    SECTION("TextCursor-Position-with-text") {

        CHECK(cursor.position() == TextCursor::Position{0,0});

        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest test");
        REQUIRE(doc.getLines().size() == 3);
        CHECK(cursor.position() == TextCursor::Position{9,2});


        cursor.moveCharacterLeft(selection);
        CHECK(cursor.position() == TextCursor::Position{8,2});
        cursor.moveWordLeft(selection);
        CHECK(cursor.position() == TextCursor::Position{5,2});
        cursor.moveToStartOfLine(selection);
        CHECK(cursor.position() == TextCursor::Position{0,2});

        cursor.moveCharacterRight(selection);
        CHECK(cursor.position() == TextCursor::Position{1,2});
        cursor.moveWordRight(selection);
        CHECK(cursor.position() == TextCursor::Position{4,2});
        cursor.moveToEndOfLine(selection);
        CHECK(cursor.position() == TextCursor::Position{9,2});

        cursor.moveToStartOfDocument(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});
        cursor.moveToEndOfDocument(selection);
        CHECK(cursor.position() == TextCursor::Position{9,2});

        cursor.moveUp();
        CHECK(cursor.position() == TextCursor::Position{9,1});
        cursor.moveDown();
        CHECK(cursor.position() == TextCursor::Position{9,2});

        cursor.setPosition({3,2}, selection);
        CHECK(cursor.position() == TextCursor::Position{3,2});
    }

    SECTION("move-up") {
        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest test");
        cursor.setPosition({0,1});
        cursor.moveUp(selection);
        CHECK(cursor.position() == TextCursor::Position{0,0});
    }

    SECTION("move-up-position1-1") {
        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest test");
        cursor.setPosition({1,1});
        cursor.moveUp(selection);
        CHECK(cursor.position() == TextCursor::Position{1,0});
    }

    SECTION("move-down") {
        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest test");
        cursor.setPosition({0,1});
        cursor.moveDown(selection);
        CHECK(cursor.position() == TextCursor::Position{0,2});
    }

    SECTION("move-down-notext") {
        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest");
        cursor.setPosition({8,1});
        cursor.moveDown(selection);
        CHECK(cursor.position() == TextCursor::Position{4,2});
    }

    SECTION("delete") {
        cursor.insertText("test test\ntest test\ntest test");
        REQUIRE(doc.getLines().size() == 3);
        REQUIRE(doc.getLines()[0].size() == 9);
        REQUIRE(doc.getLines()[1].size() == 9);
        REQUIRE(doc.getLines()[2].size() == 9);
        CHECK(cursor.position() == TextCursor::Position{9,2});

        cursor.deletePreviousCharacter();
        REQUIRE(doc.getLines().size() == 3);
        REQUIRE(doc.getLines()[0].size() == 9);
        REQUIRE(doc.getLines()[1].size() == 9);
        REQUIRE(doc.getLines()[2].size() == 8);
        CHECK(cursor.position() == TextCursor::Position{8,2});
        cursor.deletePreviousWord();
        REQUIRE(doc.getLines().size() == 3);
        REQUIRE(doc.getLines()[0].size() == 9);
        REQUIRE(doc.getLines()[1].size() == 9);
        REQUIRE(doc.getLines()[2].size() == 5);
        CHECK(cursor.position() == TextCursor::Position{5,2});

        cursor.setPosition({0,1});

        cursor.deleteCharacter();
        CHECK(cursor.position() == TextCursor::Position{0,1});
        CHECK(doc.getLines().size() == 3);
        CHECK(doc.getLines()[0].size() == 9);
        CHECK(doc.getLines()[1].size() == 8);
        CHECK(doc.getLines()[2].size() == 5);

        cursor.deleteWord();
        CHECK(cursor.position() == TextCursor::Position{0,1});
        CHECK(doc.getLines().size() == 3);
        CHECK(doc.getLines()[0].size() == 9);
        CHECK(doc.getLines()[1].size() == 5);
        CHECK(doc.getLines()[2].size() == 5);
    }

    SECTION("delete-newline") {
        cursor.insertText("\n\n\n\n\n\n");
        for (int i = 6; i > 0; i--) {
            CAPTURE(i);
            cursor.deletePreviousCharacter();
            CHECK(cursor.position() == TextCursor::Position{0,i - 1});
        }

        cursor.insertText("\n\n\n\n\n\n");
        CHECK(cursor.position() == TextCursor::Position{0,6});
        CHECK(doc.getLines().size() == 7);

        cursor.deletePreviousCharacter();
        CHECK(cursor.position() == TextCursor::Position{0,5});
        CHECK(doc.getLines().size() == 6);

        cursor.deletePreviousWord();
        CHECK(cursor.position() == TextCursor::Position{0,4});
        CHECK(doc.getLines().size() == 5);

        cursor.setPosition({0,2});

        cursor.deleteCharacter();
        CHECK(cursor.position() == TextCursor::Position{0,2});
        CHECK(doc.getLines().size() == 4);

        cursor.deleteWord();
        CHECK(cursor.position() == TextCursor::Position{0,2});
        CHECK(doc.getLines().size() == 3);
    }

    SECTION("delete-word") {
        cursor.insertText("a bb  ccc   dddd     eeeee");
        cursor.deletePreviousWord();
        CHECK(cursor.position() == TextCursor::Position{21,0});
        cursor.deletePreviousWord();
        CHECK(cursor.position() == TextCursor::Position{12,0});
        cursor.deletePreviousWord();
        CHECK(cursor.position() == TextCursor::Position{6,0});
        cursor.deletePreviousWord();
        CHECK(cursor.position() == TextCursor::Position{2,0});
        cursor.deletePreviousWord();
        CHECK(cursor.position() == TextCursor::Position{0,0});

        cursor.insertText("a bb  ccc   dddd     eeeee");
        cursor.setPosition({7,0});
        cursor.deletePreviousWord();
        CHECK(cursor.position() == TextCursor::Position{6,0});

        cursor.selectAll();
        cursor.insertText("a bb  ccc   dddd     eeeee");
        cursor.setPosition({8,0});
        cursor.deletePreviousWord();
        CHECK(cursor.position() == TextCursor::Position{6,0});

        cursor.selectAll();
        cursor.insertText("a bb  ccc   dddd     eeeee");
        cursor.setPosition({0,0});
        CHECK(doc.getLines()[0].size() == 26);
        cursor.deleteWord();
        CHECK(doc.getLines()[0].size() == 25);
        cursor.deleteWord();
        CHECK(doc.getLines()[0].size() == 22);
        cursor.deleteWord();
        CHECK(doc.getLines()[0].size() == 17);
        cursor.deleteWord();
        CHECK(doc.getLines()[0].size() == 10);
        cursor.deleteWord();
        CHECK(doc.getLines()[0].size() == 0);
    }

    SECTION("delete-line") {
        bool emptyline = GENERATE(true, false);
        if(emptyline) {
            cursor.insertText("test test\n\ntest test");
            cursor.setPosition({0,1});
            CHECK(cursor.position() == TextCursor::Position{0,1});
        } else {
            cursor.insertText("test test\nremove remove\ntest test");
            cursor.setPosition({4,0});
            cursor.moveDown(true);
            cursor.moveWordRight(true);
            CHECK(cursor.position() == TextCursor::Position{6,1});
        }
        REQUIRE(doc.getLines().size() == 3);

        cursor.deleteLine();
        CAPTURE(doc.getLines());
        CHECK(cursor.position() == TextCursor::Position{0,1});
        CHECK(doc.getLines().size() == 2);

        QVector<QString> text;
        text.append("test test");
        text.append("test test");

        CHECK(doc.getLines() == text);
    }

    SECTION("delete-fist-line") {
        bool emptyline = GENERATE(true, false);
        CAPTURE(emptyline);
        if(emptyline) {
            cursor.insertText("\ntest test\ntest test");
            cursor.setPosition({0,0});
            CHECK(cursor.position() == TextCursor::Position{0,0});
        } else {
            cursor.insertText("remove remove\ntest test\ntest test");
            cursor.setPosition({0,1});
            cursor.moveUp(true);
            CHECK(cursor.position() == TextCursor::Position{0,0});
        }
        REQUIRE(doc.getLines().size() == 3);
        cursor.deleteLine();
        CAPTURE(doc.getLines());
        CHECK(cursor.position() == TextCursor::Position{0,0});
        CHECK(doc.getLines().size() == 2);
        CHECK(cursor.hasSelection() == false);

        QVector<QString> text;
        text.append("test test");
        text.append("test test");

        CHECK(doc.getLines() == text);
    }

    SECTION("delete-last-line") {
        bool emptyline = GENERATE(true, false);
        CAPTURE(emptyline);
        if(emptyline) {
            cursor.insertText("test test\ntest test\n");
            cursor.setPosition({0,2});
            CHECK(cursor.position() == TextCursor::Position{0,2});
        } else {
            cursor.insertText("test test\ntest test\nremove remove");
            cursor.setPosition({0,2});
            CHECK(cursor.position() == TextCursor::Position{0,2});
        }
        REQUIRE(doc.getLines().size() == 3);
        cursor.deleteLine();
        CAPTURE(doc.getLines());
        CHECK(cursor.position() == TextCursor::Position{9,1});
        CHECK(doc.getLines().size() == 2);
        CHECK(cursor.hasSelection() == false);

        QVector<QString> text;
        text.append("test test");
        text.append("test test");

        CHECK(doc.getLines() == text);
    }

    SECTION("hasSelection-and-pos") {
        cursor.insertText("");
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());

        cursor.moveCharacterLeft(true);
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());

        cursor.moveCharacterRight(true);
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());

        cursor.insertText(" ");
        cursor.moveCharacterLeft(true);
        CHECK(cursor.hasSelection() == true);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == TextCursor::Position{1,0});

        cursor.moveCharacterRight(true);
        CHECK(cursor.hasSelection() == true); //fix false
        //der bug führt auch dazu das backspace 2x gedrückt werden muss
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());

        cursor.moveCharacterLeft(true);
        CHECK(cursor.hasSelection() == true);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == TextCursor::Position{1,0});

        cursor.moveCharacterRight(true);
        CHECK(cursor.hasSelection() == true); //fix false
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());

        cursor.selectAll();
        CHECK(cursor.hasSelection() == true);
        cursor.insertText(" ");
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());
    }

    SECTION("selectAll") {
        cursor.selectAll();
        CHECK(cursor.hasSelection() == true); //fix false
        CHECK(cursor.selectedText() == "");

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectedText() == "");

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectedText() == "");

        cursor.insertText(" ");
        CHECK(cursor.selectedText() == "");
        CHECK(cursor.hasSelection() == false);

        cursor.selectAll();
        CHECK(cursor.selectedText() == " ");
        CHECK(cursor.hasSelection() == true);

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectedText() == "");

        cursor.moveCharacterLeft();

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectedText() == "");

        cursor.selectAll();
        cursor.insertText("\n");
        CHECK(cursor.selectedText() == "");
        CHECK(cursor.hasSelection() == false);

        cursor.selectAll();
        CHECK(cursor.selectedText() == "\n");
        CHECK(cursor.hasSelection() == true);
    }

    SECTION("removeSelectedText") {
        CHECK(cursor.hasSelection() == false);
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 0);

        cursor.selectAll();
        CHECK(cursor.hasSelection() == true); //fix false
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 0);

        cursor.insertText(" ");
        cursor.selectAll();
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 0);

        cursor.insertText("\n");
        CHECK(doc.getLines().size() == 2);
        cursor.selectAll();
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 0);

        cursor.insertText("aRemovEb");
        cursor.moveCharacterLeft();
        for(int i=0; i < 6; i++) {
            cursor.moveCharacterLeft(true);
        }
        CHECK(cursor.selectedText() == "RemovE");
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 2);

        //clear
        cursor.selectAll();
        cursor.removeSelectedText();

        cursor.insertText("RemovEb");
        cursor.moveCharacterLeft();
        for(int i=0; i < 6; i++) {
            cursor.moveCharacterLeft(true);
        }
        CHECK(cursor.selectedText() == "RemovE");
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 1);

        //clear
        cursor.selectAll();
        cursor.removeSelectedText();

        cursor.insertText("aRemovE");
        for(int i=0; i < 6; i++) {
            cursor.moveCharacterLeft(true);
        }
        CHECK(cursor.selectedText() == "RemovE");
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 1);

        //clear
        cursor.selectAll();
        cursor.removeSelectedText();

        cursor.insertText("aRem\novEb");
        cursor.moveCharacterLeft();
        for(int i=0; i < 7; i++) {
            cursor.moveCharacterLeft(true);
        }
        CHECK(cursor.selectedText() == "Rem\novE");
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 2);

        //clear
        cursor.selectAll();
        cursor.removeSelectedText();

        cursor.insertText("aRem\novE\n");
        for(int i=0; i < 8; i++) {
            cursor.moveCharacterLeft(true);
        }
        CHECK(cursor.selectedText() == "Rem\novE\n");
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 1);
        CHECK(doc.getLines()[0] == "a");

        //clear
        cursor.selectAll();
        cursor.removeSelectedText();

        cursor.insertText("\nRem\novEb");
        cursor.moveCharacterLeft();
        for(int i=0; i < 8; i++) {
            cursor.moveCharacterLeft(true);
        }
        CHECK(cursor.selectedText() == "\nRem\novE");
        cursor.removeSelectedText();
        CHECK(doc.getLines().size() == 1);
        CHECK(doc.getLines()[0].size() == 1);
        CHECK(doc.getLines()[0] == "b");
    }

    SECTION("at") {
        CHECK(cursor.atStart() == true);
        CHECK(cursor.atEnd() == true);
        CHECK(cursor.atLineStart() == true);
        CHECK(cursor.atLineEnd() == true);

        cursor.insertText(" ");
        CHECK(cursor.atStart() == false);
        CHECK(cursor.atEnd() == true);
        CHECK(cursor.atLineStart() == false);
        CHECK(cursor.atLineEnd() == true);

        cursor.insertText("\n");
        CHECK(cursor.atStart() == false);
        CHECK(cursor.atEnd() == true);
        CHECK(cursor.atLineStart() == true);
        CHECK(cursor.atLineEnd() == true);

        cursor.insertText(" ");
        CHECK(cursor.atStart() == false);
        CHECK(cursor.atEnd() == true);
        CHECK(cursor.atLineStart() == false);
        CHECK(cursor.atLineEnd() == true);

        cursor.moveCharacterLeft();
        CHECK(cursor.atStart() == false);
        CHECK(cursor.atEnd() == false);
        CHECK(cursor.atLineStart() == true);
        CHECK(cursor.atLineEnd() == false);

        cursor.moveToStartOfDocument();
        CHECK(cursor.atStart() == true);
        CHECK(cursor.atEnd() == false);
        CHECK(cursor.atLineStart() == true);
        CHECK(cursor.atLineEnd() == false);

        //clear
        cursor.selectAll();
        cursor.insertText("\n");
        CHECK(cursor.atStart() == false);
        CHECK(cursor.atEnd() == true);
        CHECK(cursor.atLineStart() == true);
        CHECK(cursor.atLineEnd() == true);

        cursor.moveUp();
        CHECK(cursor.atStart() == true);
        CHECK(cursor.atEnd() == false);
        CHECK(cursor.atLineStart() == true);
        CHECK(cursor.atLineEnd() == true);

    }
}
