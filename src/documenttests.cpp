#include "document.h"
#include "file.h"
#include <Tui/ZRoot.h>

#include "../third-party/catch.hpp"

#include <Tui/ZTerminal.h>

class DocumentTestHelper {
public:
    Document &getDoc(File *f) {
        return f->_doc;
    }
};

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
}