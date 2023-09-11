// SPDX-License-Identifier: BSL-1.0

#include "document.h"

#include "documenttests.h"

#include "catchwrapper.h"

#include <Tui/ZRoot.h>
#include <Tui/ZTest.h>

#include <Tui/ZTerminal.h>
#include <Tui/ZTextMetrics.h>

static QVector<QString> docToVec(const Document &doc) {
    QVector<QString> ret;

    for (int i = 0; i < doc.lineCount(); i++) {
        ret.append(doc.line(i));
    }

    return ret;
}


TEST_CASE("Document") {
    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);

    Document doc;

    TextCursor cursor{&doc, nullptr, [&terminal,&doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    SECTION("insert nonewline") {
        doc.setNoNewline(true);

        cursor.insertText("test\ntest");

        REQUIRE(doc.lineCount() == 2);
        CHECK(doc.line(0) == "test");
        CHECK(doc.line(1) == "test");
    }
    SECTION("insert and remove nonewline") {
        doc.setNoNewline(true);
        cursor.insertText("\n");
        REQUIRE(doc.lineCount() == 1);
        CHECK(doc.noNewLine() == false);
    }

    SECTION("inser-empty") {
        cursor.insertText("");
        REQUIRE(doc.lineCount() == 1);
        CHECK(cursor.position() == TextCursor::Position{0,0});
    }

    SECTION("inser-empty-line-and-text") {
        cursor.insertText("\ntest test");
        REQUIRE(doc.lineCount() == 2);
        CHECK(cursor.position() == TextCursor::Position{9,1});
    }

    SECTION("inser-lines") {
        cursor.insertText("test test");
        REQUIRE(doc.lineCount() == 1);
        CHECK(cursor.position() == TextCursor::Position{9,0});
        cursor.selectAll();

        cursor.insertText("test test\ntest test");
        REQUIRE(doc.lineCount() == 2);
        CHECK(cursor.position() == TextCursor::Position{9,1});
        cursor.selectAll();

        cursor.insertText("test test\ntest test\ntest test\n");
        REQUIRE(doc.lineCount() == 4);
        CHECK(cursor.position() == TextCursor::Position{0,3});
        cursor.selectAll();

        cursor.insertText("test test\ntest test\n");
        cursor.insertText("test test\ntest test");
        REQUIRE(doc.lineCount() == 4);
        CHECK(cursor.position() == TextCursor::Position{9,3});
        cursor.selectAll();
    }

    SECTION("inser-and-selection") {
        cursor.insertText("test test");
        REQUIRE(doc.lineCount() == 1);
        CHECK(cursor.position() == TextCursor::Position{9,0});
        cursor.moveWordLeft(true);
        CHECK(cursor.position() == TextCursor::Position{5,0});
        cursor.insertText("new new");

        QVector<QString> test;
        test.append("test new new");
        CHECK(cursor.position() == TextCursor::Position{12,0});
        CHECK(docToVec(doc) == test);

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
            REQUIRE(doc.lineCount() == 4);
            cursor.selectAll();
            cursor.removeSelectedText();
            REQUIRE(doc.lineCount() == 1);
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
        REQUIRE(doc.lineCount() == 3);
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
        REQUIRE(doc.lineCount() == 3);
        REQUIRE(doc.lineCodeUnits(0) == 9);
        REQUIRE(doc.lineCodeUnits(1) == 9);
        REQUIRE(doc.lineCodeUnits(2) == 9);
        CHECK(cursor.position() == TextCursor::Position{9,2});

        cursor.deletePreviousCharacter();
        REQUIRE(doc.lineCount() == 3);
        REQUIRE(doc.lineCodeUnits(0) == 9);
        REQUIRE(doc.lineCodeUnits(1) == 9);
        REQUIRE(doc.lineCodeUnits(2) == 8);
        CHECK(cursor.position() == TextCursor::Position{8,2});
        cursor.deletePreviousWord();
        REQUIRE(doc.lineCount() == 3);
        REQUIRE(doc.lineCodeUnits(0) == 9);
        REQUIRE(doc.lineCodeUnits(1) == 9);
        REQUIRE(doc.lineCodeUnits(2) == 5);
        CHECK(cursor.position() == TextCursor::Position{5,2});

        cursor.setPosition({0,1});

        cursor.deleteCharacter();
        CHECK(cursor.position() == TextCursor::Position{0,1});
        CHECK(doc.lineCount() == 3);
        CHECK(doc.lineCodeUnits(0) == 9);
        CHECK(doc.lineCodeUnits(1) == 8);
        CHECK(doc.lineCodeUnits(2) == 5);

        cursor.deleteWord();
        CHECK(cursor.position() == TextCursor::Position{0,1});
        CHECK(doc.lineCount() == 3);
        CHECK(doc.lineCodeUnits(0) == 9);
        CHECK(doc.lineCodeUnits(1) == 5);
        CHECK(doc.lineCodeUnits(2) == 5);
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
        CHECK(doc.lineCount() == 7);

        cursor.deletePreviousCharacter();
        CHECK(cursor.position() == TextCursor::Position{0,5});
        CHECK(doc.lineCount() == 6);

        cursor.deletePreviousWord();
        CHECK(cursor.position() == TextCursor::Position{0,4});
        CHECK(doc.lineCount() == 5);

        cursor.setPosition({0,2});

        cursor.deleteCharacter();
        CHECK(cursor.position() == TextCursor::Position{0,2});
        CHECK(doc.lineCount() == 4);

        cursor.deleteWord();
        CHECK(cursor.position() == TextCursor::Position{0,2});
        CHECK(doc.lineCount() == 3);
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
        CHECK(doc.lineCodeUnits(0) == 26);
        cursor.deleteWord();
        CHECK(doc.lineCodeUnits(0) == 25);
        cursor.deleteWord();
        CHECK(doc.lineCodeUnits(0) == 22);
        cursor.deleteWord();
        CHECK(doc.lineCodeUnits(0) == 17);
        cursor.deleteWord();
        CHECK(doc.lineCodeUnits(0) == 10);
        cursor.deleteWord();
        CHECK(doc.lineCodeUnits(0) == 0);
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
        REQUIRE(doc.lineCount() == 3);

        cursor.deleteLine();
        CAPTURE(docToVec(doc));
        CHECK(cursor.position() == TextCursor::Position{0,1});
        CHECK(doc.lineCount() == 2);

        QVector<QString> text;
        text.append("test test");
        text.append("test test");

        CHECK(docToVec(doc) == text);
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
        REQUIRE(doc.lineCount() == 3);
        cursor.deleteLine();
        CAPTURE(docToVec(doc));
        CHECK(cursor.position() == TextCursor::Position{0,0});
        CHECK(doc.lineCount() == 2);
        CHECK(cursor.hasSelection() == false);

        QVector<QString> text;
        text.append("test test");
        text.append("test test");

        CHECK(docToVec(doc) == text);
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
        REQUIRE(doc.lineCount() == 3);
        cursor.deleteLine();
        CAPTURE(docToVec(doc));
        CHECK(cursor.position() == TextCursor::Position{9,1});
        CHECK(doc.lineCount() == 2);
        CHECK(cursor.hasSelection() == false);

        QVector<QString> text;
        text.append("test test");
        text.append("test test");

        CHECK(docToVec(doc) == text);
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
        CHECK(cursor.hasSelection() == false);
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
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());

        cursor.selectAll();
        CHECK(cursor.hasSelection() == true);
        cursor.insertText(" ");
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());
    }

    SECTION("no-selection") {
        cursor.insertText("abc");
        cursor.moveCharacterLeft();
        cursor.moveCharacterLeft(true);
        cursor.moveCharacterRight(true);
        CHECK(cursor.hasSelection() == false);
        cursor.deletePreviousCharacter();
        CHECK(doc.line(0) == "ac");
    }

    SECTION("selectAll") {
        cursor.selectAll();
        CHECK(cursor.hasSelection() == false);
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
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 0);

        cursor.selectAll();
        CHECK(cursor.hasSelection() == false);
        cursor.removeSelectedText();
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 0);

        cursor.insertText(" ");
        cursor.selectAll();
        cursor.removeSelectedText();
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 0);

        cursor.insertText("\n");
        CHECK(doc.lineCount() == 2);
        cursor.selectAll();
        cursor.removeSelectedText();
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 0);

        cursor.insertText("aRemovEb");
        cursor.moveCharacterLeft();
        for(int i=0; i < 6; i++) {
            cursor.moveCharacterLeft(true);
        }
        CHECK(cursor.selectedText() == "RemovE");
        cursor.removeSelectedText();
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 2);

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
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 1);

        //clear
        cursor.selectAll();
        cursor.removeSelectedText();

        cursor.insertText("aRemovE");
        for(int i=0; i < 6; i++) {
            cursor.moveCharacterLeft(true);
        }
        CHECK(cursor.selectedText() == "RemovE");
        cursor.removeSelectedText();
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 1);

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
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 2);

        //clear
        cursor.selectAll();
        cursor.removeSelectedText();

        cursor.insertText("aRem\novE\n");
        for(int i=0; i < 8; i++) {
            cursor.moveCharacterLeft(true);
        }
        CHECK(cursor.selectedText() == "Rem\novE\n");
        cursor.removeSelectedText();
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 1);
        CHECK(doc.line(0) == "a");

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
        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 1);
        CHECK(doc.line(0) == "b");
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

TEST_CASE("Cursor") {
    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);

    Document doc;

    TextCursor cursor1{&doc, nullptr, [&terminal,&doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };
    cursor1.insertText(" \n \n \n \n");

    TextCursor cursor2{&doc, nullptr, [&terminal,&doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    CHECK(cursor1.position() == TextCursor::Position{0,4});
    CHECK(cursor2.position() == TextCursor::Position{0,0});

    SECTION("delete-previous-line") {
        cursor2.setPosition({0,3});
        CHECK(cursor2.position() == TextCursor::Position{0,3});
        CHECK(doc.lineCount() == 5);
        cursor2.deleteCharacter();
        cursor2.deleteCharacter();
        CHECK(cursor2.position() == TextCursor::Position{0,3});
        CHECK(cursor1.position() == TextCursor::Position{0,3});
        CHECK(doc.lineCount() == 4);
    }
    SECTION("delete-on-cursor") {
        cursor1.setPosition({0,2});
        cursor2.setPosition({0,2});
        cursor1.deleteCharacter();
        CHECK(cursor1.position() == TextCursor::Position{0,2});
        CHECK(cursor2.position() == TextCursor::Position{0,2});
        cursor1.deletePreviousCharacter();
        CHECK(cursor1.position() == TextCursor::Position{1,1});
        CHECK(cursor2.position() == TextCursor::Position{1,1});
    }
    SECTION("sort") {
        cursor1.selectAll();
        cursor1.insertText("3\n4\n2\n1");
        cursor1.setPosition({0,1});
        cursor1.moveCharacterRight(true);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectedText() == "4");

        cursor2.selectAll();
        CHECK(cursor1.position() == TextCursor::Position{1,1});
        CHECK(cursor2.position() == TextCursor::Position{1,3});

        doc.sortLines(0,4, &cursor2);
        CHECK(doc.lineCount() == 4);
        REQUIRE(doc.line(0) == "1");
        REQUIRE(doc.line(1) == "2");
        REQUIRE(doc.line(2) == "3");
        REQUIRE(doc.line(3) == "4");

        CHECK(cursor1.position() == TextCursor::Position{1,3});
        CHECK(cursor2.position() == TextCursor::Position{1,0});
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectedText() == "4");
        CHECK(cursor2.hasSelection() == true);
        //CHECK(cursor2.selectedText() == "1\n2\n3\n4"); TODO: ich würde erwarten das die Start und end Position noch selectiert ist.
    }
    SECTION("select") {
        cursor1.setPosition({0,2});
        cursor1.insertText("hallo welt");

        bool selectionDirection = GENERATE(false, true);
        CAPTURE(selectionDirection);

        if (selectionDirection) {
            cursor1.moveToStartOfLine(true);
        } else {
            auto pos = cursor1.position();
            cursor1.moveToStartOfLine();
            cursor1.setPosition(pos, true);
        }

        CHECK(cursor1.selectedText() == "hallo welt");

        SECTION("hasSelection") {
            CHECK(cursor2.selectedText() == "");
            CHECK(cursor2.hasSelection() == false);
        }

        SECTION("in-selection") {
            cursor2.setPosition({5,2});

            SECTION("insert") {
                cursor2.insertText("a");
                CHECK(cursor1.selectedText() == "halloa welt");
            }
            SECTION("insert-newline") {
                cursor2.insertText("\n");
                CHECK(cursor1.selectedText() == "hallo\n welt");
            }
            SECTION("tab") {
                cursor2.insertText("\t");
                CHECK(cursor1.selectedText() == "hallo\t welt");
            }
            SECTION("del") {
                cursor2.deleteCharacter();
                CHECK(cursor1.selectedText() == "hallowelt");
            }
            SECTION("del-word") {
                cursor2.deleteWord();
                CHECK(cursor1.selectedText() == "hallo");
            }
            SECTION("del-line") {
                cursor2.deleteLine();
                CHECK(cursor1.selectedText() == "");
                CHECK(cursor1.hasSelection() == false);
            }
            SECTION("del-previous") {
                cursor2.deletePreviousCharacter();
                CHECK(cursor1.selectedText() == "hall welt");
            }
            SECTION("del-previous-word") {
                cursor2.deletePreviousWord();
                CHECK(cursor1.selectedText() == " welt");
            }
            SECTION("del-previous-line") {
                cursor2.setPosition({0,2});
                cursor2.deletePreviousCharacter();
                CHECK(cursor1.selectedText() == "hallo welt");
            }
            SECTION("del-all") {
                cursor2.selectAll();
                cursor2.deletePreviousCharacter();
                CHECK(cursor1.selectedText() == "");
                CHECK(cursor1.hasSelection() == false);
                CHECK(cursor1.position() == TextCursor::Position{0,0});
                CHECK(cursor2.position() == TextCursor::Position{0,0});
            }
        }

        SECTION("before-selection") {
            cursor2.setPosition({0,2});

            SECTION("insert") {
                cursor2.insertText("a");
                CHECK(cursor1.selectedText() == "hallo welt");
            }
            SECTION("insert-newline") {
                cursor2.insertText("\n");
                CHECK(cursor1.selectedText() == "hallo welt");
            }
            SECTION("tab") {
                cursor2.insertText("\t");
                CHECK(cursor1.selectedText() == "hallo welt");
            }
            SECTION("del") {
                cursor2.deleteCharacter();
                CHECK(cursor1.selectedText() == "allo welt");
            }
            SECTION("del-word") {
                cursor2.deleteWord();
                CHECK(cursor1.selectedText() == " welt");
            }
        }

        SECTION("after-selection") {
            cursor2.setPosition({10,2});

            SECTION("insert") {
                cursor2.insertText("a");
                CHECK(cursor1.selectedText() == "hallo welt");
            }
            SECTION("insert-newline") {
                cursor2.insertText("\n");
                CHECK(cursor1.selectedText() == "hallo welt");
            }
            SECTION("tab") {
                cursor2.insertText("\t");
                CHECK(cursor1.selectedText() == "hallo welt");
            }
            SECTION("del") {
                cursor2.deletePreviousCharacter();
                CHECK(cursor1.selectedText() == "hallo wel");
            }
            SECTION("del-word") {
                cursor2.deletePreviousWord();
                CHECK(cursor1.selectedText() == "hallo ");
            }
        }

        SECTION("marker") {
            LineMarker marker(&doc);
            CHECK(marker.line() == 0);

            // will move after generating, not active
            LineMarker marker2(&doc);
            CHECK(marker2.line() == 0);

            // wrong marker
            marker.setLine(45);
            CHECK(marker.line() == 4);
            cursor1.setPosition({0,0});
            cursor1.insertText("\n");
            CHECK(marker.line() == 5);
            CHECK(marker2.line() == 1);

            // cursor1 add lines
            marker.setLine(5);
            CHECK(marker.line() == 5);
            cursor1.setPosition({0,5});
            cursor1.insertText(" ");
            CHECK(cursor1.position() == TextCursor::Position{1,5});
            cursor1.insertText("\n");
            CHECK(marker.line() == 5);
            CHECK(marker2.line() == 1);

            // wrong marker
            marker.setLine(-1);
            CHECK(marker.line() == 0);
            cursor1.setPosition({0,0});
            cursor1.insertText("\n");
            CHECK(marker.line() == 1);
            CHECK(marker2.line() == 2);

            // cursor1 add and remove line
            marker.setLine(1);
            CHECK(marker.line() == 1);
            cursor1.setPosition({0,0});
            cursor1.insertText("\n");
            CHECK(marker.line() == 2);
            CHECK(marker2.line() == 3);

            // cursor2 add and remove line
            cursor2.setPosition({0,0});
            cursor2.insertText("\n");
            CHECK(marker.line() == 3);
            cursor2.deleteLine();
            CHECK(marker.line() == 2);

            // delete the line with marker
            cursor1.setPosition({0,2});
            cursor1.deleteLine();
            CHECK(marker.line() == 2);

            // delete the lines with marker
            cursor2.setPosition({0,1});
            cursor2.setPosition({0,3},true);
            cursor2.insertText(" ");
            CHECK(marker.line() == 1);

            // selectAll and delete
            cursor2.selectAll();
            cursor2.insertText(" ");
            CHECK(marker.line() == 0);
            CHECK(marker2.line() == 0);
        }
    }
}

TEST_CASE("Search") {
    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);
    Document doc;

    TextCursor cursor1{&doc, nullptr, [&terminal,&doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    SECTION("no char") {
        cursor1.insertText("");
        cursor1 = doc.findSync(" ", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == false);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 0);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 0);
    }
    SECTION("one char") {
        cursor1.insertText("m");
        cursor1 = doc.findSync("m", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 0);
    }
    SECTION("one char t") {
        cursor1.insertText("test");
        cursor1 = doc.findSync("t", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 0);

        cursor1 = doc.findSync("t", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 3);
        CHECK(cursor1.selectionEndPos().codeUnit == 4);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 0);
    }
    SECTION("one char repeated") {
        cursor1.insertText("tt");
        cursor1 = doc.findSync("t", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 0);

        cursor1 = doc.findSync("t", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 0);
    }
    SECTION("two char") {
        cursor1.insertText("tt");
        cursor1 = doc.findSync("tt", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 0);

        cursor1 = doc.findSync("tt", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 0);
    }
    SECTION("two char, to lines") {
        cursor1.insertText("tt\ntt");
        cursor1 = doc.findSync("tt", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 0);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);

        cursor1 = doc.findSync("tt", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 1);
        CHECK(cursor1.selectionEndPos().line == 1);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);
    }
    SECTION("two char multiline") {
        cursor1.insertText("at\nba");
        cursor1 = doc.findSync("t\nb", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 1);
        CHECK(cursor1.selectionStartPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
    }
    SECTION("two char multiline2") {
        cursor1.insertText("at\nt\nta");
        cursor1 = doc.findSync("t\nt", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 1);
        CHECK(cursor1.selectionStartPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);

        cursor1.setPosition({0, 1});
        cursor1 = doc.findSync("t\nt", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 1);
        CHECK(cursor1.selectionEndPos().line == 2);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
    }
    SECTION("three multiline") {
        cursor1.insertText("bat\nzy\nga");
        cursor1 = doc.findSync("t\nzy\ng", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 2);
        CHECK(cursor1.selectionStartPos().codeUnit == 2);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);

    }
    SECTION("four multiline") {
        cursor1.insertText("bae\nrt\nzu\nia");
        cursor1 = doc.findSync("e\nrt\nzu\ni", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 3);
        CHECK(cursor1.selectionStartPos().codeUnit == 2);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
    }
    SECTION("four multiline double") {
        cursor1.insertText("ab\nab\nab\n ab\nab\nab");
        cursor1 = doc.findSync("ab\nab\nab", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 2);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);

        cursor1 = doc.findSync("ab\nab\nab", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 3);
        CHECK(cursor1.selectionEndPos().line == 5);
        CHECK(cursor1.selectionStartPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);
    }
    SECTION("first not match") {
        cursor1.insertText("tt\naa\ntt\ntt");
        cursor1 = doc.findSync("tt\ntt", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 2);
        CHECK(cursor1.selectionEndPos().line == 3);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);
    }
    SECTION("first not match of three") {
        cursor1.insertText("tt\naa\ntt\ntt\nbb\ntt\ntt");
        cursor1 = doc.findSync("tt\nbb\ntt", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 3);
        CHECK(cursor1.selectionEndPos().line == 5);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);
    }
    SECTION("line break") {
        cursor1.insertText("tt\naa\nttt\ntt\nbb\ntt\ntt");
        cursor1 = doc.findSync("\n", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 1);
        CHECK(cursor1.selectionStartPos().codeUnit == 2);
        CHECK(cursor1.selectionEndPos().codeUnit == 0);

        cursor1 = doc.findSync("\n", cursor1, Document::FindFlag::FindWrap);
        cursor1 = doc.findSync("\n", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 2);
        CHECK(cursor1.selectionEndPos().line == 3);
        CHECK(cursor1.selectionStartPos().codeUnit == 3);
        CHECK(cursor1.selectionEndPos().codeUnit == 0);
    }
    SECTION("cursor in search string") {
        cursor1.insertText("blah\nblub\nblah\nblub");
        cursor1.setPosition({2,0});
        cursor1 = doc.findSync("lah\nblub", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 2);
        CHECK(cursor1.selectionEndPos().line == 3);
        CHECK(cursor1.selectionStartPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().codeUnit == 4);
    }
    SECTION("cursor in search string with wraparound") {
        cursor1.insertText("blah\nblub");
        cursor1.setPosition({2,0});
        cursor1 = doc.findSync("lah\nblub", cursor1, Document::FindFlag::FindWrap);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().line == 1);
        CHECK(cursor1.selectionStartPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().codeUnit == 4);
    }

    SECTION("backward hb") {
        cursor1.insertText("blah\nblub");
        cursor1 = doc.findSync("h\nb", cursor1, {Document::FindFlag::FindWrap, Document::FindFlag::FindBackward});
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 3);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().line == 1);
    }

    SECTION("backward hbl") {
        cursor1.insertText("blah\nblub");
        cursor1 = doc.findSync("h\nbl", cursor1, {Document::FindFlag::FindWrap, Document::FindFlag::FindBackward});
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 3);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 2);
        CHECK(cursor1.selectionEndPos().line == 1);
    }

    SECTION("backward ahb") {
        cursor1.insertText("blah\nblub");
        cursor1 = doc.findSync("ah\nb", cursor1, {Document::FindFlag::FindWrap, Document::FindFlag::FindBackward});
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 2);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().line == 1);
    }

    SECTION("backward 123") {

        cursor1.insertText("123\n123\n123\n123");
        cursor1 = doc.findSync("3\n1", cursor1, {Document::FindFlag::FindWrap, Document::FindFlag::FindBackward});
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 2);
        CHECK(cursor1.selectionStartPos().line == 2);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().line == 3);

        cursor1 = doc.findSync("3\n1", cursor1, {Document::FindFlag::FindWrap, Document::FindFlag::FindBackward});
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 2);
        CHECK(cursor1.selectionStartPos().line == 1);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().line == 2);

        cursor1 = doc.findSync("3\n1", cursor1, {Document::FindFlag::FindWrap, Document::FindFlag::FindBackward});
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 2);
        CHECK(cursor1.selectionStartPos().line == 0);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().line == 1);

        cursor1 = doc.findSync("3\n1", cursor1, {Document::FindFlag::FindWrap, Document::FindFlag::FindBackward});
        CHECK(cursor1.selectionStartPos().codeUnit == 2);
        CHECK(cursor1.selectionStartPos().line == 2);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().line == 3);
    }

    SECTION("backward one char in line") {
        cursor1.insertText("t\nt\nt");
        cursor1 = doc.findSync("t\nt", cursor1, {Document::FindFlag::FindWrap, Document::FindFlag::FindBackward});
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectionStartPos().codeUnit == 0);
        CHECK(cursor1.selectionStartPos().line == 1);
        CHECK(cursor1.selectionEndPos().codeUnit == 1);
        CHECK(cursor1.selectionEndPos().line == 2);
    }
}
