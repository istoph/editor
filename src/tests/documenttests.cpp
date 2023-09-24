// SPDX-License-Identifier: BSL-1.0

#include "document.h"

#include <optional>


#include "catchwrapper.h"

#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>
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


namespace {

    struct SearchTestCase {
        QString documentContents;
        bool hasMatch;
        TextCursor::Position start;
        TextCursor::Position end;
        TextCursor::Position foundStart;
        TextCursor::Position foundEnd;
        QString marker;
    };

    struct MatchPosition {
        TextCursor::Position foundStart{0, 0};
        TextCursor::Position foundEnd{0, 0};
        QString marker;
    };

}


// Generate a document contents and a list of expected matches from a specially formatted string.
// Lines that don't contain a '|' or '>' character are ignored.
// Lines should be visibly aligned on their '|' and '>' characters.
// Lines with a '|' character denote a line in the document to be used in the search test.
//     Although is it not checked the text before the '|' should be the line index (starting with 0).
//     Everything after the '|' is the line contents. Don't end lines with spaces or tabs are IDEs are commonly
//     configured to remove them at the end of the line.
// Line with a '>' character denote a search pattern matches in the preceeding '|' line.
//     Each search result uses a unique (for the whole input) character.
//     Single-character matches are marked with a character below the match in the '|' line.
//     Multi-character matches are marked with a run of characters covering the whole match blow the match in the '|' line.
//     Matches may span multiple document lines. If a match only consists of the line break in the document align the
//     match character one position right of the line end in the '|' line.
//     If overlapping matches exist multiple '>' lines per '|' line can be use.
static auto parseSearchInfo(const QString &input) {
    QStringList lines;

    int line = -1;
    int lineCodeUnits = -1;

    QMap<QChar, MatchPosition> matchesMap;

    auto extendOrCreateMatch = [&] (int codeUnit, QChar id) {
        if (!matchesMap.contains(id)) {
            if (codeUnit == lines.back().size()) {
                matchesMap[id] = MatchPosition{{codeUnit, line}, {0, line + 1}, id};
            } else {
                matchesMap[id] = MatchPosition{{codeUnit, line}, {codeUnit + 1, line}, id};
            }
        } else {
            if (matchesMap[id].foundEnd.line == line) {
                REQUIRE(matchesMap[id].foundEnd.codeUnit == codeUnit);
                matchesMap[id].foundEnd.codeUnit = codeUnit + 1;
            } else {
                REQUIRE(matchesMap[id].foundEnd.line + 1 == line);
                REQUIRE(codeUnit == 0);
                matchesMap[id].foundEnd.codeUnit = 1;
                matchesMap[id].foundEnd.line = line;
            }
        }
    };

    for (QString inputLine: input.split("\n")) {
        if (inputLine.contains('|')) {
            line += 1;
            lineCodeUnits = inputLine.section('|', 1).size();
            lines.append(inputLine.section('|', 1));
        } else if (inputLine.contains('>')) {
            QString part = inputLine.section('>', 1);
            for (int i = 0; i < part.size(); i++) {
                if (part[i] != ' ') {
                    extendOrCreateMatch(i, part[i]);
                }
            }
        }
    }

    QString documentContents = lines.join("\n");

    return std::make_tuple(matchesMap, documentContents, lines);
}

// See parseSearchInfo for description of input format
static std::vector<SearchTestCase> generateTestCases(const QString &input) {

    auto [matchesMap, documentContents, lines] = parseSearchInfo(input);

    if (matchesMap.isEmpty()) {
        return { {documentContents, false, {0,0}, {lines.last().size(), lines.size() - 1}, {0, 0}, {0, 0}, {}} };
    }

    QList<MatchPosition> matches = matchesMap.values();
    std::sort(matches.begin(), matches.end(), [](auto &a, auto &b) {
        return a.foundStart < b.foundStart;
    });

    std::vector<SearchTestCase> ret;

    ret.push_back(SearchTestCase{documentContents, true, {0, 0}, matches[0].foundStart, matches[0].foundStart, matches[0].foundEnd, matches[0].marker});
    TextCursor::Position nextStart = matches[0].foundStart;

    auto advanceNextStart = [&, lines=lines] {
        if (nextStart.codeUnit == lines[nextStart.line].size()) {
            nextStart.codeUnit = 0;
            nextStart.line += 1;
        } else {
            nextStart.codeUnit += 1;
        }
    };

    advanceNextStart();

    for (int i = 1; i < matches.size(); i++) {
        ret.push_back(SearchTestCase{documentContents, true, nextStart, matches[i].foundStart, matches[i].foundStart, matches[i].foundEnd, matches[i].marker});
        nextStart = matches[i].foundStart;
        advanceNextStart();
    }
    ret.push_back(SearchTestCase{documentContents, true, nextStart, {lines.last().size(), lines.size() - 1}, matches[0].foundStart, matches[0].foundEnd, matches[0].marker});

    return ret;
}

// See parseSearchInfo for description of input format
static std::vector<SearchTestCase> generateTestCasesBackward(const QString &input) {

    auto [matchesMap, documentContents, lines] = parseSearchInfo(input);

    if (matchesMap.isEmpty()) {
        return { {documentContents, false, {0,0}, {lines.last().size(), lines.size() - 1}, {0, 0}, {0, 0}, {}} };
    }

    QList<MatchPosition> matches = matchesMap.values();
    std::sort(matches.begin(), matches.end(), [](auto &a, auto &b) {
        return a.foundStart < b.foundStart;
    });

    std::vector<SearchTestCase> ret;

    ret.push_back(SearchTestCase{documentContents, true, matches.last().foundEnd, {lines.last().size(), lines.size() - 1},
                                 matches.last().foundStart, matches.last().foundEnd, matches.last().marker});

    TextCursor::Position nextEnd = matches.last().foundEnd;

    auto moveBackNextEnd = [&, lines=lines] {
        if (nextEnd.codeUnit == 0) {
            nextEnd.line -= 1;
            nextEnd.codeUnit = lines[nextEnd.line].size();
        } else {
            nextEnd.codeUnit--;
        }
    };

    moveBackNextEnd();

    for (int i = matches.size() - 2; i >= 0; i--) {
        ret.push_back(SearchTestCase{documentContents, true, matches[i].foundEnd, nextEnd, matches[i].foundStart, matches[i].foundEnd, matches[i].marker});
        nextEnd = matches[i].foundEnd;
        moveBackNextEnd();
    }

    ret.push_back(SearchTestCase{documentContents, true, {0, 0}, nextEnd, matches.last().foundStart, matches.last().foundEnd, matches.last().marker});

    return ret;
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

    auto runChecks = [&](const SearchTestCase &testCase, const QString &needle, Qt::CaseSensitivity caseMatching) {
        cursor1.insertText(testCase.documentContents);

        const bool wrapAround = GENERATE(false, true);

        CAPTURE(testCase.start);
        CAPTURE(testCase.end);
        CAPTURE(testCase.foundStart);
        CAPTURE(testCase.foundEnd);
        CAPTURE(wrapAround);
        Document::FindFlags options = wrapAround ? Document::FindFlag::FindWrap : Document::FindFlags{};
        if (caseMatching == Qt::CaseSensitive) {
            options |= Document::FindFlag::FindCaseSensitively;
        }

        cursor1.setPosition(testCase.start, true);
        cursor1.setAnchorPosition({0, 0});

        if (testCase.hasMatch) {
            auto result = doc.findSync(needle, cursor1, options);
            if (!wrapAround && testCase.start > testCase.foundStart) {
                CAPTURE(result.anchor());
                CAPTURE(result.position());
                CHECK(!result.hasSelection());
            } else {
                CHECK(result.hasSelection());
                CHECK(result.anchor() == testCase.foundStart);
                CHECK(result.position() == testCase.foundEnd);
            }

            while (cursor1.position() < testCase.end) {
                CAPTURE(cursor1.position());
                REQUIRE(!cursor1.atEnd());
                cursor1.moveCharacterRight();
                // check if we overstepped
                if (cursor1.position() > testCase.end) break;

                cursor1.setAnchorPosition({0, 0});
                auto result = doc.findSync(needle, cursor1, options);
                if (!wrapAround && testCase.start > testCase.foundStart) {
                    CAPTURE(result.anchor());
                    CAPTURE(result.position());
                    CHECK(!result.hasSelection());
                } else {
                    CHECK(result.hasSelection());
                    CHECK(result.anchor() == testCase.foundStart);
                    CHECK(result.position() == testCase.foundEnd);
                }
            }
        } else {
            auto result = doc.findSync(needle, cursor1, options);
            CHECK(!result.hasSelection());

            while (!cursor1.atEnd()) {
                CAPTURE(cursor1.position());
                cursor1.moveCharacterRight();

                cursor1.setAnchorPosition({0, 0});
                auto result = doc.findSync(needle, cursor1, options);
                CHECK(!result.hasSelection());
            }
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
        static auto testCases = generateTestCases(R"(
                                                  0|test
                                                   >1  2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "t", Qt::CaseSensitive);
    }

    SECTION("one char t - mismatched case") {
        static auto testCases = generateTestCases(R"(
                                                  0|Test
                                                   >   2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "t", Qt::CaseSensitive);
    }

    SECTION("one char t - case insensitive") {
        static auto testCases = generateTestCases(R"(
                                                  0|Test
                                                   >1  2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "t", Qt::CaseInsensitive);
    }

    SECTION("one char repeated") {
        static auto testCases = generateTestCases(R"(
                                                  0|tt
                                                   >12
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "t", Qt::CaseSensitive);
    }

    SECTION("two char") {
        static auto testCases = generateTestCases(R"(
                                                  0|tt
                                                   >11
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "tt", Qt::CaseSensitive);
    }

    SECTION("two char, two lines") {
        static auto testCases = generateTestCases(R"(
                                                  0|tt
                                                   >11
                                                  1|tt
                                                   >22
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "tt", Qt::CaseSensitive);
    }

    SECTION("two char multiline") {
        static auto testCases = generateTestCases(R"(
                                                  0|at
                                                   > 1
                                                  1|ba
                                                   >1
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "t\nb", Qt::CaseSensitive);
    }

    SECTION("multiline case mismatch first line") {
        static auto testCases = generateTestCases(R"(
                                                  0|heLlo
                                                  1|world
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "hello\nworld", Qt::CaseSensitive);
    }

    SECTION("multiline case mismatch second line") {
        static auto testCases = generateTestCases(R"(
                                                  0|hello
                                                  1|woRld
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "hello\nworld", Qt::CaseSensitive);
    }

    SECTION("multiline case insensitive") {
        static auto testCases = generateTestCases(R"(
                                                  0|helLo
                                                   >11111
                                                  1|woRld
                                                   >11111
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "hello\nworld", Qt::CaseInsensitive);
    }

    SECTION("two char multiline2") {
        static auto testCases = generateTestCases(R"(
                                                  0|at
                                                   > 1
                                                  1|t
                                                   >1
                                                   >2
                                                  2|ta
                                                   >2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "t\nt", Qt::CaseSensitive);
    }

    SECTION("three multiline") {
        static auto testCases = generateTestCases(R"(
                                                  0|bat
                                                   >  1
                                                  1|zy
                                                   >11
                                                  2|ga
                                                   >1
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "t\nzy\ng", Qt::CaseSensitive);
    }

    SECTION("three multiline case mismatch first line") {
        static auto testCases = generateTestCases(R"(
                                                  0|hEllo
                                                  1|whole
                                                  2|world
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "hello\nwhole\nworld", Qt::CaseSensitive);
    }

    SECTION("three multiline case mismatch middle line") {
        static auto testCases = generateTestCases(R"(
                                                  0|hello
                                                  1|whOle
                                                  2|world
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "hello\nwhole\nworld", Qt::CaseSensitive);
    }

    SECTION("three multiline case mismatch last line") {
        static auto testCases = generateTestCases(R"(
                                                  0|hello
                                                  1|whole
                                                  2|worlD
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "hello\nwhole\nworld", Qt::CaseSensitive);
    }

    SECTION("three multiline case insensitive") {
        static auto testCases = generateTestCases(R"(
                                                  0|heLlo
                                                   >11111
                                                  1|wHole
                                                   >11111
                                                  2|worlD
                                                   >11111
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "hello\nwhole\nworld", Qt::CaseInsensitive);
    }

    SECTION("four multiline") {
        static auto testCases = generateTestCases(R"(
                                                  0|bae
                                                   >  1
                                                  1|rt
                                                   >11
                                                  2|zu
                                                   >11
                                                  3|ia
                                                   >1
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "e\nrt\nzu\ni", Qt::CaseSensitive);
    }

    SECTION("four multiline double") {
        static auto testCases = generateTestCases(R"(
                                                  0|ab
                                                   >11
                                                  1|ab
                                                   >11
                                                  2|ab
                                                   >11
                                                  3| ab
                                                   > 22
                                                  4|ab
                                                   >22
                                                  5|ab
                                                   >22
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "ab\nab\nab", Qt::CaseSensitive);
    }

    SECTION("first not match") {
        static auto testCases = generateTestCases(R"(
                                                  0|tt
                                                  1|aa
                                                  2|tt
                                                   >11
                                                  3|tt
                                                   >11
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "tt\ntt", Qt::CaseSensitive);
    }

    SECTION("first not match of three") {
        static auto testCases = generateTestCases(R"(
                                                  0|tt
                                                  1|aa
                                                  2|tt
                                                  3|tt
                                                   >11
                                                  4|bb
                                                   >11
                                                  5|tt
                                                   >11
                                                  6|tt
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "tt\nbb\ntt", Qt::CaseSensitive);
    }

    SECTION("line break") {
        static auto testCases = generateTestCases(R"(
                                                  0|tt
                                                   >  1
                                                  1|aa
                                                   >  2
                                                  2|ttt
                                                   >   3
                                                  3|tt
                                                   >  4
                                                  4|bb
                                                   >  5
                                                  5|tt
                                                   >  6
                                                  6|tt
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "\n", Qt::CaseSensitive);
    }

    SECTION("cursor in search string") {
        static auto testCases = generateTestCases(R"(
                                                  0|blah
                                                   > 111
                                                  1|blub
                                                   >1111
                                                  2|blah
                                                   > 222
                                                  3|blub
                                                   >2222
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "lah\nblub", Qt::CaseSensitive);
    }

    SECTION("cursor in search string with wraparound") {
        static auto testCases = generateTestCases(R"(
                                                  0|blah
                                                   > 111
                                                  1|blub
                                                   >1111
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "lah\nblub", Qt::CaseSensitive);
    }


    auto runChecksBackward = [&](const SearchTestCase &testCase, const QString &needle, Qt::CaseSensitivity caseMatching) {
        cursor1.insertText(testCase.documentContents);

        const bool wrapAround = GENERATE(false, true);
        const bool useSelection = GENERATE(false, true);

        CAPTURE(testCase.start);
        CAPTURE(testCase.end);
        CAPTURE(testCase.foundStart);
        CAPTURE(testCase.foundEnd);
        CAPTURE(wrapAround);
        CAPTURE(useSelection);
        Document::FindFlags options = wrapAround ? Document::FindFlag::FindWrap : Document::FindFlags{};
        options |= Document::FindFlag::FindBackward;
        if (caseMatching == Qt::CaseSensitive) {
            options |= Document::FindFlag::FindCaseSensitively;
        }

        cursor1.setPosition(testCase.start);

        if (useSelection && !cursor1.atEnd() && !cursor1.atStart()) {
            cursor1.moveCharacterRight();
            cursor1.setAnchorPosition({0, 0});
        }

        if (testCase.hasMatch) {
            auto result = doc.findSync(needle, cursor1, options);
            if (!wrapAround && testCase.start <= testCase.foundStart) {
                CAPTURE(result.anchor());
                CAPTURE(result.position());
                CHECK_FALSE(result.hasSelection());
            } else {
                CHECK(result.hasSelection());
                CHECK(result.anchor() == testCase.foundStart);
                CHECK(result.position() == testCase.foundEnd);
            }

            while (cursor1.position() < testCase.end) {
                REQUIRE(!cursor1.atEnd());
                cursor1.moveCharacterRight();
                // check if we overstepped
                if (cursor1.position() > testCase.end) break;
                CAPTURE(cursor1.position());

                //cursor1.setAnchorPosition({0, 0});
                auto result = doc.findSync(needle, cursor1, options);
                if (!wrapAround && testCase.start <= testCase.foundStart) {
                    CAPTURE(result.anchor());
                    CAPTURE(result.position());
                    CHECK_FALSE(result.hasSelection());
                } else {
                    CHECK(result.hasSelection());
                    CHECK(result.anchor() == testCase.foundStart);
                    CHECK(result.position() == testCase.foundEnd);
                }
            }
        } else {
            auto result = doc.findSync(needle, cursor1, options);
            CHECK(!result.hasSelection());

            while (!cursor1.atEnd()) {
                CAPTURE(cursor1.position());
                cursor1.moveCharacterRight();

                cursor1.setAnchorPosition({0, 0});
                auto result = doc.findSync(needle, cursor1, options);
                CHECK(!result.hasSelection());
            }
        }
    };

    SECTION("backward one char t") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|test
                                                   >1  2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "t", Qt::CaseSensitive);
    }

    SECTION("backward one char t - mismatched case") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|Test
                                                   >   2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "t", Qt::CaseSensitive);
    }

    SECTION("backward one char t - case insensitive") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|Test
                                                   >1  2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "t", Qt::CaseInsensitive);
    }


    SECTION("backward hb") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|blah
                                                   >   1
                                                  1|blub
                                                   >1
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "h\nb", Qt::CaseSensitive);
    }

    SECTION("backward hbl") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|blah
                                                   >   1
                                                  1|blub
                                                   >11
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "h\nbl", Qt::CaseSensitive);
    }

    SECTION("backward ahb") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|blah
                                                   >  11
                                                  1|blub
                                                   >1
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "ah\nb", Qt::CaseSensitive);
    }

    SECTION("backward multiline case mismatch first line") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|heLlo
                                                  1|world
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "hello\nworld", Qt::CaseSensitive);
    }

    SECTION("backward multiline case mismatch second line") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|hello
                                                  1|woRld
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "hello\nworld", Qt::CaseSensitive);
    }

    SECTION("backward multiline case insensitive") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|heLlo
                                                   >11111
                                                  1|woRld
                                                   >11111
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "hello\nworld", Qt::CaseInsensitive);
    }

    SECTION("backward three multiline case mismatch first line") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|hEllo
                                                  1|whole
                                                  2|world
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "hello\nwhole\nworld", Qt::CaseSensitive);
    }

    SECTION("backward three multiline case mismatch middle line") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|hello
                                                  1|whOle
                                                  2|world
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "hello\nwhole\nworld", Qt::CaseSensitive);
    }

    SECTION("backward three multiline case mismatch last line") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|hello
                                                  1|whole
                                                  2|worlD
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "hello\nwhole\nworld", Qt::CaseSensitive);
    }

    SECTION("backward three multiline case insensitive") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|heLlo
                                                   >11111
                                                  1|wHole
                                                   >11111
                                                  2|worlD
                                                   >11111
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "hello\nwhole\nworld", Qt::CaseInsensitive);
    }

    SECTION("backward 123") {

        static auto testCases = generateTestCasesBackward(R"(
                                                  0|123
                                                   >  a
                                                  1|123
                                                   >a b
                                                  2|123
                                                   >b c
                                                  3|123
                                                   >c
                                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "3\n1", Qt::CaseSensitive);
    }

    SECTION("backward one char in line") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|t
                                                   >1
                                                  1|t
                                                   >1
                                                   >2
                                                  2|t
                                                   >2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "t\nt", Qt::CaseSensitive);
    }
}


TEST_CASE("regex search") {
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

    struct MatchCaptures {
        QStringList captures;
        QMap<QString, QString> named;
    };

    auto runChecks = [&](const SearchTestCase &testCase, const QRegularExpression &needle, Qt::CaseSensitivity caseMatching,
            const QMap<QString, MatchCaptures> expectedCapturesMap) {
        cursor1.insertText(testCase.documentContents);

        const bool wrapAround = GENERATE(false, true);

        auto checkCaptures = [&] (const DocumentFindResult &res) {
            REQUIRE(expectedCapturesMap.contains(testCase.marker));
            auto &expectedCaptures = expectedCapturesMap[testCase.marker];
            CHECK(res.regexLastCapturedIndex() == expectedCaptures.captures.size() - 1);
            if (res.regexLastCapturedIndex() == expectedCaptures.captures.size() - 1) {
                for (int i = 0; i < expectedCaptures.captures.size(); i++) {
                    CHECK(res.regexCapture(i) == expectedCaptures.captures[i]);
                }
            }
            for (QString key: expectedCaptures.named.keys()) {
                CHECK(res.regexCapture(key) == expectedCaptures.named.value(key));
            }
        };

        CAPTURE(testCase.marker);
        CAPTURE(testCase.start);
        CAPTURE(testCase.end);
        CAPTURE(testCase.foundStart);
        CAPTURE(testCase.foundEnd);
        CAPTURE(wrapAround);
        Document::FindFlags options = wrapAround ? Document::FindFlag::FindWrap : Document::FindFlags{};
        if (caseMatching == Qt::CaseSensitive) {
            options |= Document::FindFlag::FindCaseSensitively;
        }

        cursor1.setPosition(testCase.start, true);
        cursor1.setAnchorPosition({0, 0});

        if (testCase.hasMatch) {
            DocumentFindResult result = doc.findSyncWithDetails(needle, cursor1, options);
            if (!wrapAround && testCase.start > testCase.foundStart) {
                CAPTURE(result.cursor.anchor());
                CAPTURE(result.cursor.position());
                CHECK(!result.cursor.hasSelection());
            } else {
                CHECK(result.cursor.hasSelection());
                CHECK(result.cursor.anchor() == testCase.foundStart);
                CHECK(result.cursor.position() == testCase.foundEnd);
                checkCaptures(result);
            }

            while (cursor1.position() < testCase.end) {
                CAPTURE(cursor1.position());
                REQUIRE(!cursor1.atEnd());
                cursor1.moveCharacterRight();
                // check if we overstepped
                if (cursor1.position() > testCase.end) break;

                cursor1.setAnchorPosition({0, 0});
                DocumentFindResult result = doc.findSyncWithDetails(needle, cursor1, options);
                if (!wrapAround && testCase.start > testCase.foundStart) {
                    CAPTURE(result.cursor.anchor());
                    CAPTURE(result.cursor.position());
                    CHECK(!result.cursor.hasSelection());
                } else {
                    CHECK(result.cursor.hasSelection());
                    CHECK(result.cursor.anchor() == testCase.foundStart);
                    CHECK(result.cursor.position() == testCase.foundEnd);
                    checkCaptures(result);
                }
            }
        } else {
            auto result = doc.findSync(needle, cursor1, options);
            CHECK(!result.hasSelection());

            while (!cursor1.atEnd()) {
                CAPTURE(cursor1.position());
                cursor1.moveCharacterRight();

                cursor1.setAnchorPosition({0, 0});
                auto result = doc.findSync(needle, cursor1, options);
                CHECK(!result.hasSelection());
            }
        }
    };

    SECTION("literal-a") {
        static auto testCases = generateTestCases(R"(
                                                  0|some Text
                                                  1|same Thing
                                                   > 1
                                                  2|aaaa bbbb
                                                   >2345
                                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("a"), Qt::CaseSensitive,
                {
                      {"1", MatchCaptures{ {"a"}, {}}},
                      {"2", MatchCaptures{ {"a"}, {}}},
                      {"3", MatchCaptures{ {"a"}, {}}},
                      {"4", MatchCaptures{ {"a"}, {}}},
                      {"5", MatchCaptures{ {"a"}, {}}}
                });

    }

    SECTION("one char t - mismatched case") {
        static auto testCases = generateTestCases(R"(
                                                  0|Test
                                                   >   2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("t"), Qt::CaseSensitive, {
              {"2", MatchCaptures{ {"t"}, {}}},
        });
    }

    SECTION("one char t - case insensitive") {
        static auto testCases = generateTestCases(R"(
                                                  0|Test
                                                   >1  2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("t"), Qt::CaseInsensitive, {
              {"1", MatchCaptures{ {"T"}, {}}},
              {"2", MatchCaptures{ {"t"}, {}}},
        });
    }

    SECTION("one char t - mismatched case with pattern option") {
        static auto testCases = generateTestCases(R"(
                                                  0|Test
                                                   >   2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("t", QRegularExpression::PatternOption::CaseInsensitiveOption),
                  Qt::CaseSensitive, {
              {"2", MatchCaptures{ {"t"}, {}}},
        });
    }

    SECTION("literal-abc") {
        static auto testCases = generateTestCases(R"(
            0|some Test
            1|abc Thing
             >111
            2|xabcabc bbbb
             > 222333
        )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("abc"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"abc"}, {}}},
              {"2", MatchCaptures{ {"abc"}, {}}},
              {"3", MatchCaptures{ {"abc"}, {}}},
        });
    }

    SECTION("literal-abc-nonutf16-in-line") {
        static auto testCases = generateTestCases(QString(R"(
                                  0|some Test
                                  1|abc Xhing
                                   >111
                                  2|xabcabc bbbb
                                   > 222333
                              )").replace('X', QChar(0xdc00)));

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression{"abc"}, Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"abc"}, {}}},
              {"2", MatchCaptures{ {"abc"}, {}}},
              {"3", MatchCaptures{ {"abc"}, {}}},
        });
    }

    SECTION("literal-abc-nonutf16-at-end") {
        static auto testCases = generateTestCases(QString(R"(
                                  0|some Test
                                  1|abc thingX
                                   >111
                                  2|xabcabc bbbb
                                   > 222333
                              )").replace('X', QChar(0xd800)));

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression{"abc"}, Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"abc"}, {}}},
              {"2", MatchCaptures{ {"abc"}, {}}},
              {"3", MatchCaptures{ {"abc"}, {}}},
        });
    }

    SECTION("multiline line literal") {
        static auto testCases = generateTestCases(R"(
                                  0|some Test
                                   >        1
                                  1|abc Thing
                                   >1
                                  2|xabcabc bbbb
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("t\na"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"t\na"}, {}}},
        });
    }

    SECTION("three multiline line literal") {
        static auto testCases = generateTestCases(R"(
                                  0|some Test
                                   >        1
                                  1|abc Thing
                                   >111111111
                                  2|xabcabc bbbb
                                   >1111
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("t\n.*\nxabc"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"t\nabc Thing\nxabc"}, {}}},
        });
    }

    SECTION("multiline line dotdefault") {
        static auto testCases = generateTestCases(R"(
                                  0|some Test
                                  1|abc Thing
                                  2|xabcabc bbbb
                              )");
        REQUIRE(testCases[0].hasMatch == false);
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("t.*xabc"), Qt::CaseSensitive, {});
    }

    SECTION("multiline dotall") {
        static auto testCases = generateTestCases(R"(
                                  0|some Test
                                   >        1
                                  1|abc Thing
                                   >111111111
                                  2|xabcabc bbbb
                                   >1111
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression{"t.*xabc", QRegularExpression::PatternOption::DotMatchesEverythingOption},
                  Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"t\nabc Thing\nxabc"}, {}}},
        });
    }

    SECTION("anchors") {
        static auto testCases = generateTestCases(R"(
                                  0|some Test
                                  1|abc Thing
                                   >111111111
                                  2|xabcabc bbbb
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("^abc Thing$"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"abc Thing"}, {}}},
        });
    }

    SECTION("numbers") {
        static auto testCases = generateTestCases(R"(
                                  0|This is 1 test
                                   >        1
                                  1|2 test
                                   >2
                                  2|and the last 3
                                   >             3
                                  3|or 123 456tests
                                   >   456 789
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("[0-9]"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"1"}, {}}},
              {"2", MatchCaptures{ {"2"}, {}}},
              {"3", MatchCaptures{ {"3"}, {}}},
              {"4", MatchCaptures{ {"1"}, {}}},
              {"5", MatchCaptures{ {"2"}, {}}},
              {"6", MatchCaptures{ {"3"}, {}}},
              {"7", MatchCaptures{ {"4"}, {}}},
              {"8", MatchCaptures{ {"5"}, {}}},
              {"9", MatchCaptures{ {"6"}, {}}},
        });
    }

    SECTION("numbers+") {
        static auto testCases = generateTestCases(R"(
                                  0|This is 1 test
                                   >        1
                                  1|2 test
                                   >2
                                  2|and the last 3
                                   >             3
                                  3|or 123 456tests
                                   >   444 555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("[0-9]+"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"1"}, {}}},
              {"2", MatchCaptures{ {"2"}, {}}},
              {"3", MatchCaptures{ {"3"}, {}}},
              {"4", MatchCaptures{ {"123"}, {}}},
              {"5", MatchCaptures{ {"456"}, {}}},
        });
    }

    SECTION("captures") {
        static auto testCases = generateTestCases(R"(
                                  0|This is <1> test
                                   >        111
                                  1|<2> test
                                   >222
                                  2|and the last <3>
                                   >             333
                                  3|or <123> <456>tests
                                   >   44444 55555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("<([0-9]+)>"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"<1>", "1"}, {}}},
              {"2", MatchCaptures{ {"<2>", "2"}, {}}},
              {"3", MatchCaptures{ {"<3>", "3"}, {}}},
              {"4", MatchCaptures{ {"<123>", "123"}, {}}},
              {"5", MatchCaptures{ {"<456>", "456"}, {}}},
        });
    }

    SECTION("named captures") {
        static auto testCases = generateTestCases(R"(
                                  0|This is <1> test
                                   >        111
                                  1|<2> test
                                   >222
                                  2|and the last <3>
                                   >             333
                                  3|or <123> <456>tests
                                   >   44444 55555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("<(?<zahl>[0-9]+)>"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"<1>", "1"}, {{"zahl", "1"}}}},
              {"2", MatchCaptures{ {"<2>", "2"}, {{"zahl", "2"}}}},
              {"3", MatchCaptures{ {"<3>", "3"}, {{"zahl", "3"}}}},
              {"4", MatchCaptures{ {"<123>", "123"}, {{"zahl", "123"}}}},
              {"5", MatchCaptures{ {"<456>", "456"}, {{"zahl", "456"}}}},
        });
    }

    SECTION("captures 2x") {
        static auto testCases = generateTestCases(R"(
                                  0|This is <1=x> test
                                   >        11111
                                  1|<2=u> test
                                   >22222
                                  2|and the last <3=t>
                                   >             33333
                                  3|or <123=xut> <456=L>tests
                                   >   444444444 5555555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("<([0-9]+)=([a-zA-Z]+)>"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"<1=x>", "1", "x"}, {}}},
              {"2", MatchCaptures{ {"<2=u>", "2", "u"}, {}}},
              {"3", MatchCaptures{ {"<3=t>", "3", "t"}, {}}},
              {"4", MatchCaptures{ {"<123=xut>", "123", "xut"}, {}}},
              {"5", MatchCaptures{ {"<456=L>", "456", "L"}, {}}},
        });
    }

    SECTION("named captures 2x") {
        static auto testCases = generateTestCases(R"(
                                  0|This is <1=x> test
                                   >        11111
                                  1|<2=u> test
                                   >22222
                                  2|and the last <3=t>
                                   >             33333
                                  3|or <123=xut> <456=L>tests
                                   >   444444444 5555555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("<(?<zahl>[0-9]+)=(?<letters>[a-zA-Z]+)>"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"<1=x>", "1", "x"}, {{"zahl", "1"}, {"letters", "x"}}}},
              {"2", MatchCaptures{ {"<2=u>", "2", "u"}, {{"zahl", "2"}, {"letters", "u"}}}},
              {"3", MatchCaptures{ {"<3=t>", "3", "t"}, {{"zahl", "3"}, {"letters", "t"}}}},
              {"4", MatchCaptures{ {"<123=xut>", "123", "xut"}, {{"zahl", "123"}, {"letters", "xut"}}}},
              {"5", MatchCaptures{ {"<456=L>", "456", "L"}, {{"zahl", "456"}, {"letters", "L"}}}},
        });
    }

    auto runChecksBackward = [&](const SearchTestCase &testCase, const QRegularExpression &needle, Qt::CaseSensitivity caseMatching,
            const QMap<QString, MatchCaptures> expectedCapturesMap) {
        cursor1.insertText(testCase.documentContents);

        const bool wrapAround = GENERATE(false, true);
        const bool useSelection = GENERATE(false, true);

        auto checkCaptures = [&] (const DocumentFindResult &res) {
            REQUIRE(expectedCapturesMap.contains(testCase.marker));
            auto &expectedCaptures = expectedCapturesMap[testCase.marker];
            CHECK(res.regexLastCapturedIndex() == expectedCaptures.captures.size() - 1);
            if (res.regexLastCapturedIndex() == expectedCaptures.captures.size() - 1) {
                for (int i = 0; i < expectedCaptures.captures.size(); i++) {
                    CHECK(res.regexCapture(i) == expectedCaptures.captures[i]);
                }
            }
            for (QString key: expectedCaptures.named.keys()) {
                CHECK(res.regexCapture(key) == expectedCaptures.named.value(key));
            }
        };

        CAPTURE(testCase.marker);
        CAPTURE(testCase.start);
        CAPTURE(testCase.end);
        CAPTURE(testCase.foundStart);
        CAPTURE(testCase.foundEnd);
        CAPTURE(wrapAround);
        CAPTURE(useSelection);
        Document::FindFlags options = wrapAround ? Document::FindFlag::FindWrap : Document::FindFlags{};
        options |= Document::FindFlag::FindBackward;
        if (caseMatching == Qt::CaseSensitive) {
            options |= Document::FindFlag::FindCaseSensitively;
        }

        cursor1.setPosition(testCase.start);

        if (useSelection && !cursor1.atEnd() && !cursor1.atStart()) {
            cursor1.moveCharacterRight();
            cursor1.setAnchorPosition({0, 0});
        }

        if (testCase.hasMatch) {
            DocumentFindResult result = doc.findSyncWithDetails(needle, cursor1, options);
            if (!wrapAround && testCase.start <= testCase.foundStart) {
                CAPTURE(result.cursor.anchor());
                CAPTURE(result.cursor.position());
                CHECK_FALSE(result.cursor.hasSelection());
            } else {
                CHECK(result.cursor.hasSelection());
                CHECK(result.cursor.anchor() == testCase.foundStart);
                CHECK(result.cursor.position() == testCase.foundEnd);
                checkCaptures(result);
            }

            while (cursor1.position() < testCase.end) {
                REQUIRE(!cursor1.atEnd());
                cursor1.moveCharacterRight();
                // check if we overstepped
                if (cursor1.position() > testCase.end) break;
                CAPTURE(cursor1.position());

                //cursor1.setAnchorPosition({0, 0});
                DocumentFindResult result = doc.findSyncWithDetails(needle, cursor1, options);
                if (!wrapAround && testCase.start <= testCase.foundStart) {
                    CAPTURE(result.cursor.anchor());
                    CAPTURE(result.cursor.position());
                    CHECK_FALSE(result.cursor.hasSelection());
                } else {
                    CHECK(result.cursor.hasSelection());
                    CHECK(result.cursor.anchor() == testCase.foundStart);
                    CHECK(result.cursor.position() == testCase.foundEnd);
                    checkCaptures(result);
                }
            }
        } else {
            auto result = doc.findSync(needle, cursor1, options);
            CHECK(!result.hasSelection());

            while (!cursor1.atEnd()) {
                CAPTURE(cursor1.position());
                cursor1.moveCharacterRight();

                cursor1.setAnchorPosition({0, 0});
                auto result = doc.findSync(needle, cursor1, options);
                CHECK(!result.hasSelection());
            }
        }
    };


    SECTION("backward literal-a") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|some Text
                                                  1|same Thing
                                                   > 1
                                                  2|aaaa bbbb
                                                   >2345
                                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("a"), Qt::CaseSensitive,
        {
              {"1", MatchCaptures{ {"a"}, {}}},
              {"2", MatchCaptures{ {"a"}, {}}},
              {"3", MatchCaptures{ {"a"}, {}}},
              {"4", MatchCaptures{ {"a"}, {}}},
              {"5", MatchCaptures{ {"a"}, {}}}
        });

    }

    SECTION("backward one char t - mismatched case") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|Test
                                                   >   2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("t"), Qt::CaseSensitive, {
                              {"2", MatchCaptures{ {"t"}, {}}},
                        });
    }

    SECTION("backward one char t - case insensitive") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|Test
                                                   >1  2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("t"), Qt::CaseInsensitive, {
              {"1", MatchCaptures{ {"T"}, {}}},
              {"2", MatchCaptures{ {"t"}, {}}},
        });
    }

    SECTION("backward one char t - mismatched case with pattern option") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|Test
                                                   >   2
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("t", QRegularExpression::PatternOption::CaseInsensitiveOption),
                  Qt::CaseSensitive, {
              {"2", MatchCaptures{ {"t"}, {}}},
        });
    }

    SECTION("backward literal-abc") {
        static auto testCases = generateTestCasesBackward(R"(
            0|some Test
            1|abc Thing
             >111
            2|xabcabc bbbb
             > 222333
        )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("abc"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"abc"}, {}}},
              {"2", MatchCaptures{ {"abc"}, {}}},
              {"3", MatchCaptures{ {"abc"}, {}}},
        });
    }

    SECTION("backward literal-abc-nonutf16-in-line") {
        static auto testCases = generateTestCasesBackward(QString(R"(
                                  0|some Test
                                  1|abc Xhing
                                   >111
                                  2|xabcabc bbbb
                                   > 222333
                              )").replace('X', QChar(0xdc00)));

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression{"abc"}, Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"abc"}, {}}},
              {"2", MatchCaptures{ {"abc"}, {}}},
              {"3", MatchCaptures{ {"abc"}, {}}},
        });
    }

    SECTION("backward literal-abc-nonutf16-at-end") {
        static auto testCases = generateTestCasesBackward(QString(R"(
                                  0|some Test
                                  1|abc thingX
                                   >111
                                  2|xabcabc bbbb
                                   > 222333
                              )").replace('X', QChar(0xd800)));

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression{"abc"}, Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"abc"}, {}}},
              {"2", MatchCaptures{ {"abc"}, {}}},
              {"3", MatchCaptures{ {"abc"}, {}}},
        });
    }

    SECTION("backward multiline line literal") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|some Test
                                   >        1
                                  1|abc Thing
                                   >1
                                  2|xabcabc bbbb
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("t\na"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"t\na"}, {}}},
        });
    }

    SECTION("backward three multiline line literal") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|some Test
                                   >        1
                                  1|abc Thing
                                   >111111111
                                  2|xabcabc bbbb
                                   >1111
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("t\n.*\nxabc"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"t\nabc Thing\nxabc"}, {}}},
        });
    }

    SECTION("backward multiline line dotdefault") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|some Test
                                  1|abc Thing
                                  2|xabcabc bbbb
                              )");
        REQUIRE(testCases[0].hasMatch == false);
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("t.*xabc"), Qt::CaseSensitive, {});
    }

    SECTION("backward multiline dotall") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|some Test
                                   >        1
                                  1|abc Thing
                                   >111111111
                                  2|xabcabc bbbb
                                   >1111
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression{"t.*xabc", QRegularExpression::PatternOption::DotMatchesEverythingOption},
                  Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"t\nabc Thing\nxabc"}, {}}},
        });
    }


    SECTION("backward anchors") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|some Test
                                  1|abc Thing
                                   >111111111
                                  2|xabcabc bbbb
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("^abc Thing$"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"abc Thing"}, {}}},
        });
    }

    SECTION("backward numbers") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|This is 1 test
                                   >        1
                                  1|2 test
                                   >2
                                  2|and the last 3
                                   >             3
                                  3|or 123 456tests
                                   >   456 789
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("[0-9]"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"1"}, {}}},
              {"2", MatchCaptures{ {"2"}, {}}},
              {"3", MatchCaptures{ {"3"}, {}}},
              {"4", MatchCaptures{ {"1"}, {}}},
              {"5", MatchCaptures{ {"2"}, {}}},
              {"6", MatchCaptures{ {"3"}, {}}},
              {"7", MatchCaptures{ {"4"}, {}}},
              {"8", MatchCaptures{ {"5"}, {}}},
              {"9", MatchCaptures{ {"6"}, {}}},
        });
    }

    SECTION("backward numbers+") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|This is 1 test
                                   >        1
                                  1|2 test
                                   >2
                                  2|and the last 3
                                   >             3
                                  3|or 123 456tests
                                   >   444 555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("[0-9]+"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"1"}, {}}},
              {"2", MatchCaptures{ {"2"}, {}}},
              {"3", MatchCaptures{ {"3"}, {}}},
              {"4", MatchCaptures{ {"123"}, {}}},
              {"5", MatchCaptures{ {"456"}, {}}},
        });
    }

    SECTION("backward captures") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|This is <1> test
                                   >        111
                                  1|<2> test
                                   >222
                                  2|and the last <3>
                                   >             333
                                  3|or <123> <456>tests
                                   >   44444 55555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("<([0-9]+)>"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"<1>", "1"}, {}}},
              {"2", MatchCaptures{ {"<2>", "2"}, {}}},
              {"3", MatchCaptures{ {"<3>", "3"}, {}}},
              {"4", MatchCaptures{ {"<123>", "123"}, {}}},
              {"5", MatchCaptures{ {"<456>", "456"}, {}}},
        });
    }

    SECTION("backward named captures") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|This is <1> test
                                   >        111
                                  1|<2> test
                                   >222
                                  2|and the last <3>
                                   >             333
                                  3|or <123> <456>tests
                                   >   44444 55555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("<(?<zahl>[0-9]+)>"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"<1>", "1"}, {{"zahl", "1"}}}},
              {"2", MatchCaptures{ {"<2>", "2"}, {{"zahl", "2"}}}},
              {"3", MatchCaptures{ {"<3>", "3"}, {{"zahl", "3"}}}},
              {"4", MatchCaptures{ {"<123>", "123"}, {{"zahl", "123"}}}},
              {"5", MatchCaptures{ {"<456>", "456"}, {{"zahl", "456"}}}},
        });
    }

    SECTION("backward captures 2x") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|This is <1=x> test
                                   >        11111
                                  1|<2=u> test
                                   >22222
                                  2|and the last <3=t>
                                   >             33333
                                  3|or <123=xut> <456=L>tests
                                   >   444444444 5555555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("<([0-9]+)=([a-zA-Z]+)>"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"<1=x>", "1", "x"}, {}}},
              {"2", MatchCaptures{ {"<2=u>", "2", "u"}, {}}},
              {"3", MatchCaptures{ {"<3=t>", "3", "t"}, {}}},
              {"4", MatchCaptures{ {"<123=xut>", "123", "xut"}, {}}},
              {"5", MatchCaptures{ {"<456=L>", "456", "L"}, {}}},
        });
    }

    SECTION("backward named captures 2x") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|This is <1=x> test
                                   >        11111
                                  1|<2=u> test
                                   >22222
                                  2|and the last <3=t>
                                   >             33333
                                  3|or <123=xut> <456=L>tests
                                   >   444444444 5555555
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("<(?<zahl>[0-9]+)=(?<letters>[a-zA-Z]+)>"), Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"<1=x>", "1", "x"}, {{"zahl", "1"}, {"letters", "x"}}}},
              {"2", MatchCaptures{ {"<2=u>", "2", "u"}, {{"zahl", "2"}, {"letters", "u"}}}},
              {"3", MatchCaptures{ {"<3=t>", "3", "t"}, {{"zahl", "3"}, {"letters", "t"}}}},
              {"4", MatchCaptures{ {"<123=xut>", "123", "xut"}, {{"zahl", "123"}, {"letters", "xut"}}}},
              {"5", MatchCaptures{ {"<456=L>", "456", "L"}, {{"zahl", "456"}, {"letters", "L"}}}},
        });
    }

}

TEST_CASE("Document Undo Redo") {
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

    struct Action {
        bool b;
        std::function<void()> step;
    };

    auto runChecks = [&](std::initializer_list<Action> steps) {

        const bool groupSteps = GENERATE(false, true);
        CAPTURE(groupSteps);

        struct DocumentState {
            QList<QString> lines;
            bool noNewline = false;
        };

        QList<DocumentState> states;

        auto captureState = [&] {
            QList<QString> lines;
            for (int i = 0; i < doc.lineCount(); i++) {
                lines.append(doc.line(i));
            }
            states.append(DocumentState{lines, doc.noNewLine()});
        };

        auto compareState = [&] (const DocumentState &state) {
            QList<QString> lines;
            for (int i = 0; i < doc.lineCount(); i++) {
                lines.append(doc.line(i));
            }
            CHECK(state.lines == lines);
            CHECK(state.noNewline == doc.noNewLine());
        };

        captureState();
        compareState(states.last());
        for (const auto& step: steps) {
            std::optional<Document::UndoGroup> group;
            if (groupSteps) {
                auto dummyCursor = cursor1;
                group.emplace(doc.startUndoGroup(&dummyCursor));
            }
            step.step();
            if (step.b) {
                captureState();
                compareState(states.last());
            }
        }

        int stateIndex = states.size() - 1;

        auto dummyCursor = cursor1;

        while (doc.isUndoAvailable()) {
            doc.undo(&dummyCursor);
            REQUIRE(stateIndex > 0);
            stateIndex -= 1;
            compareState(states[stateIndex]);
        }

        REQUIRE(stateIndex == 0);

        while (doc.isRedoAvailable()) {
            doc.redo(&dummyCursor);
            REQUIRE(stateIndex + 1 < states.size());
            stateIndex += 1;
            compareState(states[stateIndex]);
        }

        REQUIRE(stateIndex + 1 == states.size());
    };

    SECTION("inserts") {
        runChecks({
                      { false, [&] { cursor1.insertText("a"); } },
                      { false, [&] { cursor1.insertText("b"); } },
                      { true,  [&] { cursor1.insertText("c"); } }
                  });
    }

    SECTION("inserts2") {
        runChecks({
                      { false, [&] { cursor1.insertText("a"); } },
                      { false, [&] { cursor1.insertText("b"); } },
                      { true,  [&] { cursor1.insertText("c"); } },
                      { false, [&] { cursor1.insertText(" "); } },
                      { false, [&] { cursor1.insertText("d"); } },
                      { false, [&] { cursor1.insertText("e"); } },
                      { true,  [&] { cursor1.insertText("f"); } }
                  });
    }

    SECTION("newline") {
        runChecks({
                      { true,  [&] { cursor1.insertText("\n"); } },
                      { false, [&] { cursor1.insertText("\n"); } },
                      { false, [&] { cursor1.insertText("a"); } },
                      { true,  [&] { cursor1.insertText("b"); } },
                      { false, [&] { cursor1.insertText("\n"); } },
                      { true,  [&] { cursor1.insertText("c"); } },
                      { true,  [&] { cursor1.insertText("\n"); } }
                  });
    }

    SECTION("space") {
        runChecks({
                      { true,  [&] { cursor1.insertText(" "); } },
                      { false, [&] { cursor1.insertText(" "); } },
                      { false, [&] { cursor1.insertText("a"); } },
                      { true,  [&] { cursor1.insertText("b"); } },
                      { false, [&] { cursor1.insertText(" "); } },
                      { true,  [&] { cursor1.insertText("c"); } },
                      { true,  [&] { cursor1.insertText(" "); } }
                  });
    }


    SECTION("tab") {
        runChecks({
                      { true,  [&] { cursor1.insertText("\t"); } },
                      { false, [&] { cursor1.insertText("\t"); } },
                      { false, [&] { cursor1.insertText("a"); } },
                      { true,  [&] { cursor1.insertText("b"); } },
                      { false, [&] { cursor1.insertText("\t"); } },
                      { true,  [&] { cursor1.insertText("c"); } },
                      { true,  [&] { cursor1.insertText("\t"); } }
                  });
    }

}
