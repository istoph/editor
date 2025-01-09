// SPDX-License-Identifier: BSL-1.0

#include "catchwrapper.h"

#include <Tui/ZClipboard.h>
#include <Tui/ZDocument.h>
#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>
#include <Tui/ZTextMetrics.h>
#include <Tui/ZWindow.h>

#include "../file.h"
#include "eventrecorder.h"

class DocumentTestHelper {
public:
    Tui::ZDocument &getDoc(File *f) {
        return *f->document();
    }
    void f3(bool backward, Tui::ZTerminal *terminal, File *f) {
        bool t = GENERATE(true, false);
        CAPTURE(t);
        if (t) {
            if (backward) {
                Tui::ZTest::sendKey(terminal, Qt::Key_F3, Qt::KeyboardModifier::ShiftModifier);
            } else {
                Tui::ZTest::sendKey(terminal, Qt::Key_F3, Qt::KeyboardModifier::NoModifier);
            }
        } else {
            f->runSearch(backward);
        }
    }
};

TEST_CASE("file") {
    Tui::ZTerminal::OffScreen of(80, 24);
    Tui::ZTerminal terminal(of);
    Tui::ZRoot root;
    Tui::ZWindow *w = new Tui::ZWindow(&root);
    terminal.setMainWidget(&root);
    w->setGeometry({0, 0, 80, 24});

    File *f = new File(terminal.textMetrics(), w);
    f->setFocus();
    f->setGeometry({0, 0, 80, 24});

    DocumentTestHelper t;
    Tui::ZDocument &doc = t.getDoc(f);
    Tui::ZDocumentCursor cursor{&doc, [&terminal,&doc](int line, bool wrappingAllowed) {
            (void)wrappingAllowed;
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    //OHNE TEXT
    CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
    CHECK(f->hasSelection() == false);

    auto testCase = GENERATE(as<Qt::KeyboardModifiers>{}, Qt::KeyboardModifier::NoModifier, Qt::KeyboardModifier::AltModifier, Qt::KeyboardModifier::MetaModifier,
                             Qt::KeyboardModifier::ShiftModifier, Qt::KeyboardModifier::KeypadModifier, Qt::KeyboardModifier::ControlModifier,
                             Qt::KeyboardModifier::GroupSwitchModifier, Qt::KeyboardModifier::KeyboardModifierMask,
                             Qt::KeyboardModifier::ShiftModifier | Qt::KeyboardModifier::ControlModifier
                             //Qt::KeyboardModifier::ShiftModifier | Qt::KeyboardModifier::AltModifier //TODO: multi select
                             //Qt::KeyboardModifier::ShiftModifier | Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ControlModifier
                             );
    CAPTURE(testCase);

    SECTION("key-left") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-right") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-up") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-down") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Down, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-home") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-end") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_End, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-page-up") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageUp, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-page-down") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageDown, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-enter") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, testCase);
        if (Qt::KeyboardModifier::NoModifier == testCase || Qt::KeyboardModifier::KeypadModifier == testCase) {
            CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
        } else {
            CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        }
    }
    SECTION("key-tab-space") {
        f->setUseTabChar(false);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, testCase);
        if (Qt::KeyboardModifier::NoModifier == testCase) {
            CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,0});
            CHECK(doc.line(0) == "        ");
        } else {
            CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        }
    }
    SECTION("key-tab") {
        f->setUseTabChar(true);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, testCase);
        if (Qt::KeyboardModifier::NoModifier == testCase) {
            CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,0});
            CHECK(doc.line(0) == "\t");
        } else {
            CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        }
    }
    SECTION("key-insert") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Insert, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-delete") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-backspace") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Backspace, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
    SECTION("key-escape") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Escape, testCase);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
    }
}

TEST_CASE("file-getseter") {
    Tui::ZTerminal::OffScreen of(80, 24);
    Tui::ZTerminal terminal(of);
    Tui::ZRoot root;
    Tui::ZWindow *w = new Tui::ZWindow(&root);
    terminal.setMainWidget(&root);
    w->setGeometry({0, 0, 80, 24});

    File *f = new File(terminal.textMetrics(), w);

    DocumentTestHelper t;

    Tui::ZDocument &doc = t.getDoc(f);
    Tui::ZDocumentCursor cursor{&doc, [&terminal,&doc](int line, bool wrappingAllowed) {
            (void)wrappingAllowed;
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    CHECK(f->getFilename() == "");
    CHECK(f->tabStopDistance() == 8); //default?
    CHECK(f->useTabChar() == false);
    CHECK(f->eatSpaceBeforeTabs() == true);
    CHECK(f->formattingCharacters() == true);
    CHECK(f->colorTabs() == true); //default?
    CHECK(f->colorTrailingSpaces() == true); //default?
    CHECK(f->selectedText() == "");
    CHECK(f->hasSelection() == false);
    CHECK(f->overwriteMode() == false);
    CHECK(f->isModified() == false);
    CHECK(f->highlightBracket() == false);
    CHECK(f->attributesFile() == "");
    CHECK(f->document()->crLfMode() == false);
    CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
    CHECK(f->scrollPositionLine() == 0);
    CHECK(f->scrollPositionColumn() == 0);
    CHECK(f->rightMarginHint() == 0);
    CHECK(f->isNewFile() == false); //default?


    SECTION("getFilename") {
        f->setFilename("test");
        //TODO: set path
        //CHECK(f->getFilename() == "test");
    }
    SECTION("gotoline") {
        QString str = GENERATE("+", "");
        f->insertText("123\n123\n123");

        f->gotoLine(str + "0,0");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        f->gotoLine(str + "0,-1");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        f->gotoLine(str + "-1,0");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        f->gotoLine(str + "1,1");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        f->gotoLine(str + "2,2");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,1});
        f->gotoLine(str + "3,3");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,2});
        f->gotoLine(str + "4,4");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{3,2});
        f->gotoLine(str + "6500000,6500000");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{3,2});

        f->gotoLine(str + "1");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        f->gotoLine(str + "2");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
        f->gotoLine(str + "3");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,2});
        f->gotoLine(str + "4");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,2});
    }
    SECTION("tabsize") {
        for(int i = -1; i <= 10; i++) {
            CAPTURE(i);
            f->setTabStopDistance(i);
            CHECK(f->tabStopDistance() == std::max(1,i));
        }
    }
    SECTION("setTabOption") {
        f->setUseTabChar(true);
        CHECK(f->useTabChar() == true);
        f->setUseTabChar(false);
        CHECK(f->useTabChar() == false);
    }
    SECTION("setEatSpaceBeforeTabs") {
        f->setEatSpaceBeforeTabs(true);
        CHECK(f->eatSpaceBeforeTabs() == true);
        f->setEatSpaceBeforeTabs(false);
        CHECK(f->eatSpaceBeforeTabs() == false);
    }
    SECTION("setFormattingCharacters") {
        f->setFormattingCharacters(true);
        CHECK(f->formattingCharacters() == true);
        f->setFormattingCharacters(false);
        CHECK(f->formattingCharacters() == false);
    }
    SECTION("colorTabs") {
        f->setColorTabs(true);
        CHECK(f->colorTabs() == true);
        f->setColorTabs(false);
        CHECK(f->colorTabs() == false);
    }
    SECTION("colorSpaceEnd") {
        f->setColorTrailingSpaces(true);
        CHECK(f->colorTrailingSpaces() == true);
        f->setColorTrailingSpaces(false);
        CHECK(f->colorTrailingSpaces() == false);
    }
    SECTION("colorSpaceEnd") {
        Tui::ZTextOption::WrapMode wrap = GENERATE(Tui::ZTextOption::WrapMode::NoWrap,
                                                   Tui::ZTextOption::WrapMode::WordWrap,
                                                   Tui::ZTextOption::WrapMode::WrapAnywhere);
        f->setWordWrapMode(wrap);
        CHECK(f->wordWrapMode() == wrap);

    }
    SECTION("overwrite") {
        f->toggleOverwriteMode();
        CHECK(f->overwriteMode() == true);
        f->toggleOverwriteMode();
        CHECK(f->overwriteMode() == false);
    }
    SECTION("crLfMode") {
        f->document()->setCrLfMode(true);
        CHECK(f->document()->crLfMode() == true);
        f->document()->setCrLfMode(false);
        CHECK(f->document()->crLfMode() == false);
        //TODO check output file
    }
    SECTION("rightMarginHint") {
        for(int i = -1; i <= 10; i++) {
            CAPTURE(i);
            f->setRightMarginHint(i);
            CHECK(f->rightMarginHint() == std::max(0,i));
        }
    }
}

TEST_CASE("actions") {
    Tui::ZTerminal::OffScreen of(80, 24);
    Tui::ZTerminal terminal(of);
    Tui::ZRoot root;
    Tui::ZWindow *w = new Tui::ZWindow(&root);
    terminal.setMainWidget(&root);
    w->setGeometry({0, 0, 80, 24});

    File *f = new File(terminal.textMetrics(), w);

    DocumentTestHelper t;

    Tui::ZDocument &doc = t.getDoc(f);
    Tui::ZDocumentCursor cursor{&doc, [&terminal,&doc](int line, bool wrappingAllowed) {
            (void)wrappingAllowed;
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    f->setFocus();
    f->setGeometry({0, 0, 80, 24});

    CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
    Tui::ZTest::sendText(&terminal, "    text", Qt::KeyboardModifier::NoModifier);

    Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::KeyboardModifier::NoModifier);
    Tui::ZTest::sendText(&terminal, "    new1", Qt::KeyboardModifier::NoModifier);
    CHECK(doc.lineCount() == 2);

    SECTION("acv") {
        Tui::ZTest::sendText(&terminal, "a", Qt::KeyboardModifier::ControlModifier);
        Tui::ZTest::sendText(&terminal, "c", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        CHECK(doc.lineCount() == 2);
        Tui::ZTest::sendText(&terminal, "v", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        CHECK(doc.lineCount() == 2);
    }
    SECTION("acv") {
        Tui::ZTest::sendText(&terminal, "a", Qt::KeyboardModifier::ControlModifier);
        Tui::ZTest::sendText(&terminal, "c", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        CHECK(doc.lineCount() == 2);
        Tui::ZTest::sendText(&terminal, "v", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        CHECK(doc.lineCount() == 2);
    }
    SECTION("ac-right-v") {
        Tui::ZTest::sendText(&terminal, "a", Qt::KeyboardModifier::ControlModifier);
        Tui::ZTest::sendText(&terminal, "c", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        CHECK(doc.lineCount() == 2);
        CHECK(root.findFacet<Tui::ZClipboard>()->contents() == "    text\n    new1");
        Tui::ZTest::sendKey(&terminal, Tui::Key_Right, Qt::KeyboardModifier::NoModifier);
        Tui::ZTest::sendText(&terminal, "v", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,2});
        CHECK(doc.lineCount() == 3);
    }
    SECTION("axv") {
        Tui::ZTest::sendText(&terminal, "a", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        CHECK(f->hasSelection() == true);
        Tui::ZTest::sendText(&terminal, "x", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
        Tui::ZTest::sendText(&terminal, "v", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->hasSelection() == false);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
    }
    SECTION("cv-newline") {
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        Tui::ZTest::sendKey(&terminal, Tui::Key_Up, Qt::KeyboardModifier::NoModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,0});
        Tui::ZTest::sendKey(&terminal, Tui::Key_Right, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
        Tui::ZTest::sendText(&terminal, "c", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->hasSelection() == true);
        CHECK(root.findFacet<Tui::ZClipboard>()->contents() == "\n");

        CHECK(doc.lineCount() == 2);
        Tui::ZTest::sendText(&terminal, "v", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
        CHECK(doc.lineCount() == 2);
        CHECK(f->hasSelection() == false);

        Tui::ZTest::sendText(&terminal, "v", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,2});
        CHECK(doc.lineCount() == 3);
        CHECK(doc.line(1) == "");
    }

    SECTION("sort") {
        // Alt + Shift + s sort selected lines
        //CAPTURE(doc.line(1));
        CHECK(doc.lineCount() == 2);
        CHECK(doc.line(1) == "    new1");
        Tui::ZTest::sendText(&terminal, "a", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->hasSelection() == true);
        Tui::ZTest::sendText(&terminal, "S", Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(doc.line(0) == "    new1");
        CHECK(doc.lineCount() == 2);
        CHECK(f->hasSelection() == true);
    }
    SECTION("sort-231") {
        f->newText("123");
        f->insertText("3\n2\n1");
        CHECK(doc.lineCount() == 3);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,2});
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageUp, Qt::KeyboardModifier::NoModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::NoModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Down, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
        CHECK(f->hasSelection() == true);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Down, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,2});
        CHECK(f->hasSelection() == true);
        CHECK(f->selectedText() == "3\n2\n");
        Tui::ZTest::sendText(&terminal, "S", Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(doc.line(0) == "2");
        CHECK(doc.line(1) == "3");
        CHECK(doc.line(2) == "1");
    }
    SECTION("sort-312") {
        f->newText("123");
        f->insertText("3\n2\n1");
        CHECK(doc.lineCount() == 3);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,2});
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageUp, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == true);
        CHECK(f->selectedText() == "3\n2\n1");
        Tui::ZTest::sendText(&terminal, "S", Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(doc.line(0) == "1");
        CHECK(doc.line(1) == "2");
        CHECK(doc.line(2) == "3");
    }
    SECTION("sort-3") {
        f->newText("123");
        f->insertText("3\n2\n1");
        CHECK(doc.lineCount() == 3);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,2});
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageUp, Qt::KeyboardModifier::NoModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == false);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,0});
        CHECK(f->hasSelection() == true);
        CHECK(f->selectedText() == "3");
        Tui::ZTest::sendText(&terminal, "S", Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(doc.line(0) == "3");
        CHECK(doc.line(1) == "2");
        CHECK(doc.line(2) == "1");
    }

    //delete
    SECTION("key-delete") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::KeyboardModifier::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::KeyboardModifier::NoModifier);
        CHECK(doc.line(1) == "  new1");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
    }
    SECTION("key-delete-remove-line") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, Qt::KeyboardModifier::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::KeyboardModifier::NoModifier);
        CHECK(doc.line(0) == "    text    new1");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,0});
    }
    SECTION("ctrl+key-delete") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::KeyboardModifier::ControlModifier);
        CHECK(doc.line(1) == "");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
    }
    SECTION("shift+key-delete") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::KeyboardModifier::ShiftModifier);
        CHECK(doc.line(1) == "    new1");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
    }
    SECTION("alt+key-delete") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::KeyboardModifier::AltModifier);
        CHECK(doc.line(1) == "    new1");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
    }
    //backspace
    SECTION("key-backspace") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Backspace, Qt::KeyboardModifier::NoModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{7,1});
        CHECK(doc.line(1) == "    new");
    }
    SECTION("ctrl+key-backspace") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Backspace, Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4,1});
        CHECK(doc.line(1) == "    ");
    }
    SECTION("alt+key-backspace") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Backspace, Qt::KeyboardModifier::AltModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        CHECK(doc.line(1) == "    new1");
    }
    SECTION("key-left") {
        for(int i = 0; i <= 8; i++) {
            CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8 - i, 1});
            Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::NoModifier);
        }
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8, 0});
    }

    //left
    SECTION("crl+key-left") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4, 1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0, 1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8, 0});
    }

    SECTION("crl+shift+key-left") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4, 1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0, 1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8, 0});
    }

    //right
    SECTION("crl+key-right") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::ControlModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8, 0});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0, 1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8, 1});
    }
    SECTION("crl+key-right") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::ControlModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8, 0});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0, 1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8, 1});
    }


    SECTION("scroll") {
        auto testCase = GENERATE(Tui::ZTextOption::WrapMode::NoWrap, Tui::ZTextOption::WrapMode::WordWrap, Tui::ZTextOption::WrapMode::WrapAnywhere);
        f->setWordWrapMode(testCase);
        CHECK(f->scrollPositionColumn() == 0);
        CHECK(f->scrollPositionLine() == 0);

        //TODO: #193 scrollup with Crl+Up and wraped lines.
        //Tui::ZTest::sendText(&terminal, QString("a").repeated(100), Qt::KeyboardModifier::NoModifier);

        for (int i = 0; i < 48; i++) {
            Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::KeyboardModifier::NoModifier);
        }
        CHECK(f->scrollPositionColumn() == 0);
        CHECK(f->scrollPositionLine() == 27);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,49});
        for (int i = 0; i <= 26; i++) {
            Tui::ZTest::sendKey(&terminal, Qt::Key_Up, Qt::KeyboardModifier::ControlModifier);
            CHECK(f->scrollPositionColumn() == 0);
            CHECK(f->scrollPositionLine() == 26 - i);
        }
    }

    SECTION("move-lines-up") {
        CHECK(doc.lineCount() == 2);
        CHECK(f->scrollPositionColumn() == 0);
        CHECK(f->scrollPositionLine() == 0);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, (Qt::KeyboardModifier::ShiftModifier | Qt::KeyboardModifier::ControlModifier));
        CHECK(doc.line(0) == "    new1");
    }
    SECTION("move-lines-down") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, Qt::KeyboardModifier::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Down, (Qt::KeyboardModifier::ShiftModifier | Qt::KeyboardModifier::ControlModifier));
        CHECK(doc.line(0) == "    new1");
    }

    SECTION("select-tab") {
        Tui::ZTest::sendText(&terminal, "a", Qt::KeyboardModifier::ControlModifier);
        CHECK(f->hasSelection() == true);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::KeyboardModifier::ShiftModifier);
        CHECK(doc.line(0) == "text");
        CHECK(doc.line(1) == "new1");
        Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::KeyboardModifier::NoModifier);
        CHECK(doc.line(0) == "        text");
        CHECK(doc.line(1) == "        new1");
    }
    SECTION("tab") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::KeyboardModifier::ShiftModifier);
        CHECK(doc.line(0) == "    text");
        CHECK(doc.line(1) == "new1");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4, 1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::KeyboardModifier::NoModifier);
        CHECK(doc.line(1) == "new1    ");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8, 1});
    }

    //home end
    SECTION("home-home") {
        Qt::KeyboardModifier shift = GENERATE(Qt::KeyboardModifier::NoModifier, Qt::KeyboardModifier::ShiftModifier);
        CAPTURE(shift);

        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
    }

    SECTION("home-end") {
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::NoModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_End, Qt::KeyboardModifier::NoModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
    }
    SECTION("shift+home-end") {
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_End, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
    }
    SECTION("crl+home-end") {
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        Tui::ZTest::sendKey(&terminal, Qt::Key_End, Qt::KeyboardModifier::ControlModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
    }
    SECTION("crl+shift+home-end") {
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, (Qt::KeyboardModifier::ShiftModifier | Qt::KeyboardModifier::ControlModifier));
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        Tui::ZTest::sendKey(&terminal, Qt::Key_End, (Qt::KeyboardModifier::ShiftModifier | Qt::KeyboardModifier::ControlModifier));
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
    }

    SECTION("up-down-page-up-page-down") {
        Qt::KeyboardModifier shift = GENERATE(Qt::KeyboardModifier::NoModifier, Qt::KeyboardModifier::ShiftModifier);
        CAPTURE(shift);

        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::KeyboardModifier::NoModifier);
        Tui::ZTest::sendText(&terminal, "    new2", Qt::KeyboardModifier::NoModifier);
        CHECK(doc.lineCount() == 3);

        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,2});
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageUp, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageDown, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,2});
    }

    //esc
    SECTION("esc") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, (Qt::KeyboardModifier::ShiftModifier | Qt::KeyboardModifier::ControlModifier));
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == true);

        Tui::ZTest::sendKey(&terminal, Qt::Key_Escape, Qt::KeyboardModifier::NoModifier);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
        CHECK(f->hasSelection() == true);
    }

    //eat space
    SECTION("eat space") {
        f->setTabStopDistance(4);
        Tui::ZTest::sendText(&terminal, "a", Qt::KeyboardModifier::ControlModifier);

        //prepare enviromend
        for(int i = 1; i <= 6; i++) {
            for(int space = 1; space < i; space++) {
                Tui::ZTest::sendKey(&terminal, Qt::Key_Space, Qt::KeyboardModifier::NoModifier);
            }
            Tui::ZTest::sendText(&terminal, "a", Qt::KeyboardModifier::NoModifier);
            Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::KeyboardModifier::NoModifier);
        }
        CHECK(doc.line(0) == "a");
        CHECK(doc.line(1) == " a");
        CHECK(doc.line(2) == "  a");
        CHECK(doc.line(3) == "   a");
        CHECK(doc.line(4) == "    a");
        CHECK(doc.line(5) == "     a");

        SECTION("tab") {
            Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::ControlModifier);
            for(int i = 1; i <= 6; i++) {
                Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::KeyboardModifier::NoModifier);
                Tui::ZTest::sendKey(&terminal, Qt::Key_Down, Qt::KeyboardModifier::NoModifier);
                Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::NoModifier);
            }
            CHECK(doc.line(0) == "    a");
            CHECK(doc.line(1) == "    a");
            CHECK(doc.line(2) == "    a");
            CHECK(doc.line(3) == "    a");
            CHECK(doc.line(4) == "        a");
            CHECK(doc.line(5) == "        a");
        }
        SECTION("shift+tab") {
            Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::ControlModifier);
            for(int i = 1; i <= 6; i++) {
                Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::KeyboardModifier::ShiftModifier);
                Tui::ZTest::sendKey(&terminal, Qt::Key_Down, Qt::KeyboardModifier::NoModifier);
            }
            CHECK(doc.line(0) == "a");
            CHECK(doc.line(1) == "a");
            CHECK(doc.line(2) == "a");
            CHECK(doc.line(3) == "a");
            CHECK(doc.line(4) == "a");
            CHECK(doc.line(5) == " a");
        }
    }

    //page up down
    SECTION("page") {
        //80x24 terminal
        //f->setGeometry({0, 0, 80, 24});

        Tui::ZTest::sendText(&terminal, "a", Qt::KeyboardModifier::ControlModifier);
        Qt::KeyboardModifier shift = GENERATE(Qt::KeyboardModifier::NoModifier, Qt::KeyboardModifier::ShiftModifier);
        CAPTURE(shift);

        //prepare enviromend
        for(int i = 1; i <= 50; i++) {
            Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::KeyboardModifier::NoModifier);
        }

        Tui::ZTest::sendKey(&terminal, Qt::Key_Home, Qt::KeyboardModifier::ControlModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageDown, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,23});
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageDown, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,46});
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageDown, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,50});

        Tui::ZTest::sendKey(&terminal, Qt::Key_PageUp, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,27});
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageUp, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,4});
        Tui::ZTest::sendKey(&terminal, Qt::Key_PageUp, shift);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{0,0});
    }

    SECTION("search") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        f->setSearchText("t");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(false, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{5,0});

        recorder.clearEvents();
        t.f3(false, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,0});

        recorder.clearEvents();
        t.f3(false, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{5,0});
        recorder.clearEvents();
    }
    SECTION("search-t") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool backward = GENERATE(true, false);
        CAPTURE(backward);
        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        CAPTURE(backward);

        f->selectAll();
        f->insertText("t");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,0});
        CHECK(f->hasSelection() == false);
        recorder.clearEvents();

        f->setSearchText("t");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        /*
        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);
        */
    }
    SECTION("search-t-t") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool backward = GENERATE(true, false);
        CAPTURE(backward);
        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);


        f->selectAll();
        f->insertText("t t");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{3,0});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->hasSelection() == true);
        recorder.clearEvents();

        f->setSearchText("t");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{3,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);
    }

    SECTION("search-space-t-t") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText(" t t ");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{5,0});
        CHECK(f->hasSelection() == false);
        recorder.clearEvents();

        f->setSearchText("t");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(false, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(false, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(false, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);
    }

    SECTION("searchBackword-space-t-t") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText(" t t ");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{5,0});
        CHECK(f->hasSelection() == false);
        recorder.clearEvents();

        f->setSearchText("t");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(true, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(true, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(true, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);
    }

    SECTION("search-t-newline-t") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool backward = GENERATE(true, false);
        CAPTURE(backward);
        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText("t\nt");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->hasSelection() == true);
        recorder.clearEvents();

        f->setSearchText("t");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,1});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{1,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);
    }

    SECTION("search-t-newline-space-t") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool backward = GENERATE(true, false);
        CAPTURE(backward);
        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText(" t\n t");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->hasSelection() == true);
        recorder.clearEvents();

        f->setSearchText("t");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,1});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);
    }

    SECTION("search-aa") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool backward = GENERATE(true, false);
        CAPTURE(backward);
        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText(" aa\naa");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->hasSelection() == true);
        recorder.clearEvents();

        f->setSearchText("aa");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{3,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,1});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);
    }

    SECTION("search-aa-aa") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool backward = GENERATE(true, false);
        CAPTURE(backward);
        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText(" aa aa\naa");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->hasSelection() == true);
        recorder.clearEvents();

        f->setSearchText("aa");

        QList<Tui::ZDocumentCursor::Position> positions;

        if (backward) {
            positions << Tui::ZDocumentCursor::Position{6,0} << Tui::ZDocumentCursor::Position{3,0} << Tui::ZDocumentCursor::Position{2,1};
        } else {
            positions << Tui::ZDocumentCursor::Position{3,0} << Tui::ZDocumentCursor::Position{6,0} << Tui::ZDocumentCursor::Position{2,1};
        }

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        for (Tui::ZDocumentCursor::Position point : positions) {
            t.f3(backward, &terminal, f);
            recorder.waitForEvent(cursorSignal);
            CHECK(f->cursorPosition() == point);
            recorder.clearEvents();
            CHECK(f->hasSelection() == true);
        }
    }

    SECTION("search-asd") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText("asd \n\nasd");
        f->setCursorPosition(Tui::ZDocumentCursor::Position{4,0});
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4,0});
        CHECK(f->hasSelection() == false);
        recorder.clearEvents();

        f->setSearchText("asd");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(true, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{3,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

    }

    SECTION("search-forward-backward") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText("bb\nbb\nbb");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,2});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->hasSelection() == true);
        recorder.clearEvents();

        f->setSearchText("bb");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();

        SECTION("toggel-wraparound") {
            for(int i = 0; i < 3; i++) {
                t.f3(false, &terminal, f);
                recorder.waitForEvent(cursorSignal);
                CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,0});
                recorder.clearEvents();
                CHECK(f->hasSelection() == true);

                t.f3(true, &terminal, f);
                recorder.waitForEvent(cursorSignal);
                CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,2});
                recorder.clearEvents();
                CHECK(f->hasSelection() == true);
            }
        }

        SECTION("without-wraparound") {
            QList<Tui::ZDocumentCursor::Position> positions;

            positions << Tui::ZDocumentCursor::Position{2,1} << Tui::ZDocumentCursor::Position{2,0} << Tui::ZDocumentCursor::Position{2,1} << Tui::ZDocumentCursor::Position{2,2} << Tui::ZDocumentCursor::Position{2,0} << Tui::ZDocumentCursor::Position{2,2} << Tui::ZDocumentCursor::Position{2,1};
            bool backward = true;
            for (Tui::ZDocumentCursor::Position point : positions) {
                t.f3(backward, &terminal, f);
                recorder.waitForEvent(cursorSignal);
                CHECK(f->cursorPosition() == point);
                recorder.clearEvents();
                CHECK(f->hasSelection() == true);

                if (point.line == 0) backward = !backward;
            }
        }
    }


    SECTION("search-smiley") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool backward = GENERATE(true, false);
        CAPTURE(backward);
        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText("😎😎");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4,0});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->hasSelection() == true);
        recorder.clearEvents();

        f->setSearchText("😎");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{4,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);
    }

    SECTION("search-smiley-multiline") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));

        bool backward = GENERATE(true, false);
        CAPTURE(backward);
        bool reg = GENERATE(true, false);
        CAPTURE(reg);
        f->setRegex(reg);

        f->selectAll();
        f->insertText("😎\n😎");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,1});
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::KeyboardModifier::ShiftModifier);
        CHECK(f->hasSelection() == true);
        recorder.clearEvents();

        f->setSearchText("😎");

        recorder.waitForEvent(cursorSignal);
        recorder.clearEvents();
        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,0});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);

        t.f3(backward, &terminal, f);
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,1});
        recorder.clearEvents();
        CHECK(f->hasSelection() == true);
    }

    SECTION("replace") {
        EventRecorder recorder;
        auto cursorSignal = recorder.watchSignal(f, RECORDER_SIGNAL(&File::cursorPositionChanged));
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});

        f->replaceAll("1","2");
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{8,1});
        CHECK(doc.line(1) == "    new2");
        recorder.clearEvents();

        f->replaceAll("e","E");
        recorder.waitForEvent(cursorSignal);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{6,1});
        recorder.clearEvents();

        f->replaceAll(" ","   ");
        recorder.waitForEvent(cursorSignal);
        CHECK(doc.line(0) == "            tExt");
        CHECK(doc.line(1) == "            nEw2");
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{12,1});
        recorder.clearEvents();
    }
}

TEST_CASE("multiline") {
    Tui::ZTerminal::OffScreen of(80, 24);
    Tui::ZTerminal terminal(of);
    Tui::ZRoot root;
    Tui::ZWindow *w = new Tui::ZWindow(&root);
    terminal.setMainWidget(&root);
    w->setGeometry({0, 0, 80, 24});

    File *f = new File(terminal.textMetrics(), w);
    f->setFocus();
    f->setGeometry({0, 0, 80, 24});

    DocumentTestHelper t;
    Tui::ZDocument &doc = t.getDoc(f);
    Tui::ZDocumentCursor cursor{&doc, [&terminal,&doc](int line, bool wrappingAllowed) {
            (void)wrappingAllowed;
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    f->insertText("abc\ndef\nghi\njkl");
    f->setCursorPosition({2,1});

    SECTION("select-up") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == true);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,1}); //TODO: position 2,0
        CHECK(f->anchorPosition() == Tui::ZDocumentCursor::Position{2,1});
    }
    SECTION("select-down") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Down, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == true);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,1}); //TODO: position 2,2
        CHECK(f->anchorPosition() == Tui::ZDocumentCursor::Position{2,1});
    }
    SECTION("select-up-left") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == true);
        //left
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == true);
        CHECK(f->hasMultiInsert() == false);
    }
    SECTION("select-down-rgiht") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Down, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == true);
        //right
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == true);
        CHECK(f->hasMultiInsert() == false);
    }
    SECTION("select-down-rgiht-key") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Down, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == true);
        CHECK(f->hasMultiInsert() == false);

        SECTION("add-a") {
            Tui::ZTest::sendText(&terminal, "A", Qt::KeyboardModifier::NoModifier);
            CHECK(f->hasSelection() == true);
            CHECK(f->hasBlockSelection() == false);
            CHECK(f->hasMultiInsert() == true);
            CHECK(doc.line(0) == "abc");
            CHECK(doc.line(1) == "deA");
            CHECK(doc.line(2) == "ghA");
            CHECK(doc.line(3) == "jkl");
        }
        SECTION("delete") {
            Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::KeyboardModifier::NoModifier);
            CHECK(f->hasSelection() == false);
            CHECK(f->hasBlockSelection() == false);
            CHECK(f->hasMultiInsert() == false);
            CHECK(doc.line(0) == "abc");
            CHECK(doc.line(1) == "de");
            CHECK(doc.line(2) == "gh");
            CHECK(doc.line(3) == "jkl");
        }
        SECTION("backspace") {
            Tui::ZTest::sendKey(&terminal, Qt::Key_Backspace, Qt::KeyboardModifier::NoModifier);
            CHECK(f->hasSelection() == false);
            CHECK(f->hasBlockSelection() == false);
            CHECK(f->hasMultiInsert() == false);
            CHECK(doc.line(0) == "abc");
            CHECK(doc.line(1) == "de");
            CHECK(doc.line(2) == "gh");
            CHECK(doc.line(3) == "jkl");
        }
    }
    SECTION("select-up-left") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == true);

        SECTION("add-a") {
            Tui::ZTest::sendText(&terminal, "A", Qt::KeyboardModifier::NoModifier);
            CHECK(f->hasSelection() == true);
            CHECK(f->hasBlockSelection() == false);
            CHECK(f->hasMultiInsert() == true);
            CHECK(doc.line(0) == "abAc");
            CHECK(doc.line(1) == "deAf");
            CHECK(doc.line(2) == "ghi");
            CHECK(doc.line(3) == "jkl");
        }
        SECTION("delete") {
            Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::KeyboardModifier::NoModifier);
            CHECK(f->hasSelection() == true);
            CHECK(f->hasBlockSelection() == false);
            CHECK(f->hasMultiInsert() == true);
            CHECK(doc.line(0) == "ab");
            CHECK(doc.line(1) == "de");
            CHECK(doc.line(2) == "ghi");
            CHECK(doc.line(3) == "jkl");
        }
        SECTION("backspace") {
            Tui::ZTest::sendKey(&terminal, Qt::Key_Backspace, Qt::KeyboardModifier::NoModifier);
            CHECK(f->hasSelection() == true);
            CHECK(f->hasBlockSelection() == false);
            CHECK(f->hasMultiInsert() == true);
            CHECK(doc.line(0) == "ac");
            CHECK(doc.line(1) == "df");
            CHECK(doc.line(2) == "ghi");
            CHECK(doc.line(3) == "jkl");
        }
    }
    SECTION("select-up-down") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == true);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Down, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == true);
    }
    SECTION("select-and-esc") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == true);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Escape, Qt::KeyboardModifier::NoModifier);
        CHECK(f->hasSelection() == false);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == false);
        CHECK(f->cursorPosition() == Tui::ZDocumentCursor::Position{2,0});
    }
    SECTION("select-left-right") {
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == true);
        CHECK(f->hasMultiInsert() == false);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, (Qt::KeyboardModifier::AltModifier | Qt::KeyboardModifier::ShiftModifier));
        CHECK(f->hasSelection() == true);
        CHECK(f->hasBlockSelection() == false);
        CHECK(f->hasMultiInsert() == true);
    }
}

