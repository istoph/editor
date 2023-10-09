// SPDX-License-Identifier: BSL-1.0

#include "document.h"

#include <optional>
#include <random>

#include <QBuffer>
#include <QElapsedTimer>
#include <QThreadPool>

#include "catchwrapper.h"

#include "eventrecorder.h"

#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>
#include <Tui/ZTextMetrics.h>
#include <Tui/ZTextOption.h>

static QVector<QString> docToVec(const Document &doc) {
    QVector<QString> ret;

    for (int i = 0; i < doc.lineCount(); i++) {
        ret.append(doc.line(i));
    }

    return ret;
}

static QVector<QString> snapToVec(const DocumentSnapshot &snap) {
    QVector<QString> ret;

    for (int i = 0; i < snap.lineCount(); i++) {
        ret.append(snap.line(i));
    }

    return ret;
}

namespace {
    class TestUserData : public UserData {

    };
}

TEST_CASE("Document") {
    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);

    Document doc;

    TextCursor cursor{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    TextCursor wrappedCursor{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            if (wrappingAllowed) {
                Tui::ZTextOption option;
                option.setWrapMode(Tui::ZTextOption::WrapMode::WordWrap);
                lay.setTextOption(option);
            }
            lay.doLayout(40);
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

    SECTION("insert-empty") {
        cursor.insertText("");
        REQUIRE(doc.lineCount() == 1);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
    }

    SECTION("insert-empty-line-and-text") {
        cursor.insertText("\ntest test");
        REQUIRE(doc.lineCount() == 2);
        CHECK(cursor.position() == TextCursor::Position{9, 1});
    }

    SECTION("insert-lines") {
        cursor.insertText("test test");
        REQUIRE(doc.lineCount() == 1);
        CHECK(cursor.position() == TextCursor::Position{9, 0});
        cursor.selectAll();

        cursor.insertText("test test\ntest test");
        REQUIRE(doc.lineCount() == 2);
        CHECK(cursor.position() == TextCursor::Position{9, 1});
        cursor.selectAll();

        cursor.insertText("test test\ntest test\ntest test\n");
        REQUIRE(doc.lineCount() == 4);
        CHECK(cursor.position() == TextCursor::Position{0, 3});
        cursor.selectAll();

        cursor.insertText("test test\ntest test\n");
        cursor.insertText("test test\ntest test");
        REQUIRE(doc.lineCount() == 4);
        CHECK(cursor.position() == TextCursor::Position{9, 3});
        cursor.selectAll();
    }

    SECTION("insert-and-selection") {
        cursor.insertText("test test");
        REQUIRE(doc.lineCount() == 1);
        CHECK(cursor.position() == TextCursor::Position{9, 0});
        cursor.moveWordLeft(true);
        CHECK(cursor.position() == TextCursor::Position{5, 0});
        cursor.insertText("new new");

        QVector<QString> test;
        test.append("test new new");
        CHECK(cursor.position() == TextCursor::Position{12, 0});
        CHECK(docToVec(doc) == test);

        cursor.moveWordLeft(true);
        cursor.moveWordLeft(true);
        cursor.moveWordLeft(true);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        cursor.insertText("\nold");
    }

    SECTION("insert contentsChanged signal") {
        cursor.insertText("test test");

        // signal emitted async

        EventRecorder recorder;
        auto changedSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::contentsChanged));

        recorder.waitForEvent(changedSignal);
        CHECK(recorder.consumeFirst(changedSignal));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("insert modificationChanged signal") {
        EventRecorder recorder;

        // signal emited sync
        auto modificationChangedSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::modificationChanged));

        cursor.insertText("test test");

        recorder.waitForEvent(modificationChangedSignal);
        CHECK(recorder.consumeFirst(modificationChangedSignal, true));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("insert undoAvailable signal") {
        EventRecorder recorder;

        // signal emited sync
        auto undoAvailableSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::undoAvailable));

        cursor.insertText("test test");

        recorder.waitForEvent(undoAvailableSignal);
        CHECK(recorder.consumeFirst(undoAvailableSignal, true));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("position and anchor") {
        cursor.insertText("1234\nabcdefgh\nABCDE");

        SECTION("basic") {
            cursor.setPosition({3, 0});
            CHECK(cursor.anchor() == TextCursor::Position{3, 0});
            CHECK(cursor.position() == TextCursor::Position{3, 0});
            CHECK(cursor.hasSelection() == false);
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{3, 0});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{3, 0});
        }

        SECTION("out of range code unit") {
            cursor.setPosition({5, 0});
            CHECK(cursor.anchor() == TextCursor::Position{4, 0});
            CHECK(cursor.position() == TextCursor::Position{4, 0});
            CHECK(cursor.hasSelection() == false);
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{4, 0});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{4, 0});
        }

        SECTION("out of range line") {
            cursor.setPosition({5, 3});
            CHECK(cursor.anchor() == TextCursor::Position{5, 2});
            CHECK(cursor.position() == TextCursor::Position{5, 2});
            CHECK(cursor.hasSelection() == false);
            CHECK(cursor.verticalMovementColumn() == 5);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{5, 2});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{5, 2});
        }

        SECTION("vertical movement column") {
            cursor.setPosition({3, 0});
            cursor.setVerticalMovementColumn(5);
            CHECK(cursor.anchor() == TextCursor::Position{3, 0});
            CHECK(cursor.position() == TextCursor::Position{3, 0});
            CHECK(cursor.hasSelection() == false);
            CHECK(cursor.verticalMovementColumn() == 5);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{3, 0});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{3, 0});
        }

        SECTION("selection by extend") {
            cursor.setPosition({3, 0});
            cursor.setPosition({1, 1}, true);
            CHECK(cursor.anchor() == TextCursor::Position{3, 0});
            CHECK(cursor.position() == TextCursor::Position{1, 1});
            CHECK(cursor.hasSelection() == true);
            CHECK(cursor.verticalMovementColumn() == 1);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{3, 0});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{1, 1});
        }

        SECTION("selection by extend - reversed") {
            cursor.setPosition({1, 1});
            cursor.setPosition({3, 0}, true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 1});
            CHECK(cursor.position() == TextCursor::Position{3, 0});
            CHECK(cursor.hasSelection() == true);
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{3, 0});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{1, 1});
        }

        SECTION("selection by setAnchor") {
            cursor.setPosition({1, 1});
            cursor.setAnchorPosition({3, 0});
            CHECK(cursor.anchor() == TextCursor::Position{3, 0});
            CHECK(cursor.position() == TextCursor::Position{1, 1});
            CHECK(cursor.hasSelection() == true);
            CHECK(cursor.verticalMovementColumn() == 1);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{3, 0});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{1, 1});
        }

        SECTION("selection by setAnchor - reversed") {
            cursor.setPosition({3, 0});
            cursor.setAnchorPosition({1, 1});
            CHECK(cursor.anchor() == TextCursor::Position{1, 1});
            CHECK(cursor.position() == TextCursor::Position{3, 0});
            CHECK(cursor.hasSelection() == true);
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{3, 0});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{1, 1});
        }

        SECTION("setVerticalMovementColumn") {
            cursor.setPosition({1, 1});
            cursor.setVerticalMovementColumn(100);
            CHECK(cursor.anchor() == TextCursor::Position{1, 1});
            CHECK(cursor.position() == TextCursor::Position{1, 1});
            CHECK(cursor.hasSelection() == false);
            CHECK(cursor.verticalMovementColumn() == 100);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{1, 1});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{1, 1});
        }

        SECTION("setPositionPreservingVerticalMovementColumn") {
            cursor.setPosition({1, 1});
            cursor.setPositionPreservingVerticalMovementColumn({3, 1});
            CHECK(cursor.anchor() == TextCursor::Position{3, 1});
            CHECK(cursor.position() == TextCursor::Position{3, 1});
            CHECK(cursor.hasSelection() == false);
            CHECK(cursor.verticalMovementColumn() == 1);
            CHECK(cursor.selectionStartPos() == TextCursor::Position{3, 1});
            CHECK(cursor.selectionEndPos() == TextCursor::Position{3, 1});
        }
    }

    SECTION("reset") {
        cursor.insertText("\ntest test");
        REQUIRE(doc.lineCount() == 2);
        CHECK(cursor.position() == TextCursor::Position{9, 1});
        LineMarker marker{&doc, 1};

        doc.reset();

        CHECK(doc.lineCount() == 1);
        CHECK(doc.lineCodeUnits(0) == 0);
        CHECK(doc.isModified() == false);
        CHECK(doc.isUndoAvailable() == false);
        CHECK(doc.isRedoAvailable() == false);

        CHECK(cursor.position() == TextCursor::Position{0, 0});
        CHECK(marker.line() == 0);
    }

    SECTION("reset contentsChanged signal") {
        cursor.insertText("\ntest test");

        // signal emitted async, so needs to be consumed
        EventRecorder recorder;
        auto changedSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::contentsChanged));

        recorder.waitForEvent(changedSignal);
        CHECK(recorder.consumeFirst(changedSignal));
        CHECK(recorder.noMoreEvents());

        doc.reset();
        recorder.waitForEvent(changedSignal);
        CHECK(recorder.consumeFirst(changedSignal));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("reset cursorChanged signal") {
        // brittle: The order of signals emitted for changed cursors in not specified
        cursor.insertText("\ntest test");
        REQUIRE(doc.lineCount() == 2);
        CHECK(cursor.position() == TextCursor::Position{9, 1});

        // signal emitted async, so needs to be consumed
        EventRecorder recorder;
        auto cursorChangedSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::cursorChanged));

        recorder.waitForEvent(cursorChangedSignal);
        CHECK(recorder.consumeFirst(cursorChangedSignal, (const TextCursor*)&cursor));
        CHECK(recorder.consumeFirst(cursorChangedSignal, (const TextCursor*)&wrappedCursor));
        CHECK(recorder.noMoreEvents());

        doc.reset();
        recorder.waitForEvent(cursorChangedSignal);
        CHECK(recorder.consumeFirst(cursorChangedSignal, (const TextCursor*)&cursor));
        CHECK(recorder.consumeFirst(cursorChangedSignal, (const TextCursor*)&wrappedCursor));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("reset lineMarkerChanged signal") {
        cursor.insertText("\ntest test");
        LineMarker marker{&doc, 1};

        // signal emitted async
        EventRecorder recorder;
        auto lineMarkerChangedSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::lineMarkerChanged));

        doc.reset();
        recorder.waitForEvent(lineMarkerChangedSignal);
        CHECK(recorder.consumeFirst(lineMarkerChangedSignal, (const LineMarker*)&marker));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("reset undoAvailable signal") {
        cursor.insertText("\ntest test");

        EventRecorder recorder;
        auto undoAvailableSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::undoAvailable));

        doc.reset();
        recorder.waitForEvent(undoAvailableSignal);
        CHECK(recorder.consumeFirst(undoAvailableSignal, false));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("reset redoAvailable signal") {
        cursor.insertText("\ntest test");
        cursor.insertText("\ntest test");
        doc.undo(&cursor);

        EventRecorder recorder;
        auto redoAvailableSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::redoAvailable));

        doc.reset();
        recorder.waitForEvent(redoAvailableSignal);
        CHECK(recorder.consumeFirst(redoAvailableSignal, false));
        CHECK(recorder.noMoreEvents());
    }


    SECTION("reset modificationChanged signal") {
        cursor.insertText("\ntest test");

        EventRecorder recorder;
        auto modificationChangedSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::modificationChanged));

        doc.reset();
        recorder.waitForEvent(modificationChangedSignal);
        CHECK(recorder.consumeFirst(modificationChangedSignal, false));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("readFrom - writeTo") {
        QByteArray inData;
        QBuffer inFile(&inData);
        REQUIRE(inFile.open(QIODevice::ReadOnly));

        QByteArray outData;
        QBuffer outFile(&outData);
        REQUIRE(outFile.open(QIODevice::WriteOnly));

        SECTION("empty") {
            bool allLinesCrLf = doc.readFrom(&inFile);
            CHECK(docToVec(doc) == QVector<QString>{ "" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == true);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData.size() == 0);
        }

        SECTION("simple") {
            inData = QByteArray("line1\nline2\n");
            bool allLinesCrLf = doc.readFrom(&inFile);
            CHECK(docToVec(doc) == QVector<QString>{ "line1", "line2" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == false);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("initial cursor position") {
            inData = QByteArray("line1\nline2\n");
            bool allLinesCrLf = doc.readFrom(&inFile, TextCursor::Position{1, 1}, &cursor);
            CHECK(docToVec(doc) == QVector<QString>{ "line1", "line2" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == false);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);
            CHECK(cursor.position() == TextCursor::Position{1, 1});
            CHECK(cursor.hasSelection() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("initial cursor position with out of range code unit") {
            inData = QByteArray("line1\nline2\n");
            bool allLinesCrLf = doc.readFrom(&inFile, TextCursor::Position{10, 1}, &cursor);
            CHECK(docToVec(doc) == QVector<QString>{ "line1", "line2" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == false);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);
            CHECK(cursor.position() == TextCursor::Position{5, 1});
            CHECK(cursor.hasSelection() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("initial cursor position with out of range line") {
            inData = QByteArray("line1\nline2\n");
            bool allLinesCrLf = doc.readFrom(&inFile, TextCursor::Position{2, 10}, &cursor);
            CHECK(docToVec(doc) == QVector<QString>{ "line1", "line2" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == false);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);
            CHECK(cursor.position() == TextCursor::Position{2, 1});
            CHECK(cursor.hasSelection() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("resets cursors and line markers") {
            cursor.insertText("\ntest test");
            REQUIRE(doc.lineCount() == 2);
            CHECK(cursor.position() == TextCursor::Position{9, 1});
            LineMarker marker{&doc, 1};

            inData = QByteArray("line1\nline2\n");
            doc.readFrom(&inFile);

            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(marker.line() == 0);
        }

        SECTION("simple - missing last linebreak") {
            inData = QByteArray("line1\nline2");
            bool allLinesCrLf = doc.readFrom(&inFile);
            CHECK(docToVec(doc) == QVector<QString>{ "line1", "line2" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == true);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("mixed line endings") {
            inData = QByteArray("line1\r\nline2\nline3\n");
            bool allLinesCrLf = doc.readFrom(&inFile);
            CHECK(docToVec(doc) == QVector<QString>{ "line1\r", "line2", "line3" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == false);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }


        SECTION("mixed line endings - last line missing CR") {
            inData = QByteArray("line1\r\nline2\r\nline3\n");
            bool allLinesCrLf = doc.readFrom(&inFile);
            CHECK(docToVec(doc) == QVector<QString>{ "line1\r", "line2\r", "line3" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == false);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("mixed line endings - missing last linebreak") {
            inData = QByteArray("line1\r\nline2\nline3");
            bool allLinesCrLf = doc.readFrom(&inFile);
            CHECK(docToVec(doc) == QVector<QString>{ "line1\r", "line2", "line3" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == true);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("crlf line endings") {
            inData = QByteArray("line1\r\nline2\r\n");
            bool allLinesCrLf = doc.readFrom(&inFile);
            CHECK(docToVec(doc) == QVector<QString>{ "line1", "line2" });
            CHECK(allLinesCrLf == true);
            CHECK(doc.noNewLine() == false);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("crlf line endings - missing last linebreak") {
            inData = QByteArray("line1\r\nline2");
            bool allLinesCrLf = doc.readFrom(&inFile);
            CHECK(docToVec(doc) == QVector<QString>{ "line1", "line2" });
            CHECK(allLinesCrLf == true);
            CHECK(doc.noNewLine() == true);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("with NUL bytes") {
            inData = QByteArray("li\0e1\nline2\n", 12);
            bool allLinesCrLf = doc.readFrom(&inFile);
            std::array<char16_t, 5> expectedLine1 = {'l', 'i', 0x00, 'e', '1'};
            CHECK(docToVec(doc) == QVector<QString>{ QString::fromUtf16(expectedLine1.data(), expectedLine1.size()), "line2" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == false);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("not valid utf8") {
            // \x80\x89\xa0\xff is not valid utf8, so expect surrogate escape encoding.
            // 'e' as '\145' to work around c++ string literal parsing
            inData = QByteArray("li\x80\x89\xa0\xff\1451\nline2\n", 15);
            bool allLinesCrLf = doc.readFrom(&inFile);
            std::array<char16_t, 8> expectedLine1 = {'l', 'i', 0xdc80, 0xdc89, 0xdca0, 0xdcff, 'e', '1'};
            CHECK(docToVec(doc) == QVector<QString>{ QString::fromUtf16(expectedLine1.data(), expectedLine1.size()), "line2" });
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("with very long binary line") {
            // internal buffer is 16384 bytes

            // unseeded is ok
            std::mt19937 gen;
            std::independent_bits_engine<std::mt19937, 32, unsigned int> eng(gen);
            QByteArray random;
            random.fill('X', 16384 * 2 + 256);
            char *buf = random.data();
            for (int i = 0; i < random.size(); i += 4) {
                unsigned int item = eng();
                memcpy(&item, buf + i, 4);
            }

            inData = random + "\nsecond line\n";
            bool allLinesCrLf = doc.readFrom(&inFile);
            CHECK(doc.lineCount() == 2);
            CHECK(allLinesCrLf == false);
            CHECK(doc.noNewLine() == false);
            CHECK(doc.isModified() == false);
            CHECK(doc.isUndoAvailable() == false);
            CHECK(doc.isRedoAvailable() == false);

            doc.writeTo(&outFile, allLinesCrLf);
            CHECK(outData == inData);
        }

        SECTION("signals") {
            inData = QByteArray("line1\nline2\n");

            EventRecorder recorderModificationChanged;
            auto modificationChangedSignal = recorderModificationChanged.watchSignal(&doc, RECORDER_SIGNAL(&Document::modificationChanged));

            EventRecorder recorderRedoAvailable;
            auto redoAvailableSignal = recorderRedoAvailable.watchSignal(&doc, RECORDER_SIGNAL(&Document::redoAvailable));

            EventRecorder recorderUndoAvailable;
            auto undoAvailableSignal = recorderUndoAvailable.watchSignal(&doc, RECORDER_SIGNAL(&Document::undoAvailable));

            EventRecorder recorderContentsChanged;
            auto contentsChangedSignal = recorderContentsChanged.watchSignal(&doc, RECORDER_SIGNAL(&Document::contentsChanged));

            doc.readFrom(&inFile);

            recorderModificationChanged.waitForEvent(modificationChangedSignal);
            CHECK(recorderModificationChanged.consumeFirst(modificationChangedSignal, false));
            CHECK(recorderModificationChanged.noMoreEvents());

            recorderRedoAvailable.waitForEvent(redoAvailableSignal);
            CHECK(recorderRedoAvailable.consumeFirst(redoAvailableSignal, false));
            CHECK(recorderRedoAvailable.noMoreEvents());

            recorderUndoAvailable.waitForEvent(undoAvailableSignal);
            CHECK(recorderUndoAvailable.consumeFirst(undoAvailableSignal, false));
            CHECK(recorderUndoAvailable.noMoreEvents());

            recorderContentsChanged.waitForEvent(contentsChangedSignal);
            CHECK(recorderContentsChanged.consumeFirst(contentsChangedSignal));
            CHECK(recorderContentsChanged.noMoreEvents());
        }

        SECTION("cursors and line marker change signals") {
            cursor.insertText("\ntest test");
            REQUIRE(doc.lineCount() == 2);
            CHECK(cursor.position() == TextCursor::Position{9, 1});
            LineMarker marker{&doc, 1};

            inData = QByteArray("line1\nline2\n");

            EventRecorder recorderLineMarkerChanged;
            auto lineMarkerChangedSignal = recorderLineMarkerChanged.watchSignal(&doc, RECORDER_SIGNAL(&Document::lineMarkerChanged));

            EventRecorder recorderCursorChanged;
            auto cursorChangedSignal = recorderCursorChanged.watchSignal(&doc, RECORDER_SIGNAL(&Document::cursorChanged));

            recorderCursorChanged.waitForEvent(cursorChangedSignal);
            CHECK(recorderCursorChanged.consumeFirst(cursorChangedSignal, (const TextCursor*)&cursor));
            CHECK(recorderCursorChanged.consumeFirst(cursorChangedSignal, (const TextCursor*)&wrappedCursor));
            CHECK(recorderCursorChanged.noMoreEvents());

            doc.readFrom(&inFile);

            recorderCursorChanged.waitForEvent(cursorChangedSignal);
            CHECK(recorderCursorChanged.consumeFirst(cursorChangedSignal, (const TextCursor*)&cursor));
            CHECK(recorderCursorChanged.consumeFirst(cursorChangedSignal, (const TextCursor*)&wrappedCursor));
            CHECK(recorderCursorChanged.noMoreEvents());

            CHECK(cursor.position() == TextCursor::Position{0, 0});

            recorderLineMarkerChanged.waitForEvent(lineMarkerChangedSignal);
            CHECK(recorderLineMarkerChanged.consumeFirst(lineMarkerChangedSignal, (const LineMarker*)&marker));
            CHECK(recorderLineMarkerChanged.noMoreEvents());

            CHECK(marker.line() == 0);
        }
    }

    SECTION("markUndoStateAsSaved") {
        CHECK(doc.isModified() == false);
        CHECK(doc.isUndoAvailable() == false);
        CHECK(doc.isRedoAvailable() == false);

        cursor.insertText("abc");

        CHECK(doc.isModified() == true);
        CHECK(doc.isUndoAvailable() == true);
        CHECK(doc.isRedoAvailable() == false);

        doc.markUndoStateAsSaved();

        CHECK(doc.isModified() == false);
        CHECK(doc.isUndoAvailable() == true);
        CHECK(doc.isRedoAvailable() == false);

        cursor.insertText("def");

        CHECK(doc.isModified() == true);
        CHECK(doc.isUndoAvailable() == true);
        CHECK(doc.isRedoAvailable() == false);

        doc.undo(&cursor);

        // normally insertText "abc" followed by "def" would collapse and we would have no undo available here
        // with a markUndoStateAsSaved inbetween the intermediate undo step is preserved.
        CHECK(doc.isModified() == false);
        CHECK(doc.isUndoAvailable() == true);
        CHECK(doc.isRedoAvailable() == true);

        doc.undo(&cursor);
        // undo to a state before last markUndoStateAsSaved also reads as modified
        CHECK(doc.isModified() == true);
        CHECK(doc.isUndoAvailable() == false);
        CHECK(doc.isRedoAvailable() == true);

        doc.redo(&cursor);
        // now back at the marked as saved state
        CHECK(doc.isModified() == false);
        CHECK(doc.isUndoAvailable() == true);
        CHECK(doc.isRedoAvailable() == true);

        doc.redo(&cursor);
        CHECK(doc.isModified() == true);
        CHECK(doc.isUndoAvailable() == true);
        CHECK(doc.isRedoAvailable() == false);
    }

    SECTION("markUndoStateAsSaved signals") {
        cursor.insertText("abc");

        EventRecorder recorder;
        auto modificationChangedSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::modificationChanged));

        doc.markUndoStateAsSaved();

        recorder.waitForEvent(modificationChangedSignal);
        CHECK(recorder.consumeFirst(modificationChangedSignal, false));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("filename") {
        CHECK(doc.filename() == QString(""));
        doc.setFilename("some.txt");
        CHECK(doc.filename() == QString("some.txt"));
        doc.setFilename("other.txt");
        CHECK(doc.filename() == QString("other.txt"));
    }

    SECTION("TextCursor-Position-without-text") {

        CHECK(cursor.position() == TextCursor::Position{0, 0});

        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        //check the original after delete any text
        bool afterDeletedText = GENERATE(false, true);
        CAPTURE(afterDeletedText);
        if (afterDeletedText) {
            cursor.insertText("test test\ntest test\ntest test\n");
            REQUIRE(doc.lineCount() == 4);
            cursor.selectAll();
            cursor.removeSelectedText();
            REQUIRE(doc.lineCount() == 1);
        }

        cursor.moveCharacterLeft(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        cursor.moveWordLeft(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        cursor.moveToStartOfLine(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});

        cursor.moveCharacterRight(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        cursor.moveWordRight(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        cursor.moveToEndOfLine(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});

        cursor.moveToStartOfDocument(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        cursor.moveToStartOfDocument(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});

        cursor.moveUp();
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        cursor.moveDown();
        CHECK(cursor.position() == TextCursor::Position{0, 0});

        cursor.setPosition({3, 2}, selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
    }

    SECTION("TextCursor-Position-with-text") {

        CHECK(cursor.position() == TextCursor::Position{0, 0});

        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest test");
        REQUIRE(doc.lineCount() == 3);
        CHECK(cursor.position() == TextCursor::Position{9, 2});


        cursor.moveCharacterLeft(selection);
        CHECK(cursor.position() == TextCursor::Position{8, 2});
        cursor.moveWordLeft(selection);
        CHECK(cursor.position() == TextCursor::Position{5, 2});
        cursor.moveToStartOfLine(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 2});

        cursor.moveCharacterRight(selection);
        CHECK(cursor.position() == TextCursor::Position{1, 2});
        cursor.moveWordRight(selection);
        CHECK(cursor.position() == TextCursor::Position{4, 2});
        cursor.moveToEndOfLine(selection);
        CHECK(cursor.position() == TextCursor::Position{9, 2});

        cursor.moveToStartOfDocument(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        cursor.moveToEndOfDocument(selection);
        CHECK(cursor.position() == TextCursor::Position{9, 2});

        cursor.moveUp();
        CHECK(cursor.position() == TextCursor::Position{9, 1});
        cursor.moveDown();
        CHECK(cursor.position() == TextCursor::Position{9, 2});

        cursor.setPosition({3, 2}, selection);
        CHECK(cursor.position() == TextCursor::Position{3, 2});
    }

    SECTION("moveCharacterLeft") {
        cursor.insertText("test\ntest test\ntest test");

        SECTION("start of non first line") {
            cursor.setPosition({0, 1});
            cursor.moveCharacterLeft();
            CHECK(cursor.position() == TextCursor::Position{4, 0});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of first line, no movement") {
            cursor.setPosition({0, 0});
            cursor.moveCharacterLeft();
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of first line with selection, no movement, remove selection") {
            cursor.setPosition({1, 0});
            cursor.setPosition({0, 0}, true);
            cursor.moveCharacterLeft();
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of first line with selection, no movement, keep selection") {
            cursor.setPosition({1, 0});
            cursor.setPosition({0, 0}, true);
            cursor.moveCharacterLeft(true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 0});
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
        }

        SECTION("not at the start of a line") {
            cursor.setPosition({2, 2});
            cursor.moveCharacterLeft();
            CHECK(cursor.position() == TextCursor::Position{1, 2});
            CHECK(cursor.verticalMovementColumn() == 1);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line with selection, remove selection") {
            cursor.setPosition({3, 2});
            cursor.setPosition({2, 2}, true);
            cursor.moveCharacterLeft();
            CHECK(cursor.position() == TextCursor::Position{1, 2});
            CHECK(cursor.verticalMovementColumn() == 1);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line, create and extend selection") {
            cursor.setPosition({4, 2});
            cursor.moveCharacterLeft(true);
            CHECK(cursor.anchor() == TextCursor::Position{4, 2});
            CHECK(cursor.position() == TextCursor::Position{3, 2});
            CHECK(cursor.verticalMovementColumn() == 3);
            cursor.moveCharacterLeft(true);
            CHECK(cursor.anchor() == TextCursor::Position{4, 2});
            CHECK(cursor.position() == TextCursor::Position{2, 2});
            CHECK(cursor.verticalMovementColumn() == 2);
        }

        SECTION("astral and wide") {
            cursor.insertText("\nabc😁あdef");

            cursor.setPosition({7, 3});
            cursor.moveCharacterLeft();
            CHECK(cursor.position() == TextCursor::Position{6, 3});
            CHECK(cursor.verticalMovementColumn() == 7);
            CHECK(cursor.hasSelection() == false);
            cursor.moveCharacterLeft();
            CHECK(cursor.position() == TextCursor::Position{5, 3});
            CHECK(cursor.verticalMovementColumn() == 5);
            CHECK(cursor.hasSelection() == false);
            cursor.moveCharacterLeft();
            CHECK(cursor.position() == TextCursor::Position{3, 3});
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.hasSelection() == false);
            cursor.moveCharacterLeft();
            CHECK(cursor.position() == TextCursor::Position{2, 3});
            CHECK(cursor.verticalMovementColumn() == 2);
            CHECK(cursor.hasSelection() == false);
        }
    }

    SECTION("moveCharacterLeft with wrap") {
        cursor.insertText("Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna");
        // 000|Lorem ipsum dolor sit amet, consectetur
        // 040|adipisici elit, sed eiusmod tempor
        // 075|incidunt ut labore et dolore magna

        SECTION("Move over soft line break") {
            wrappedCursor.setPosition({76, 0});
            wrappedCursor.moveCharacterLeft();
            CHECK(wrappedCursor.position() == TextCursor::Position{75, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            wrappedCursor.moveCharacterLeft();
            CHECK(wrappedCursor.position() == TextCursor::Position{74, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            // the cursor is placed before the space, so vertical movement column does not include the space.
            CHECK(wrappedCursor.verticalMovementColumn() == 34);
        }
    }

    SECTION("moveWordLeft") {
        cursor.insertText("test\ntest test\ntest test");

        SECTION("start of non first line") {
            cursor.setPosition({0, 1});
            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{4, 0});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of first line, no movement") {
            cursor.setPosition({0, 0});
            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of first line with selection, no movement, remove selection") {
            cursor.setPosition({1, 0});
            cursor.setPosition({0, 0}, true);
            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of first line with selection, no movement, keep selection") {
            cursor.setPosition({1, 0});
            cursor.setPosition({0, 0}, true);
            cursor.moveWordLeft(true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 0});
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
        }

        SECTION("not at the start of a line, in first word") {
            cursor.setPosition({2, 1});
            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{0, 1});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line, in second word") {
            cursor.setPosition({9, 1});
            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{5, 1});
            CHECK(cursor.verticalMovementColumn() == 5);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line with selection, in second word, remove selection") {
            cursor.setPosition({9, 1});
            cursor.setPosition({8, 1});
            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{5, 1});
            CHECK(cursor.verticalMovementColumn() == 5);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line, create and extend selection") {
            cursor.setPosition({9, 2});
            cursor.moveWordLeft(true);
            CHECK(cursor.anchor() == TextCursor::Position{9, 2});
            CHECK(cursor.position() == TextCursor::Position{5, 2});
            CHECK(cursor.verticalMovementColumn() == 5);
            cursor.moveWordLeft(true);
            CHECK(cursor.anchor() == TextCursor::Position{9, 2});
            CHECK(cursor.position() == TextCursor::Position{0, 2});
            CHECK(cursor.verticalMovementColumn() == 0);
        }

        SECTION("astral and wide") {
            cursor.insertText("\nabc 😁 あ def");

            cursor.setPosition({11, 3});
            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{9, 3});
            CHECK(cursor.verticalMovementColumn() == 10);
            CHECK(cursor.hasSelection() == false);

            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{7, 3});
            CHECK(cursor.verticalMovementColumn() == 7);
            CHECK(cursor.hasSelection() == false);

            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{4, 3});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);

            cursor.moveWordLeft();
            CHECK(cursor.position() == TextCursor::Position{0, 3});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }
    }

    SECTION("moveWordLeft with wrap") {
        cursor.insertText("Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna");
        // 000|Lorem ipsum dolor sit amet, consectetur
        // 040|adipisici elit, sed eiusmod tempor
        // 075|incidunt ut labore et dolore magna

        SECTION("Move over soft line break") {
            wrappedCursor.setPosition({80, 0});
            wrappedCursor.moveWordLeft();
            CHECK(wrappedCursor.position() == TextCursor::Position{75, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            wrappedCursor.moveWordLeft();
            CHECK(wrappedCursor.position() == TextCursor::Position{68, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 28);
        }
    }

    SECTION("moveCharacterRight") {
        cursor.insertText("test test\ntest test\ntest");

        SECTION("end of non last line") {
            cursor.setPosition({9, 1});
            cursor.moveCharacterRight();
            CHECK(cursor.position() == TextCursor::Position{0, 2});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("end of last line, no movement") {
            cursor.setPosition({4, 2});
            cursor.moveCharacterRight();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("end of last line with selection, no movement, remove selection") {
            cursor.setPosition({3, 2});
            cursor.setPosition({4, 2}, true);
            cursor.moveCharacterRight();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("end of last line with selection, no movement, keep selection") {
            cursor.setPosition({3, 2});
            cursor.setPosition({4, 2}, true);
            cursor.moveCharacterRight(true);
            CHECK(cursor.anchor() == TextCursor::Position{3, 2});
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
        }

        SECTION("not at the end of a line") {
            cursor.setPosition({2, 2});
            cursor.moveCharacterRight();
            CHECK(cursor.position() == TextCursor::Position{3, 2});
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line with selection, remove selection") {
            cursor.setPosition({1, 2});
            cursor.setPosition({2, 2}, true);
            cursor.moveCharacterRight();
            CHECK(cursor.position() == TextCursor::Position{3, 2});
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line, create and extend selection") {
            cursor.setPosition({1, 2});
            cursor.moveCharacterRight(true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 2});
            CHECK(cursor.position() == TextCursor::Position{2, 2});
            CHECK(cursor.verticalMovementColumn() == 2);
            cursor.moveCharacterRight(true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 2});
            CHECK(cursor.position() == TextCursor::Position{3, 2});
            CHECK(cursor.verticalMovementColumn() == 3);
        }

        SECTION("astral and wide") {
            cursor.insertText("\nabc😁あdef");

            cursor.setPosition({2, 3});
            cursor.moveCharacterRight();
            CHECK(cursor.position() == TextCursor::Position{3, 3});
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.hasSelection() == false);
            cursor.moveCharacterRight();
            CHECK(cursor.position() == TextCursor::Position{5, 3});
            CHECK(cursor.verticalMovementColumn() == 5);
            CHECK(cursor.hasSelection() == false);
            cursor.moveCharacterRight();
            CHECK(cursor.position() == TextCursor::Position{6, 3});
            CHECK(cursor.verticalMovementColumn() == 7);
            CHECK(cursor.hasSelection() == false);
            cursor.moveCharacterRight();
            CHECK(cursor.position() == TextCursor::Position{7, 3});
            CHECK(cursor.verticalMovementColumn() == 8);
            CHECK(cursor.hasSelection() == false);
        }
    }

    SECTION("moveCharacterRight with wrap") {
        cursor.insertText("Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna");
        // 000|Lorem ipsum dolor sit amet, consectetur
        // 040|adipisici elit, sed eiusmod tempor
        // 075|incidunt ut labore et dolore magna

        SECTION("Move over soft line break") {
            wrappedCursor.setPosition({73, 0});
            wrappedCursor.moveCharacterRight();
            CHECK(wrappedCursor.position() == TextCursor::Position{74, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 34);
            wrappedCursor.moveCharacterRight();
            CHECK(wrappedCursor.position() == TextCursor::Position{75, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
        }
    }

    SECTION("moveWordRight") {
        cursor.insertText("test test\ntest test\ntest");

        SECTION("end of non last line") {
            cursor.setPosition({9, 1});
            cursor.moveWordRight();
            CHECK(cursor.position() == TextCursor::Position{0, 2});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("end of last line, no movement") {
            cursor.setPosition({4, 2});
            cursor.moveWordRight();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("end of last line with selection, no movement, remove selection") {
            cursor.setPosition({3, 2});
            cursor.setPosition({4, 2}, true);
            cursor.moveWordRight();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line") {
            cursor.setPosition({2, 1});
            cursor.moveWordRight();
            CHECK(cursor.position() == TextCursor::Position{4, 1});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line with selection, remove selection") {
            cursor.setPosition({1, 1});
            cursor.setPosition({2, 2}, true);
            cursor.moveWordRight();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line, create and extend selection") {
            cursor.setPosition({1, 1});
            cursor.moveWordRight(true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 1});
            CHECK(cursor.position() == TextCursor::Position{4, 1});
            CHECK(cursor.verticalMovementColumn() == 4);
            cursor.moveWordRight(true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 1});
            CHECK(cursor.position() == TextCursor::Position{9, 1});
            CHECK(cursor.verticalMovementColumn() == 9);
        }

        SECTION("astral and wide") {
            cursor.insertText("\nabc 😁 あ def");

            cursor.setPosition({0, 3});
            cursor.moveWordRight();
            CHECK(cursor.position() == TextCursor::Position{3, 3});
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.hasSelection() == false);

            cursor.moveWordRight();
            CHECK(cursor.position() == TextCursor::Position{6, 3});
            CHECK(cursor.verticalMovementColumn() == 6);
            CHECK(cursor.hasSelection() == false);

            cursor.moveWordRight();
            CHECK(cursor.position() == TextCursor::Position{8, 3});
            CHECK(cursor.verticalMovementColumn() == 9);
            CHECK(cursor.hasSelection() == false);

            cursor.moveWordRight();
            CHECK(cursor.position() == TextCursor::Position{12, 3});
            CHECK(cursor.verticalMovementColumn() == 13);
            CHECK(cursor.hasSelection() == false);
        }
    }

    SECTION("moveWordRight with wrap") {
        cursor.insertText("Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna");
        // 000|Lorem ipsum dolor sit amet, consectetur
        // 040|adipisici elit, sed eiusmod tempor
        // 075|incidunt ut labore et dolore magna


        SECTION("Move over soft line break") {
            wrappedCursor.setPosition({70, 0});
            wrappedCursor.moveWordRight();
            CHECK(wrappedCursor.position() == TextCursor::Position{74, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 34);
            wrappedCursor.moveWordRight();
            CHECK(wrappedCursor.position() == TextCursor::Position{83, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 8);
        }
    }


    SECTION("moveToStartOfLine") {
        cursor.insertText("test\ntest test\ntest test");

        SECTION("start of line, no movement") {
            cursor.setPosition({0, 1});
            cursor.moveToStartOfLine();
            CHECK(cursor.position() == TextCursor::Position{0, 1});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of line with selection, no movement, remove selection") {
            cursor.setPosition({1, 1});
            cursor.setPosition({0, 1}, true);
            cursor.moveToStartOfLine();
            CHECK(cursor.position() == TextCursor::Position{0, 1});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line") {
            cursor.setPosition({2, 2});
            cursor.moveToStartOfLine();
            CHECK(cursor.position() == TextCursor::Position{0, 2});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line with selection, remove selection") {
            cursor.setPosition({3, 2});
            cursor.setPosition({2, 2}, true);
            cursor.moveToStartOfLine();
            CHECK(cursor.position() == TextCursor::Position{0, 2});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line, create and keep selection") {
            cursor.setPosition({4, 2});
            cursor.moveToStartOfLine(true);
            CHECK(cursor.anchor() == TextCursor::Position{4, 2});
            CHECK(cursor.position() == TextCursor::Position{0, 2});
            CHECK(cursor.verticalMovementColumn() == 0);
            cursor.moveToStartOfLine(true);
            CHECK(cursor.anchor() == TextCursor::Position{4, 2});
            CHECK(cursor.position() == TextCursor::Position{0, 2});
            CHECK(cursor.verticalMovementColumn() == 0);
        }
    }

    SECTION("moveToStartOfLine with wrap") {
        cursor.insertText("Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna");
        // 000|Lorem ipsum dolor sit amet, consectetur
        // 040|adipisici elit, sed eiusmod tempor
        // 075|incidunt ut labore et dolore magna

        SECTION("soft wraps are ignored") {
            wrappedCursor.setPosition({80, 0});
            wrappedCursor.moveToStartOfLine();
            CHECK(wrappedCursor.position() == TextCursor::Position{0, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
        }
    }

    SECTION("moveToStartIndentedText") {

        cursor.insertText("test\n   test test\n\t\ttest test\n  \ttest");

        SECTION("no indent") {
            SECTION("start of line, no movement") {
                cursor.setPosition({0, 0});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{0, 0});
                CHECK(cursor.verticalMovementColumn() == 0);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("start of line with selection, no movement, remove selection") {
                cursor.setPosition({1, 0});
                cursor.setPosition({0, 0}, true);
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{0, 0});
                CHECK(cursor.verticalMovementColumn() == 0);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("not at the start of a line") {
                cursor.setPosition({2, 0});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{0, 0});
                CHECK(cursor.verticalMovementColumn() == 0);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("not at the start of a line with selection, remove selection") {
                cursor.setPosition({3, 0});
                cursor.setPosition({2, 0}, true);
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{0, 0});
                CHECK(cursor.verticalMovementColumn() == 0);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("not at the start of a line, create and keep selection") {
                cursor.setPosition({4, 0});
                cursor.moveToStartIndentedText(true);
                CHECK(cursor.anchor() == TextCursor::Position{4, 0});
                CHECK(cursor.position() == TextCursor::Position{0, 0});
                CHECK(cursor.verticalMovementColumn() == 0);
                cursor.moveToStartIndentedText(true);
                CHECK(cursor.anchor() == TextCursor::Position{4, 0});
                CHECK(cursor.position() == TextCursor::Position{0, 0});
                CHECK(cursor.verticalMovementColumn() == 0);
            }
        }

        SECTION("space indent") {
            SECTION("start of line") {
                cursor.setPosition({0, 1});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 1});
                CHECK(cursor.verticalMovementColumn() == 3);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("start of indent, no movement") {
                cursor.setPosition({3, 1});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 1});
                CHECK(cursor.verticalMovementColumn() == 3);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("start of indent with selection, no movement, remove selection") {
                cursor.setPosition({4, 1});
                cursor.setPosition({3, 1}, true);
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 1});
                CHECK(cursor.verticalMovementColumn() == 3);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indent") {
                cursor.setPosition({1, 1});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 1});
                CHECK(cursor.verticalMovementColumn() == 3);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indented text") {
                cursor.setPosition({5, 1});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 1});
                CHECK(cursor.verticalMovementColumn() == 3);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indented text with selection, remove selection") {
                cursor.setPosition({6, 1});
                cursor.setPosition({5, 1}, true);
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 1});
                CHECK(cursor.verticalMovementColumn() == 3);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indented text, create and keep selection") {
                cursor.setPosition({5, 1});
                cursor.moveToStartIndentedText(true);
                CHECK(cursor.anchor() == TextCursor::Position{5, 1});
                CHECK(cursor.position() == TextCursor::Position{3, 1});
                CHECK(cursor.verticalMovementColumn() == 3);
                cursor.moveToStartIndentedText(true);
                CHECK(cursor.anchor() == TextCursor::Position{5, 1});
                CHECK(cursor.position() == TextCursor::Position{3, 1});
                CHECK(cursor.verticalMovementColumn() == 3);
            }
        }

        SECTION("tab indent") {
            SECTION("start of line") {
                cursor.setPosition({0, 2});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{2, 2});
                CHECK(cursor.verticalMovementColumn() == 16);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("start of indent, no movement") {
                cursor.setPosition({2, 2});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{2, 2});
                CHECK(cursor.verticalMovementColumn() == 16);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("start of indent with selection, no movement, remove selection") {
                cursor.setPosition({3, 2});
                cursor.setPosition({2, 2}, true);
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{2, 2});
                CHECK(cursor.verticalMovementColumn() == 16);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indent") {
                cursor.setPosition({1, 2});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{2, 2});
                CHECK(cursor.verticalMovementColumn() == 16);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indented text") {
                cursor.setPosition({5, 2});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{2, 2});
                CHECK(cursor.verticalMovementColumn() == 16);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indented text with selection, remove selection") {
                cursor.setPosition({6, 2});
                cursor.setPosition({5, 2}, true);
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{2, 2});
                CHECK(cursor.verticalMovementColumn() == 16);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indented text, create and keep selection") {
                cursor.setPosition({5, 2});
                cursor.moveToStartIndentedText(true);
                CHECK(cursor.anchor() == TextCursor::Position{5, 2});
                CHECK(cursor.position() == TextCursor::Position{2, 2});
                CHECK(cursor.verticalMovementColumn() == 16);
                cursor.moveToStartIndentedText(true);
                CHECK(cursor.anchor() == TextCursor::Position{5, 2});
                CHECK(cursor.position() == TextCursor::Position{2, 2});
                CHECK(cursor.verticalMovementColumn() == 16);
            }

        }

        SECTION("mixed indent") {
            SECTION("start of line") {
                cursor.setPosition({0, 3});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 3});
                CHECK(cursor.verticalMovementColumn() == 8);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("start of indent, no movement") {
                cursor.setPosition({3, 3});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 3});
                CHECK(cursor.verticalMovementColumn() == 8);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("start of indent with selection, no movement, remove selection") {
                cursor.setPosition({4, 3});
                cursor.setPosition({3, 3}, true);
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 3});
                CHECK(cursor.verticalMovementColumn() == 8);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indent") {
                cursor.setPosition({1, 3});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 3});
                CHECK(cursor.verticalMovementColumn() == 8);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indented text") {
                cursor.setPosition({5, 3});
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 3});
                CHECK(cursor.verticalMovementColumn() == 8);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indented text with selection, remove selection") {
                cursor.setPosition({6, 3});
                cursor.setPosition({5, 3}, true);
                cursor.moveToStartIndentedText();
                CHECK(cursor.position() == TextCursor::Position{3, 3});
                CHECK(cursor.verticalMovementColumn() == 8);
                CHECK(cursor.hasSelection() == false);
            }

            SECTION("in indented text, create and keep selection") {
                cursor.setPosition({5, 3});
                cursor.moveToStartIndentedText(true);
                CHECK(cursor.anchor() == TextCursor::Position{5, 3});
                CHECK(cursor.position() == TextCursor::Position{3, 3});
                CHECK(cursor.verticalMovementColumn() == 8);
                cursor.moveToStartIndentedText(true);
                CHECK(cursor.anchor() == TextCursor::Position{5, 3});
                CHECK(cursor.position() == TextCursor::Position{3, 3});
                CHECK(cursor.verticalMovementColumn() == 8);
            }
        }
    }

    SECTION("moveToStartIndentedText with wrap") {
        SECTION("soft wraps are ignored") {
            cursor.insertText("Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna");
            // 000|Lorem ipsum dolor sit amet, consectetur
            // 040|adipisici elit, sed eiusmod tempor
            // 075|incidunt ut labore et dolore magna

            wrappedCursor.setPosition({80, 0});
            wrappedCursor.moveToStartIndentedText();
            CHECK(wrappedCursor.position() == TextCursor::Position{0, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
        }

        SECTION("very long indent") {
            cursor.insertText("                                           pisici elit, sed eiusmod tempor incidunt ut labore et dolore magna");
            // line 0 is lots of spaces
            // 040|   pisici elit, sed eiusmod tempor
            // 075|incidunt ut labore et dolore magna

            wrappedCursor.setPosition({80, 0});
            wrappedCursor.moveToStartIndentedText();
            CHECK(wrappedCursor.position() == TextCursor::Position{43, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 3);
        }
    }

    SECTION("moveToEndOfLine") {
        cursor.insertText("test\ntest test\ntest test");

        SECTION("end of line, no movement") {
            cursor.setPosition({9, 1});
            cursor.moveToEndOfLine();
            CHECK(cursor.position() == TextCursor::Position{9, 1});
            CHECK(cursor.verticalMovementColumn() == 9);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("end of line with selection, no movement, remove selection") {
            cursor.setPosition({8, 1});
            cursor.setPosition({9, 1}, true);
            cursor.moveToEndOfLine();
            CHECK(cursor.position() == TextCursor::Position{9, 1});
            CHECK(cursor.verticalMovementColumn() == 9);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line") {
            cursor.setPosition({2, 2});
            cursor.moveToEndOfLine();
            CHECK(cursor.position() == TextCursor::Position{9, 2});
            CHECK(cursor.verticalMovementColumn() == 9);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line with selection, remove selection") {
            cursor.setPosition({3, 2});
            cursor.setPosition({2, 2}, true);
            cursor.moveToEndOfLine();
            CHECK(cursor.position() == TextCursor::Position{9, 2});
            CHECK(cursor.verticalMovementColumn() == 9);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line, create and keep selection") {
            cursor.setPosition({4, 2});
            cursor.moveToEndOfLine(true);
            CHECK(cursor.anchor() == TextCursor::Position{4, 2});
            CHECK(cursor.position() == TextCursor::Position{9, 2});
            CHECK(cursor.verticalMovementColumn() == 9);
            cursor.moveToEndOfLine(true);
            CHECK(cursor.anchor() == TextCursor::Position{4, 2});
            CHECK(cursor.position() == TextCursor::Position{9, 2});
            CHECK(cursor.verticalMovementColumn() == 9);
        }
    }

    SECTION("moveToEndOfLine with wrap") {
        cursor.insertText("Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna");
        // 000|Lorem ipsum dolor sit amet, consectetur
        // 040|adipisici elit, sed eiusmod tempor
        // 075|incidunt ut labore et dolore magna

        SECTION("soft wraps are ignored") {
            wrappedCursor.setPosition({20, 0});
            wrappedCursor.moveToEndOfLine();
            CHECK(wrappedCursor.position() == TextCursor::Position{109, 0});
            CHECK(wrappedCursor.hasSelection() == false);
            CHECK(wrappedCursor.verticalMovementColumn() == 34);
        }
    }

    SECTION("moveUp") {
        cursor.insertText("short\nlong line\nlongest line\n\nsome line");

        SECTION("first line") {
            cursor.setPosition({3, 0});
            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{3, 0});
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("first line with selection") {
            cursor.setPosition({2, 0});
            cursor.setPosition({3, 0}, true);
            cursor.moveUp();
            CHECK(cursor.anchor() == TextCursor::Position{2, 0});
            CHECK(cursor.position() == TextCursor::Position{3, 0});
            CHECK(cursor.verticalMovementColumn() == 3);
        }

        SECTION("vertical movement 7") {
            cursor.setPosition({7, 1});
            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{5, 0});
            CHECK(cursor.verticalMovementColumn() == 7);

            cursor.setPosition({7, 4});
            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{0, 3});
            CHECK(cursor.verticalMovementColumn() == 7);

            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{7, 2});
            CHECK(cursor.verticalMovementColumn() == 7);

            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{7, 1});
            CHECK(cursor.verticalMovementColumn() == 7);

            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{5, 0});
            CHECK(cursor.verticalMovementColumn() == 7);
        }

        SECTION("vertical movement 12") {
            cursor.setPosition({12, 2});
            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{9, 1});
            CHECK(cursor.verticalMovementColumn() == 12);

            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{5, 0});
            CHECK(cursor.verticalMovementColumn() == 12);
        }

        SECTION("vertical movement 100") {
            cursor.setPosition({7, 4});
            cursor.setVerticalMovementColumn(100);
            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{0, 3});
            CHECK(cursor.verticalMovementColumn() == 100);

            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{12, 2});
            CHECK(cursor.verticalMovementColumn() == 100);

            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{9, 1});
            CHECK(cursor.verticalMovementColumn() == 100);

            cursor.moveUp();
            CHECK(cursor.position() == TextCursor::Position{5, 0});
            CHECK(cursor.verticalMovementColumn() == 100);
        }
    }


    SECTION("moveUp with wrap") {
        cursor.insertText("Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat. Quis aute iure reprehenderit in voluptate velit esse cillum dolore eu");
        // 000|Lorem ipsum dolor sit amet, consectetur
        // 040|adipisici elit, sed eiusmod tempor
        // 075|incidunt ut labore et dolore magna
        // 110|aliqua. Ut enim ad minim veniam, quis
        // 148|nostrud exercitation ullamco laboris
        // 185|nisi ut aliquid ex ea commodi consequat.
        // 225| Quis aute iure reprehenderit in
        // 258|voluptate velit esse cillum dolore eu

        SECTION("intra line with wrapping - vert 0") {
            wrappedCursor.setPosition({258, 0});
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{225, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{185, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{148, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{110, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{75, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{40, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{0, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
        }

        SECTION("intra line with wrapping - vert 39") {
            wrappedCursor.setPosition({224, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{184, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{147, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{109, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{74, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{39, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{39, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
        }
    }


    SECTION("moveUp with wrap - wide and astral") {
        cursor.insertText("L😁rem ipsum d😁l😁r sit あmet, c😁nsectetur あdipisici elit, sed eiusm😁d temp😁r incidunt ut lあb😁re et d😁l😁re mあgnあ あliquあ. Ut enim あd minim veniあm, quis n😁strud exercitあti😁n ullあmc😁 lあb😁ris nisi ut あliquid ex eあ c😁mm😁di c😁nsequあt. Quis あute iure reprehenderit in v😁luptあte velit");
        //    |break ->                                v
        // 000|L😁rem ipsum d😁l😁r sit あmet, c😁
        // 034|nsectetur あdipisici elit, sed eiusm😁d
        // 073|temp😁r incidunt ut lあb😁re et d😁l😁re
        // 112| mあgnあ あliquあ. Ut enim あd minim
        // 144|veniあm, quis n😁strud exercitあti😁n
        // 180|ullあmc😁 lあb😁ris nisi ut あliquid ex
        // 217|eあ c😁mm😁di c😁nsequあt. Quis あute
        // 252|iure reprehenderit in v😁luptあte velit

        SECTION("intra line with wrapping - vert 0") {
            wrappedCursor.setPosition({252, 0});
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{217, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{180, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{144, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{112, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{73, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{34, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{0, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
        }

        SECTION("intra line with wrapping - vert 20") {
            wrappedCursor.setPosition({252 + 20, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{217 + 19, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{180 + 18, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{144 + 19, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{112 + 16, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{73 + 20, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{34 + 19, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveUp();
            CHECK(wrappedCursor.position() == TextCursor::Position{20, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
        }
    }

    SECTION("moveDown") {
        cursor.insertText("short\nlong line\nlongest line\n\nsome line");

        SECTION("last line") {
            cursor.setPosition({3, 4});
            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{3, 4});
            CHECK(cursor.verticalMovementColumn() == 3);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("last line with selection") {
            cursor.setPosition({2, 4});
            cursor.setPosition({3, 4}, true);
            cursor.moveDown();
            CHECK(cursor.anchor() == TextCursor::Position{2, 4});
            CHECK(cursor.position() == TextCursor::Position{3, 4});
            CHECK(cursor.verticalMovementColumn() == 3);
        }

        SECTION("vertical movement 7") {
            cursor.setPosition({7, 1});
            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{7, 2});
            CHECK(cursor.verticalMovementColumn() == 7);
            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{0, 3});
            CHECK(cursor.verticalMovementColumn() == 7);
            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{7, 4});
            CHECK(cursor.verticalMovementColumn() == 7);
        }

        SECTION("vertical movement 12") {
            cursor.setPosition({12, 2});
            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{0, 3});
            CHECK(cursor.verticalMovementColumn() == 12);

            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{9, 4});
            CHECK(cursor.verticalMovementColumn() == 12);
        }

        SECTION("vertical movement 100") {
            cursor.setPosition({0, 0});
            cursor.setVerticalMovementColumn(100);

            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{9, 1});
            CHECK(cursor.verticalMovementColumn() == 100);

            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{12, 2});
            CHECK(cursor.verticalMovementColumn() == 100);

            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{0, 3});
            CHECK(cursor.verticalMovementColumn() == 100);

            cursor.moveDown();
            CHECK(cursor.position() == TextCursor::Position{9, 4});
            CHECK(cursor.verticalMovementColumn() == 100);
        }
    }

    SECTION("moveDown with wrap") {
        cursor.insertText("Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat. Quis aute iure reprehenderit in voluptate velit esse cillum dolore eu");
        // 000|Lorem ipsum dolor sit amet, consectetur
        // 040|adipisici elit, sed eiusmod tempor
        // 075|incidunt ut labore et dolore magna
        // 110|aliqua. Ut enim ad minim veniam, quis
        // 148|nostrud exercitation ullamco laboris
        // 185|nisi ut aliquid ex ea commodi consequat.
        // 225| Quis aute iure reprehenderit in
        // 258|voluptate velit esse cillum dolore eu

        SECTION("intra line with wrapping - vert 0") {
            wrappedCursor.setPosition({0, 0});
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{40, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{75, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{110, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{148, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{185, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{225, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{258, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
        }

        SECTION("intra line with wrapping - vert 39") {
            wrappedCursor.setPosition({39, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{74, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{109, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{147, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{184, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{224, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{257, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{295, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 39);
            CHECK(wrappedCursor.hasSelection() == false);
        }
    }

    SECTION("moveDown with wrap - wide and astral") {
        cursor.insertText("L😁rem ipsum d😁l😁r sit あmet, c😁nsectetur あdipisici elit, sed eiusm😁d temp😁r incidunt ut lあb😁re et d😁l😁re mあgnあ あliquあ. Ut enim あd minim veniあm, quis n😁strud exercitあti😁n ullあmc😁 lあb😁ris nisi ut あliquid ex eあ c😁mm😁di c😁nsequあt. Quis あute iure reprehenderit in v😁luptあte velit");
        //    |break ->                                v
        // 000|L😁rem ipsum d😁l😁r sit あmet, c😁
        // 034|nsectetur あdipisici elit, sed eiusm😁d
        // 073|temp😁r incidunt ut lあb😁re et d😁l😁re
        // 112| mあgnあ あliquあ. Ut enim あd minim
        // 144|veniあm, quis n😁strud exercitあti😁n
        // 180|ullあmc😁 lあb😁ris nisi ut あliquid ex
        // 217|eあ c😁mm😁di c😁nsequあt. Quis あute
        // 252|iure reprehenderit in v😁luptあte velit

        SECTION("intra line with wrapping - vert 0") {
            wrappedCursor.setPosition({0, 0});
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{34, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{73, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{112, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{144, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{180, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{217, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{252, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 0);
            CHECK(wrappedCursor.hasSelection() == false);
        }
        SECTION("intra line with wrapping - vert 20") {
            wrappedCursor.setPosition({20, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{34 + 19, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{73 + 20, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{112 + 16, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{144 + 19, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{180 + 18, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{217 + 19, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
            wrappedCursor.moveDown();
            CHECK(wrappedCursor.position() == TextCursor::Position{252 + 20, 0});
            CHECK(wrappedCursor.verticalMovementColumn() == 20);
            CHECK(wrappedCursor.hasSelection() == false);
        }
    }

    SECTION("moveToStartOfDocument") {
        cursor.insertText("test\ntest test\ntest test");

        SECTION("start of non first line") {
            cursor.setPosition({0, 1});
            cursor.moveToStartOfDocument();
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of first line, no movement") {
            cursor.setPosition({0, 0});
            cursor.moveToStartOfDocument();
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of first line with selection, no movement, remove selection") {
            cursor.setPosition({1, 0});
            cursor.setPosition({0, 0}, true);
            cursor.moveToStartOfDocument();
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("start of first line with selection, no movement, keep selection") {
            cursor.setPosition({1, 0});
            cursor.setPosition({0, 0}, true);
            cursor.moveToStartOfDocument(true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 0});
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
        }

        SECTION("not at the start of a line") {
            cursor.setPosition({2, 2});
            cursor.moveToStartOfDocument();
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line with selection, remove selection") {
            cursor.setPosition({3, 2});
            cursor.setPosition({2, 2}, true);
            cursor.moveToStartOfDocument();
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the start of a line, create and keep selection") {
            cursor.setPosition({4, 2});
            cursor.moveToStartOfDocument(true);
            CHECK(cursor.anchor() == TextCursor::Position{4, 2});
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
            cursor.moveToStartOfDocument(true);
            CHECK(cursor.anchor() == TextCursor::Position{4, 2});
            CHECK(cursor.position() == TextCursor::Position{0, 0});
            CHECK(cursor.verticalMovementColumn() == 0);
        }

    }

    SECTION("moveToEndOfDocument") {
        cursor.insertText("test test\ntest test\ntest");

        SECTION("end of non last line") {
            cursor.setPosition({9, 1});
            cursor.moveToEndOfDocument();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("end of last line, no movement") {
            cursor.setPosition({4, 2});
            cursor.moveToEndOfDocument();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("end of last line with selection, no movement, remove selection") {
            cursor.setPosition({3, 2});
            cursor.setPosition({4, 2}, true);
            cursor.moveToEndOfDocument();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("end of last line with selection, no movement, keep selection") {
            cursor.setPosition({3, 2});
            cursor.setPosition({4, 2}, true);
            cursor.moveToEndOfDocument(true);
            CHECK(cursor.anchor() == TextCursor::Position{3, 2});
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
        }

        SECTION("not at the end of a line") {
            cursor.setPosition({2, 2});
            cursor.moveToEndOfDocument();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line with selection, remove selection") {
            cursor.setPosition({1, 2});
            cursor.setPosition({2, 2}, true);
            cursor.moveToEndOfDocument();
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            CHECK(cursor.hasSelection() == false);
        }

        SECTION("not at the end of a line, create and keep selection") {
            cursor.setPosition({1, 2});
            cursor.moveToEndOfDocument(true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 2});
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
            cursor.moveToEndOfDocument(true);
            CHECK(cursor.anchor() == TextCursor::Position{1, 2});
            CHECK(cursor.position() == TextCursor::Position{4, 2});
            CHECK(cursor.verticalMovementColumn() == 4);
        }

        SECTION("astral and wide") {
            cursor.insertText("\nabc😁あdef");

            cursor.setPosition({2, 2});
            cursor.moveToEndOfDocument();
            CHECK(cursor.position() == TextCursor::Position{9, 3});
            CHECK(cursor.verticalMovementColumn() == 10);
        }
    }

    SECTION("move-up") {
        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest test");
        cursor.setPosition({0, 1});
        cursor.moveUp(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 0});
    }

    SECTION("move-up-position1-1") {
        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest test");
        cursor.setPosition({1, 1});
        cursor.moveUp(selection);
        CHECK(cursor.position() == TextCursor::Position{1, 0});
    }

    SECTION("move-down") {
        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest test");
        cursor.setPosition({0, 1});
        cursor.moveDown(selection);
        CHECK(cursor.position() == TextCursor::Position{0, 2});
    }

    SECTION("move-down-notext") {
        bool selection = GENERATE(false, true);
        CAPTURE(selection);

        cursor.insertText("test test\ntest test\ntest");
        cursor.setPosition({8, 1});
        cursor.moveDown(selection);
        CHECK(cursor.position() == TextCursor::Position{4, 2});
    }

    SECTION("delete") {
        cursor.insertText("test test\ntest test\ntest test");
        REQUIRE(doc.lineCount() == 3);
        REQUIRE(doc.lineCodeUnits(0) == 9);
        REQUIRE(doc.lineCodeUnits(1) == 9);
        REQUIRE(doc.lineCodeUnits(2) == 9);
        CHECK(cursor.position() == TextCursor::Position{9, 2});

        cursor.deletePreviousCharacter();
        CHECK(docToVec(doc) == QVector<QString>{"test test", "test test", "test tes"});
        REQUIRE(doc.lineCount() == 3);
        REQUIRE(doc.lineCodeUnits(0) == 9);
        REQUIRE(doc.lineCodeUnits(1) == 9);
        REQUIRE(doc.lineCodeUnits(2) == 8);
        CHECK(cursor.position() == TextCursor::Position{8, 2});
        cursor.deletePreviousWord();
        CHECK(docToVec(doc) == QVector<QString>{"test test", "test test", "test "});
        REQUIRE(doc.lineCount() == 3);
        REQUIRE(doc.lineCodeUnits(0) == 9);
        REQUIRE(doc.lineCodeUnits(1) == 9);
        REQUIRE(doc.lineCodeUnits(2) == 5);
        CHECK(cursor.position() == TextCursor::Position{5, 2});

        cursor.setPosition({0, 1});

        cursor.deleteCharacter();
        CHECK(docToVec(doc) == QVector<QString>{"test test", "est test", "test "});
        CHECK(cursor.position() == TextCursor::Position{0, 1});
        CHECK(doc.lineCount() == 3);
        CHECK(doc.lineCodeUnits(0) == 9);
        CHECK(doc.lineCodeUnits(1) == 8);
        CHECK(doc.lineCodeUnits(2) == 5);

        cursor.deleteWord();
        CHECK(docToVec(doc) == QVector<QString>{"test test", " test", "test "});
        CHECK(cursor.position() == TextCursor::Position{0, 1});
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
            CHECK(cursor.position() == TextCursor::Position{0, i - 1});
        }
        CHECK(docToVec(doc) == QVector<QString>{""});

        cursor.insertText("\n\n\n\n\n\n");
        CHECK(cursor.position() == TextCursor::Position{0, 6});
        CHECK(doc.lineCount() == 7);

        cursor.deletePreviousCharacter();
        CHECK(docToVec(doc) == QVector<QString>{"", "", "", "", "", ""});
        CHECK(cursor.position() == TextCursor::Position{0, 5});
        CHECK(doc.lineCount() == 6);

        cursor.deletePreviousWord();
        CHECK(docToVec(doc) == QVector<QString>{"", "", "", "", ""});
        CHECK(cursor.position() == TextCursor::Position{0, 4});
        CHECK(doc.lineCount() == 5);

        cursor.setPosition({0, 2});

        cursor.deleteCharacter();
        CHECK(docToVec(doc) == QVector<QString>{"", "", "", ""});
        CHECK(cursor.position() == TextCursor::Position{0, 2});
        CHECK(doc.lineCount() == 4);

        cursor.deleteWord();
        CHECK(docToVec(doc) == QVector<QString>{"", "", ""});
        CHECK(cursor.position() == TextCursor::Position{0, 2});
        CHECK(doc.lineCount() == 3);
    }

    SECTION("delete-word") {
        cursor.insertText("a bb  ccc   dddd     eeeee");
        cursor.deletePreviousWord();
        CHECK(docToVec(doc) == QVector<QString>{"a bb  ccc   dddd     "});
        CHECK(cursor.position() == TextCursor::Position{21, 0});
        cursor.deletePreviousWord();
        CHECK(docToVec(doc) == QVector<QString>{"a bb  ccc   "});
        CHECK(cursor.position() == TextCursor::Position{12, 0});
        cursor.deletePreviousWord();
        CHECK(docToVec(doc) == QVector<QString>{"a bb  "});
        CHECK(cursor.position() == TextCursor::Position{6, 0});
        cursor.deletePreviousWord();
        CHECK(docToVec(doc) == QVector<QString>{"a "});
        CHECK(cursor.position() == TextCursor::Position{2, 0});
        cursor.deletePreviousWord();
        CHECK(docToVec(doc) == QVector<QString>{""});
        CHECK(cursor.position() == TextCursor::Position{0, 0});

        cursor.insertText("a bb  ccc   dddd     eeeee");
        cursor.setPosition({7, 0});
        cursor.deletePreviousWord();
        CHECK(docToVec(doc) == QVector<QString>{"a bb  cc   dddd     eeeee"});
        CHECK(cursor.position() == TextCursor::Position{6, 0});

        cursor.selectAll();
        cursor.insertText("a bb  ccc   dddd     eeeee");
        cursor.setPosition({8, 0});
        cursor.deletePreviousWord();
        CHECK(docToVec(doc) == QVector<QString>{"a bb  c   dddd     eeeee"});
        CHECK(cursor.position() == TextCursor::Position{6, 0});

        cursor.selectAll();
        cursor.insertText("a bb  ccc   dddd     eeeee");
        cursor.setPosition({0, 0});
        CHECK(doc.lineCodeUnits(0) == 26);
        cursor.deleteWord();
        CHECK(docToVec(doc) == QVector<QString>{" bb  ccc   dddd     eeeee"});
        CHECK(doc.lineCodeUnits(0) == 25);
        cursor.deleteWord();
        CHECK(docToVec(doc) == QVector<QString>{"  ccc   dddd     eeeee"});
        CHECK(doc.lineCodeUnits(0) == 22);
        cursor.deleteWord();
        CHECK(docToVec(doc) == QVector<QString>{"   dddd     eeeee"});
        CHECK(doc.lineCodeUnits(0) == 17);
        cursor.deleteWord();
        CHECK(docToVec(doc) == QVector<QString>{"     eeeee"});
        CHECK(doc.lineCodeUnits(0) == 10);
        cursor.deleteWord();
        CHECK(doc.lineCodeUnits(0) == 0);
    }

    SECTION("delete-line") {
        bool emptyline = GENERATE(true, false);
        if (emptyline) {
            cursor.insertText("test test\n\ntest test");
            cursor.setPosition({0, 1});
            CHECK(cursor.position() == TextCursor::Position{0, 1});
        } else {
            cursor.insertText("test test\nremove remove\ntest test");
            cursor.setPosition({4, 0});
            cursor.moveDown(true);
            cursor.moveWordRight(true);
            CHECK(cursor.position() == TextCursor::Position{6, 1});
        }
        REQUIRE(doc.lineCount() == 3);

        cursor.deleteLine();
        CHECK(docToVec(doc) == QVector<QString>{"test test", "test test"});
        CHECK(cursor.position() == TextCursor::Position{0, 1});
        CHECK(doc.lineCount() == 2);
    }

    SECTION("delete-first-line") {
        bool emptyline = GENERATE(true, false);
        CAPTURE(emptyline);
        if (emptyline) {
            cursor.insertText("\ntest test\ntest test");
            cursor.setPosition({0, 0});
            CHECK(cursor.position() == TextCursor::Position{0, 0});
        } else {
            cursor.insertText("remove remove\ntest test\ntest test");
            cursor.setPosition({0, 1});
            cursor.moveUp(true);
            CHECK(cursor.position() == TextCursor::Position{0, 0});
        }
        REQUIRE(doc.lineCount() == 3);
        cursor.deleteLine();
        CHECK(docToVec(doc) == QVector<QString>{"test test", "test test"});
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        CHECK(doc.lineCount() == 2);
        CHECK(cursor.hasSelection() == false);
    }

    SECTION("delete-last-line") {
        bool emptyline = GENERATE(true, false);
        CAPTURE(emptyline);
        if (emptyline) {
            cursor.insertText("test test\ntest test\n");
            cursor.setPosition({0, 2});
            CHECK(cursor.position() == TextCursor::Position{0, 2});
        } else {
            cursor.insertText("test test\ntest test\nremove remove");
            cursor.setPosition({0, 2});
            CHECK(cursor.position() == TextCursor::Position{0, 2});
        }
        REQUIRE(doc.lineCount() == 3);
        cursor.deleteLine();
        CHECK(docToVec(doc) == QVector<QString>{"test test", "test test"});
        CHECK(cursor.position() == TextCursor::Position{9, 1});
        CHECK(doc.lineCount() == 2);
        CHECK(cursor.hasSelection() == false);
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
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        CHECK(cursor.selectionEndPos() == TextCursor::Position{1, 0});
        CHECK(cursor.anchor() == TextCursor::Position{1, 0});

        cursor.moveCharacterRight(true);
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());
        CHECK(cursor.position() == TextCursor::Position{1, 0});
        CHECK(cursor.anchor() == TextCursor::Position{1, 0});

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());
        CHECK(cursor.position() == TextCursor::Position{1, 0});
        CHECK(cursor.anchor() == TextCursor::Position{1, 0});

        cursor.moveCharacterLeft(true);
        CHECK(cursor.hasSelection() == true);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == TextCursor::Position{1, 0});
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        CHECK(cursor.anchor() == TextCursor::Position{1, 0});

        cursor.moveCharacterRight(true);
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());
        CHECK(cursor.position() == TextCursor::Position{1, 0});
        CHECK(cursor.anchor() == TextCursor::Position{1, 0});

        cursor.selectAll();
        CHECK(cursor.hasSelection() == true);
        cursor.insertText(" ");
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.selectionStartPos() == cursor.position());
        CHECK(cursor.selectionEndPos() == cursor.position());
        CHECK(cursor.position() == TextCursor::Position{1, 0});
        CHECK(cursor.anchor() == TextCursor::Position{1, 0});
    }

    SECTION("no-selection") {
        cursor.insertText("abc");
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.anchor() == TextCursor::Position{3, 0});
        CHECK(cursor.position() == TextCursor::Position{3, 0});

        cursor.moveCharacterLeft();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.anchor() == TextCursor::Position{2, 0});
        CHECK(cursor.position() == TextCursor::Position{2, 0});

        cursor.moveCharacterLeft(true);
        CHECK(cursor.hasSelection() == true);
        CHECK(cursor.anchor() == TextCursor::Position{2, 0});
        CHECK(cursor.position() == TextCursor::Position{1, 0});

        cursor.moveCharacterRight(true);
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.position() == TextCursor::Position{2, 0});
        CHECK(cursor.anchor() == TextCursor::Position{2, 0});

        cursor.deletePreviousCharacter();
        CHECK(doc.line(0) == "ac");
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.position() == TextCursor::Position{1, 0});
        CHECK(cursor.anchor() == TextCursor::Position{1, 0});
    }

    SECTION("selectAll") {
        cursor.selectAll();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.anchor() == TextCursor::Position{0, 0});
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        CHECK(cursor.selectedText() == "");

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.anchor() == TextCursor::Position{0, 0});
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        CHECK(cursor.selectedText() == "");

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.anchor() == TextCursor::Position{0, 0});
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        CHECK(cursor.selectedText() == "");

        cursor.insertText(" ");
        CHECK(cursor.selectedText() == "");
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.anchor() == TextCursor::Position{1, 0});
        CHECK(cursor.position() == TextCursor::Position{1, 0});

        cursor.selectAll();
        CHECK(cursor.selectedText() == " ");
        CHECK(cursor.hasSelection() == true);
        CHECK(cursor.anchor() == TextCursor::Position{0, 0});
        CHECK(cursor.position() == TextCursor::Position{1, 0});

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.anchor() == TextCursor::Position{1, 0});
        CHECK(cursor.position() == TextCursor::Position{1, 0});
        CHECK(cursor.selectedText() == "");

        cursor.moveCharacterLeft();

        cursor.clearSelection();
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.anchor() == TextCursor::Position{0, 0});
        CHECK(cursor.position() == TextCursor::Position{0, 0});
        CHECK(cursor.selectedText() == "");

        cursor.selectAll();
        CHECK(cursor.hasSelection() == true);
        CHECK(cursor.anchor() == TextCursor::Position{0, 0});
        CHECK(cursor.position() == TextCursor::Position{1, 0});

        cursor.insertText("\n");
        CHECK(cursor.selectedText() == "");
        CHECK(cursor.hasSelection() == false);
        CHECK(cursor.anchor() == TextCursor::Position{0, 1});
        CHECK(cursor.position() == TextCursor::Position{0, 1});

        cursor.selectAll();
        CHECK(cursor.selectedText() == "\n");
        CHECK(cursor.hasSelection() == true);
        CHECK(cursor.anchor() == TextCursor::Position{0, 0});
        CHECK(cursor.position() == TextCursor::Position{0, 1});
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
        CHECK(doc.line(0) == "ab");

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
        CHECK(doc.line(0) == "b");

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
        CHECK(doc.line(0) == "a");

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
        CHECK(doc.line(0) == "ab");

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

    SECTION("removeSelectedText cursorChanged signal") {
        cursor.insertText("some test text");
        wrappedCursor.setPosition({4, 0}, true);

        // signal emitted async, so needs to be consumed
        EventRecorder recorder;
        auto cursorChangedSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::cursorChanged));

        recorder.waitForEvent(cursorChangedSignal);
        CHECK(recorder.consumeFirst(cursorChangedSignal, (const TextCursor*)&cursor));
        CHECK(recorder.consumeFirst(cursorChangedSignal, (const TextCursor*)&wrappedCursor));
        CHECK(recorder.noMoreEvents());

        wrappedCursor.removeSelectedText();

        recorder.waitForEvent(cursorChangedSignal);
        CHECK(recorder.consumeFirst(cursorChangedSignal, (const TextCursor*)&cursor));
        CHECK(recorder.consumeFirst(cursorChangedSignal, (const TextCursor*)&wrappedCursor));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("removeSelectedText lineMarkerChanged signal") {
        cursor.insertText("\ntext");
        wrappedCursor.setPosition({0, 0});
        wrappedCursor.setPosition({0, 1}, true);

        EventRecorder recorder;
        auto lineMarkerChangedSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::lineMarkerChanged));

        LineMarker marker{&doc, 1};
        wrappedCursor.removeSelectedText();

        recorder.waitForEvent(lineMarkerChangedSignal);
        CAPTURE(docToVec(doc));
        CHECK(recorder.consumeFirst(lineMarkerChangedSignal, (const LineMarker*)&marker));
        CHECK(recorder.noMoreEvents());
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

    SECTION("userdata") {
        cursor.insertText("test\ntest");
        REQUIRE(doc.lineCount() == 2);
        const unsigned int revision1 = doc.revision();
        const unsigned int line0revision1 = doc.lineRevision(0);
        const auto userdata1 = std::make_shared<TestUserData>();
        doc.setLineUserData(0, userdata1);

        // doc and line revision do not change on user data update
        CHECK(doc.revision() == revision1);
        CHECK(doc.lineRevision(0) == line0revision1);
        CHECK(doc.lineUserData(0) == userdata1);
        CHECK(doc.lineUserData(1) == nullptr);

        cursor.setPosition({0, 0});
        cursor.insertText("new");

        const unsigned int revision2 = doc.revision();
        const unsigned int line0revision2 = doc.lineRevision(0);

        CHECK(revision1 != revision2);
        CHECK(line0revision1 != line0revision2);
        // User data stays on line on modification
        CHECK(doc.lineUserData(0) == userdata1);
        CHECK(doc.lineUserData(1) == nullptr);

        const auto userdata2 = std::make_shared<TestUserData>();
        doc.setLineUserData(0, userdata2);

        CHECK(revision1 != revision2);
        CHECK(line0revision1 != line0revision2);

        // The new snapshot does have the newly set user data.
        CHECK(doc.lineUserData(0) == userdata2);
        CHECK(doc.lineUserData(1) == nullptr);

        // line data stays with left part of split line
        cursor.insertText("\nnova");
        CHECK(doc.lineUserData(0) == userdata2);
        CHECK(doc.lineUserData(1) == nullptr);
        CHECK(doc.lineUserData(2) == nullptr);

        const auto userdata3 = std::make_shared<TestUserData>();
        doc.setLineUserData(1, userdata3);
        CHECK(doc.lineUserData(1) == userdata3);

        cursor.setPosition({0, 0});
        cursor.moveToEndOfLine();

        // When removing line break, user data from top line is preserved.
        cursor.deleteCharacter();
        CHECK(doc.lineUserData(0) == userdata2);
        CHECK(doc.lineUserData(1) == nullptr);
    }

    SECTION("snapshot") {
        cursor.insertText("test\ntest");
        const auto userdata1 = std::make_shared<TestUserData>();
        doc.setLineUserData(0, userdata1);

        CHECK(docToVec(doc) == QVector<QString>{"test", "test"});

        DocumentSnapshot snap = doc.snapshot();

        CHECK(snap.isUpToDate() == true);
        CHECK(snap.revision() == doc.revision());
        CHECK(snapToVec(snap) == QVector<QString>{"test", "test"});
        REQUIRE(snap.lineCount() == 2);
        CHECK(snap.lineCodeUnits(0) == 4);
        CHECK(snap.lineCodeUnits(1) == 4);
        REQUIRE(doc.lineCount() == 2);
        CHECK(snap.lineUserData(0) == userdata1);
        CHECK(snap.lineUserData(1) == nullptr);
        CHECK(snap.lineRevision(0) == doc.lineRevision(0));
        CHECK(snap.lineRevision(1) == doc.lineRevision(1));

        cursor.insertText("new");

        CHECK(snap.isUpToDate() == false);
        CHECK(snap.revision() != doc.revision());
        CHECK(snapToVec(snap) == QVector<QString>{"test", "test"});
        CHECK(snap.lineUserData(0) == userdata1);
        CHECK(snap.lineUserData(1) == nullptr);
        CHECK(snap.lineRevision(0) == doc.lineRevision(0));
        CHECK(snap.lineRevision(1) != doc.lineRevision(1));

        DocumentSnapshot snap2 = doc.snapshot();
        CHECK(snap2.isUpToDate() == true);
        CHECK(snap2.revision() == doc.revision());
        CHECK(snapToVec(snap2) == QVector<QString>{"test", "testnew"});
        REQUIRE(snap2.lineCount() == 2);
        CHECK(snap2.lineCodeUnits(0) == 4);
        CHECK(snap2.lineCodeUnits(1) == 7);
        REQUIRE(doc.lineCount() == 2);
        CHECK(snap2.lineUserData(0) == userdata1);
        CHECK(snap2.lineUserData(1) == nullptr);
        CHECK(snap2.lineRevision(0) == doc.lineRevision(0));
        CHECK(snap2.lineRevision(1) == doc.lineRevision(1));

        const auto userdata2 = std::make_shared<TestUserData>();
        doc.setLineUserData(0, userdata2);

        // Changing line user data does not invalidate snapshots
        CHECK(snap2.isUpToDate() == true);

        // But the snapshot still has the old data
        CHECK(snap2.lineUserData(0) == userdata1);
        CHECK(snap2.lineUserData(1) == nullptr);

        DocumentSnapshot snap3 = doc.snapshot();
        // The new snapshot does have the newly set user data.
        CHECK(snap3.lineUserData(0) == userdata2);
        CHECK(snap3.lineUserData(1) == nullptr);
    }

}

TEST_CASE("Cursor") {
    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);

    Document doc;

    TextCursor cursor1{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };
    cursor1.insertText(" \n \n \n \n");

    TextCursor cursor2{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    CHECK(cursor1.position() == TextCursor::Position{0, 4});
    CHECK(cursor2.position() == TextCursor::Position{0, 0});

    SECTION("delete-previous-line") {
        cursor2.setPosition({0, 3});
        CHECK(cursor2.position() == TextCursor::Position{0, 3});
        CHECK(doc.lineCount() == 5);
        cursor2.deleteCharacter();
        cursor2.deleteCharacter();
        CHECK(cursor2.position() == TextCursor::Position{0, 3});
        CHECK(cursor1.position() == TextCursor::Position{0, 3});
        CHECK(doc.lineCount() == 4);
    }
    SECTION("delete-on-cursor") {
        cursor1.setPosition({0, 2});
        cursor2.setPosition({0, 2});
        cursor1.deleteCharacter();
        CHECK(cursor1.position() == TextCursor::Position{0, 2});
        CHECK(cursor2.position() == TextCursor::Position{0, 2});
        cursor1.deletePreviousCharacter();
        CHECK(cursor1.position() == TextCursor::Position{1, 1});
        CHECK(cursor2.position() == TextCursor::Position{1, 1});
    }
    SECTION("sort") {
        cursor1.selectAll();
        cursor1.insertText("3\n4\n2\n1");
        cursor1.setPosition({0, 1});
        cursor1.moveCharacterRight(true);
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectedText() == "4");

        cursor2.selectAll();
        CHECK(cursor1.position() == TextCursor::Position{1, 1});
        CHECK(cursor2.position() == TextCursor::Position{1, 3});

        doc.sortLines(0, 4, &cursor2);
        CHECK(doc.lineCount() == 4);
        REQUIRE(doc.line(0) == "1");
        REQUIRE(doc.line(1) == "2");
        REQUIRE(doc.line(2) == "3");
        REQUIRE(doc.line(3) == "4");

        CHECK(cursor1.position() == TextCursor::Position{1, 3});
        CHECK(cursor2.position() == TextCursor::Position{1, 0});
        CHECK(cursor1.hasSelection() == true);
        CHECK(cursor1.selectedText() == "4");
        CHECK(cursor2.hasSelection() == true);
    }
    SECTION("select") {
        cursor1.setPosition({0, 2});
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
            cursor2.setPosition({5, 2});

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
                cursor2.setPosition({0, 2});
                cursor2.deletePreviousCharacter();
                CHECK(cursor1.selectedText() == "hallo welt");
            }
            SECTION("del-all") {
                cursor2.selectAll();
                cursor2.deletePreviousCharacter();
                CHECK(cursor1.selectedText() == "");
                CHECK(cursor1.hasSelection() == false);
                CHECK(cursor1.position() == TextCursor::Position{0, 0});
                CHECK(cursor2.position() == TextCursor::Position{0, 0});
            }
        }

        SECTION("before-selection") {
            cursor2.setPosition({0, 2});

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
            cursor2.setPosition({10, 2});

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
            cursor1.setPosition({0, 0});
            cursor1.insertText("\n");
            CHECK(marker.line() == 5);
            CHECK(marker2.line() == 1);

            // cursor1 add lines
            marker.setLine(5);
            CHECK(marker.line() == 5);
            cursor1.setPosition({0, 5});
            cursor1.insertText(" ");
            CHECK(cursor1.position() == TextCursor::Position{1, 5});
            cursor1.insertText("\n");
            CHECK(marker.line() == 5);
            CHECK(marker2.line() == 1);

            // wrong marker
            marker.setLine(-1);
            CHECK(marker.line() == 0);
            cursor1.setPosition({0, 0});
            cursor1.insertText("\n");
            CHECK(marker.line() == 1);
            CHECK(marker2.line() == 2);

            // cursor1 add and remove line
            marker.setLine(1);
            CHECK(marker.line() == 1);
            cursor1.setPosition({0, 0});
            cursor1.insertText("\n");
            CHECK(marker.line() == 2);
            CHECK(marker2.line() == 3);

            // cursor2 add and remove line
            cursor2.setPosition({0, 0});
            cursor2.insertText("\n");
            CHECK(marker.line() == 3);
            cursor2.deleteLine();
            CHECK(marker.line() == 2);

            // delete the line with marker
            cursor1.setPosition({0, 2});
            cursor1.deleteLine();
            CHECK(marker.line() == 2);

            // delete the lines with marker
            cursor2.setPosition({0, 1});
            cursor2.setPosition({0, 3}, true);
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
        return { {documentContents, false, {0, 0}, {lines.last().size(), lines.size() - 1}, {0, 0}, {0, 0}, {}} };
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
        return { {documentContents, false, {0, 0}, {lines.last().size(), lines.size() - 1}, {0, 0}, {0, 0}, {}} };
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

    TextCursor cursor1{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
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

    SECTION("empty search string") {
        static auto testCases = generateTestCases(R"(
                                                  0|Test
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, "", Qt::CaseSensitive);
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

    SECTION("backward empty search string") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|Test
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, "", Qt::CaseSensitive);
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

    TextCursor cursor1{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
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

    SECTION("invalid regex") {
        static auto testCases = generateTestCases(R"(
                                                  0|Test
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("["), Qt::CaseSensitive, {});
    }

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

    SECTION("literal astral") {
        static auto testCases = generateTestCases(R"(
                                                  0|some Text
                                                  1|s😁me Thing
                                                   > 11
                                                  2|😁aa bbbb
                                                   >22
                                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression("😁"), Qt::CaseSensitive,
                {
                      {"1", MatchCaptures{ {"😁"}, {}}},
                      {"2", MatchCaptures{ {"😁"}, {}}}
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

    SECTION("multiline dotall prefix") {
        static auto testCases = generateTestCases(R"(
                                  0|some Test
                                  1|abc Thing
                                   >    11111
                                  2|xabcabc bbbb
                                   >1111
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecks(testCase, QRegularExpression{"T[^T]*xabc", QRegularExpression::PatternOption::DotMatchesEverythingOption},
                  Qt::CaseSensitive, {
              {"1", MatchCaptures{ {"Thing\nxabc"}, {}}},
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

    SECTION("backward invalid regex") {
        static auto testCases = generateTestCasesBackward(R"(
                                                  0|Test
                                              )");
        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("["), Qt::CaseSensitive, {});
    }

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

    SECTION("backward literal-abc with mismatched multi line option") {
        static auto testCases = generateTestCasesBackward(R"(
            0|some Test
            1|abc Thing
             >111
            2|xabcabc bbbb
             > 222333
        )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("abc", QRegularExpression::PatternOption::MultilineOption),
                          Qt::CaseSensitive, {
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

    SECTION("backward multiline line literal with regex option already set.") {
        static auto testCases = generateTestCasesBackward(R"(
                                  0|some Test
                                   >        1
                                  1|abc Thing
                                   >1
                                  2|xabcabc bbbb
                              )");

        auto testCase = GENERATE(from_range(testCases));

        runChecksBackward(testCase, QRegularExpression("t\na", QRegularExpression::PatternOption::MultilineOption),
                          Qt::CaseSensitive, {
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

TEST_CASE("async search") {
    // We basically assume that async search uses the same internal search functions as the sync search,
    // so this only tests searching lightly and focuses on the async interface.

    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);
    Document doc;

    TextCursor cursor1{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    SECTION("search substring") {
        cursor1.insertText("test a, which is a test that tests a.");
        cursor1.setPosition({0, 0});
        QFuture<DocumentFindAsyncResult> future = doc.findAsync("a", cursor1);

        future.waitForFinished();
        REQUIRE(future.isFinished());
        REQUIRE(future.isResultReadyAt(0) == true);
        DocumentFindAsyncResult result = future.result();

        CHECK(result.anchor == TextCursor::Position{5, 0});
        CHECK(result.cursor == TextCursor::Position{6, 0});
        CHECK(result.revision == doc.revision());

        // repeat with same cursor should give same result
        future = doc.findAsync("a", cursor1);
        future.waitForFinished();
        REQUIRE(future.isFinished());
        REQUIRE(future.isResultReadyAt(0) == true);
        result = future.result();

        CHECK(result.anchor == TextCursor::Position{5, 0});
        CHECK(result.cursor == TextCursor::Position{6, 0});
        CHECK(result.revision == doc.revision());

        auto cursor2 = cursor1;

        std::vector<int> positions = {17, 26, 35};
        for (int expected: positions) {
            CAPTURE(expected);
            cursor2.setPosition(result.anchor);
            cursor2.setPosition(result.cursor, true);
            // repeat with cursor build from search result should find next result;
            future = doc.findAsync("a", cursor2);
            future.waitForFinished();
            REQUIRE(future.isFinished());
            REQUIRE(future.isResultReadyAt(0) == true);
            result = future.result();

            CHECK(result.anchor == TextCursor::Position{expected, 0});
            CHECK(result.cursor == TextCursor::Position{expected + 1, 0});
            CHECK(result.revision == doc.revision());
        }
    }

    SECTION("empty search string") {
        cursor1.insertText("test a, which is a test that tests a.");
        QFuture<DocumentFindAsyncResult> future = doc.findAsync("", cursor1);
        future.waitForFinished();
        REQUIRE(future.isFinished());
        REQUIRE(future.isResultReadyAt(0) == true);
        DocumentFindAsyncResult result = future.result();

        CHECK(result.anchor == result.cursor);
        CHECK(result.revision == doc.revision());
    }

    SECTION("search substring reverse") {
        cursor1.insertText("test a, which is a test that tests a.");
        QFuture<DocumentFindAsyncResult> future = doc.findAsync("a", cursor1, Document::FindFlag::FindBackward);
        future.waitForFinished();
        REQUIRE(future.isFinished());
        REQUIRE(future.isResultReadyAt(0) == true);
        DocumentFindAsyncResult result = future.result();

        CHECK(result.anchor == TextCursor::Position{35, 0});
        CHECK(result.cursor == TextCursor::Position{36, 0});
        CHECK(result.revision == doc.revision());
    }

    SECTION("search regex") {
        cursor1.insertText("test a, which is a test that tests a.");
        cursor1.setPosition({0, 0});
        QFuture<DocumentFindAsyncResult> future = doc.findAsync(QRegularExpression(" ([^ ]+h)[^ ]+ "), cursor1);

        future.waitForFinished();
        REQUIRE(future.isFinished());
        REQUIRE(future.isResultReadyAt(0) == true);
        DocumentFindAsyncResult result = future.result();

        CHECK(result.anchor == TextCursor::Position{7, 0});
        CHECK(result.cursor == TextCursor::Position{14, 0});
        CHECK(result.revision == doc.revision());
        CHECK(result.regexLastCapturedIndex() == 1);
        CHECK(result.regexCapture(0) == QString(" which "));
        CHECK(result.regexCapture(1) == QString("wh"));
    }

    SECTION("invalid regex") {
        cursor1.insertText("test a, which is a test that tests a.");
        QFuture<DocumentFindAsyncResult> future = doc.findAsync(QRegularExpression("["), cursor1);
        future.waitForFinished();
        REQUIRE(future.isFinished());
        REQUIRE(future.isResultReadyAt(0) == true);
        DocumentFindAsyncResult result = future.result();

        CHECK(result.anchor == result.cursor);
        CHECK(result.revision == doc.revision());
    }

    SECTION("search regex reverse") {
        cursor1.insertText("test a, which is a test that tests a.");
        QFuture<DocumentFindAsyncResult> future = doc.findAsync(QRegularExpression(" ([^ ]+h)[^ ]+ "), cursor1,
                                                                Document::FindFlag::FindBackward);

        future.waitForFinished();
        REQUIRE(future.isFinished());
        REQUIRE(future.isResultReadyAt(0) == true);
        DocumentFindAsyncResult result = future.result();

        CHECK(result.anchor == TextCursor::Position{23, 0});
        CHECK(result.cursor == TextCursor::Position{29, 0});
        CHECK(result.revision == doc.revision());
        CHECK(result.regexLastCapturedIndex() == 1);
        CHECK(result.regexCapture(0) == QString(" that "));
        CHECK(result.regexCapture(1) == QString("th"));
    }

    SECTION("search regex named capture") {
        cursor1.insertText("test a, which is a test that tests a.");
        cursor1.setPosition({0, 0});
        QFuture<DocumentFindAsyncResult> future = doc.findAsync(QRegularExpression(" (?<capture>[^ ]+h)[^ ]+ "), cursor1);

        future.waitForFinished();
        REQUIRE(future.isFinished());
        REQUIRE(future.isResultReadyAt(0) == true);
        DocumentFindAsyncResult result = future.result();

        CHECK(result.anchor == TextCursor::Position{7, 0});
        CHECK(result.cursor == TextCursor::Position{14, 0});
        CHECK(result.revision == doc.revision());
        CHECK(result.regexLastCapturedIndex() == 1);
        CHECK(result.regexCapture(0) == QString(" which "));
        CHECK(result.regexCapture("capture") == QString("wh"));
    }

    SECTION("search regex with changes") {
        cursor1.insertText("test a, which is a test that tests a.");
        cursor1.setPosition({0, 0});
        QFuture<DocumentFindAsyncResult> future = doc.findAsync(QRegularExpression(" ([^ ]+h)[^ ]+ "), cursor1);

        future.waitForFinished();
        REQUIRE(future.isFinished());
        REQUIRE(future.isResultReadyAt(0) == true);
        DocumentFindAsyncResult result = future.result();
        cursor1.insertText("dummy");

        CHECK(result.anchor == TextCursor::Position{7, 0});
        CHECK(result.cursor == TextCursor::Position{14, 0});
        CHECK(result.revision != doc.revision());
        CHECK(result.regexLastCapturedIndex() == 1);
        CHECK(result.regexCapture(0) == QString(" which "));
        CHECK(result.regexCapture(1) == QString("wh"));
    }

    SECTION("search regex with cancel") {
        // We can only check if canceling the future really stops the search thread by timing the usage of the
        // threads in the thread pool.
        // We increase the searched text until we reach a decent time the search takes and then check if
        // canceling reduces the time to almost nothing.

        // There is a possibility that timing differences make this test success even when cancelation
        // does not work.

        cursor1.insertText("test a, which is a test that tests a.X");

        QFuture<DocumentFindAsyncResult> future;
        QElapsedTimer timer;

        int sizeIncrement = 100000;

        int uncanceledInMs = 0;

        bool testSuccessful = false;

        while (uncanceledInMs < 30000) {
            cursor1.setPosition({0, 0});
            cursor1.insertText(QString(' ').repeated(sizeIncrement) + "\n");
            timer.start();
            cursor1.setPosition({0, 0});
            future = doc.findAsync(QRegularExpression(".*X"), cursor1);
            future.waitForFinished();
            sizeIncrement *= 2;

            uncanceledInMs = timer.elapsed();

            if (uncanceledInMs > 300) {
                INFO("ms for uncanceled run: " << timer.elapsed());

                cursor1.setPosition({0, 0});
                timer.start();
                future = doc.findAsync(QRegularExpression(".*X"), cursor1);

                while (QThreadPool::globalInstance()->activeThreadCount() == 0) {
                    // wait
                }

                future.cancel();

                future.waitForFinished();
                REQUIRE(future.isFinished());
                REQUIRE(future.isCanceled());

                while (QThreadPool::globalInstance()->activeThreadCount() != 0) {
                    // wait
                }
                const int elapsedInMs = timer.elapsed();
                INFO("ms for canceled run: " << elapsedInMs);
                if (elapsedInMs < uncanceledInMs / 10) {
                    testSuccessful = true;
                    break;
                }
            }
        }
        CHECK(testSuccessful);
    }
}


TEST_CASE("Document Undo Redo") {
    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);
    Document doc;

    TextCursor cursor1{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    enum ActionType {
        Collapsed,
        Invisible,
        NormalStp
    };

    struct Action {
        ActionType type;
        std::function<void()> step;
    };

    auto runChecks = [&](std::initializer_list<Action> steps) {

        const bool groupSteps = GENERATE(false, true);
        CAPTURE(groupSteps);

        struct DocumentState {
            QList<QString> lines;
            bool noNewline = false;
            TextCursor::Position startCursorPos;
            TextCursor::Position endCursorPos;
        };

        QList<DocumentState> states;
        TextCursor::Position beforePos = cursor1.position();

        auto captureState = [&] {
            QList<QString> lines;
            for (int i = 0; i < doc.lineCount(); i++) {
                lines.append(doc.line(i));
            }
            states.append(DocumentState{lines, doc.noNewLine(), beforePos, cursor1.position()});
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
                group.emplace(doc.startUndoGroup(&cursor1));
            }
            step.step();
            if (step.type == NormalStp) {
                captureState();
                compareState(states.last());
                beforePos = cursor1.position();
            } else if (step.type == Invisible) {
                beforePos = cursor1.position();
            }
        }

        int stateIndex = states.size() - 1;


        while (doc.isUndoAvailable()) {
            CAPTURE(stateIndex);
            doc.undo(&cursor1);
            REQUIRE(stateIndex > 0);
            CHECK(states[stateIndex].startCursorPos == cursor1.position());
            stateIndex -= 1;
            compareState(states[stateIndex]);
        }

        REQUIRE(stateIndex == 0);

        while (doc.isRedoAvailable()) {
            doc.redo(&cursor1);
            REQUIRE(stateIndex + 1 < states.size());
            stateIndex += 1;
            compareState(states[stateIndex]);
            CHECK(states[stateIndex].endCursorPos == cursor1.position());
        }

        REQUIRE(stateIndex + 1 == states.size());
    };

    SECTION("undo on start") {
        // Should silently do nothing
        doc.undo(&cursor1);
    }

    SECTION("undo and change") {
        cursor1.insertText("first");
        doc.undo(&cursor1);
        CHECK(docToVec(doc) == QVector<QString>{""});
        cursor1.insertText("second");
        CHECK(docToVec(doc) == QVector<QString>{"second"});
        doc.undo(&cursor1);
        CHECK(docToVec(doc) == QVector<QString>{""});
        doc.redo(&cursor1);
        CHECK(docToVec(doc) == QVector<QString>{"second"});
    }

    SECTION("redo on start") {
        // Should silently do nothing
        doc.redo(&cursor1);
    }

    SECTION("undoAvailable signal") {
        cursor1.insertText("test test");

        doc.undo(&cursor1);

        EventRecorder recorder;

        // signal emited sync
        auto undoAvailableSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::undoAvailable));

        doc.redo(&cursor1);

        recorder.waitForEvent(undoAvailableSignal);
        CHECK(recorder.consumeFirst(undoAvailableSignal, true));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("redoAvailable signal") {
        cursor1.insertText("test test");

        EventRecorder recorder;

        // signal emited sync
        auto redoAvailableSignal = recorder.watchSignal(&doc, RECORDER_SIGNAL(&Document::redoAvailable));

        doc.undo(&cursor1);

        recorder.waitForEvent(redoAvailableSignal);
        CHECK(recorder.consumeFirst(redoAvailableSignal, true));
        CHECK(recorder.noMoreEvents());

        doc.redo(&cursor1);

        recorder.waitForEvent(redoAvailableSignal);
        CHECK(recorder.consumeFirst(redoAvailableSignal, false));
        CHECK(recorder.noMoreEvents());
    }

    SECTION("inserts") {
        runChecks({
                      { Collapsed, [&] { cursor1.insertText("a"); } },
                      { Collapsed, [&] { cursor1.insertText("b"); } },
                      { NormalStp, [&] { cursor1.insertText("c"); } }
                  });
    }

    SECTION("inserts2") {
        runChecks({
                      { Collapsed, [&] { cursor1.insertText("a"); } },
                      { Collapsed, [&] { cursor1.insertText("b"); } },
                      { NormalStp, [&] { cursor1.insertText("c"); } },
                      { Collapsed, [&] { cursor1.insertText(" "); } },
                      { Collapsed, [&] { cursor1.insertText("d"); } },
                      { Collapsed, [&] { cursor1.insertText("e"); } },
                      { NormalStp, [&] { cursor1.insertText("f"); } }
                  });
    }

    SECTION("insert and move") {
        runChecks({
                      { Collapsed, [&] { cursor1.insertText("a"); } },
                      { Collapsed, [&] { cursor1.insertText("b"); } },
                      { NormalStp, [&] { cursor1.insertText("c"); } },
                      { Invisible, [&] { cursor1.moveCharacterLeft(); } },
                      { Collapsed, [&] { cursor1.insertText("d"); } },
                      { Collapsed, [&] { cursor1.insertText("e"); } },
                      { NormalStp, [&] { cursor1.insertText("f"); } }
                  });
    }

    SECTION("insert and cancle collapsing") {
        runChecks({
                      { Collapsed, [&] { cursor1.insertText("a"); } },
                      { Collapsed, [&] { cursor1.insertText("b"); } },
                      { NormalStp, [&] { cursor1.insertText("c"); } },
                      { Invisible, [&] { doc.clearCollapseUndoStep(); } },
                      { Collapsed, [&] { cursor1.insertText("d"); } },
                      { Collapsed, [&] { cursor1.insertText("e"); } },
                      { NormalStp, [&] { cursor1.insertText("f"); } }
                  });
    }

    SECTION("no newline") {
        runChecks({
                      { Collapsed, [&] { cursor1.insertText("a"); } },
                      { Collapsed, [&] { cursor1.insertText("b"); } },
                      { NormalStp, [&] { cursor1.insertText("c"); } },
                      { NormalStp, [&] { doc.setNoNewline(true); } }
                  });
    }

    SECTION("group with cursor movement") {
        runChecks({
                      { NormalStp, [&] { cursor1.insertText("    "); } },
                      { NormalStp, [&] { cursor1.insertText("    "); } },
                      { NormalStp, [&] { cursor1.insertText("    "); } },
                      { NormalStp, [&] {
                            const auto [cursorCodeUnit, cursorLine] = cursor1.position();
                            auto group = doc.startUndoGroup(&cursor1);
                            cursor1.setPosition({0, 0});
                            cursor1.setPosition({4, 0}, true);
                            cursor1.removeSelectedText();
                            cursor1.setPosition({cursorCodeUnit - 4, cursorLine});
                        } }
                  });
    }

    SECTION("newline") {
        runChecks({
                      { NormalStp, [&] { cursor1.insertText("\n"); } },
                      { Collapsed, [&] { cursor1.insertText("\n"); } },
                      { Collapsed, [&] { cursor1.insertText("a"); } },
                      { NormalStp, [&] { cursor1.insertText("b"); } },
                      { Collapsed, [&] { cursor1.insertText("\n"); } },
                      { NormalStp, [&] { cursor1.insertText("c"); } },
                      { NormalStp, [&] { cursor1.insertText("\n"); } }
                  });
    }

    SECTION("space") {
        runChecks({
                      { NormalStp, [&] { cursor1.insertText(" "); } },
                      { Collapsed, [&] { cursor1.insertText(" "); } },
                      { Collapsed, [&] { cursor1.insertText("a"); } },
                      { NormalStp, [&] { cursor1.insertText("b"); } },
                      { Collapsed, [&] { cursor1.insertText(" "); } },
                      { NormalStp, [&] { cursor1.insertText("c"); } },
                      { NormalStp, [&] { cursor1.insertText(" "); } }
                  });
    }


    SECTION("tab") {
        runChecks({
                      { NormalStp, [&] { cursor1.insertText("\t"); } },
                      { Collapsed, [&] { cursor1.insertText("\t"); } },
                      { Collapsed, [&] { cursor1.insertText("a"); } },
                      { NormalStp, [&] { cursor1.insertText("b"); } },
                      { Collapsed, [&] { cursor1.insertText("\t"); } },
                      { NormalStp, [&] { cursor1.insertText("c"); } },
                      { NormalStp, [&] { cursor1.insertText("\t"); } }
                  });
    }

}

TEST_CASE("Document additional cursor adjustments") {
    // Check if modifiation, undo and redo correctly moves other cursors active on the document.

    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);
    Document doc;

    TextCursor cursor1{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    QStringList lines;

    auto positionAt = [&] (QChar ch) -> TextCursor::Position {
        int lineNo = 0;
        for (const QString &line: lines) {
            if (line.contains(ch)) {
                return {line.indexOf(ch), lineNo};
            }
            lineNo += 1;
        }
        // should never be reached
        REQUIRE(false);
        return {0, 0};
    };

    auto posAdd = [] (TextCursor::Position in, int codeUnit, int line) -> TextCursor::Position {
        return {in.codeUnit + codeUnit, in.line + line};
    };

    SECTION("Insert midline") {
        lines = QStringList{
            "A            B",
            "C >< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        TextCursor curA = cursor1;
        curA.setPosition(positionAt('A'));

        TextCursor curB = cursor1;
        curB.setPosition(positionAt('B'));

        TextCursor curBtoE = cursor1;
        curBtoE.setPosition(positionAt('B'));
        curBtoE.setPosition(positionAt('E'), true);

        TextCursor curEtoB = cursor1;
        curEtoB.setPosition(positionAt('E'));
        curEtoB.setPosition(positionAt('B'), true);

        TextCursor curC = cursor1;
        curC.setPosition(positionAt('C'));

        TextCursor curCtoD = cursor1;
        curCtoD.setPosition(positionAt('C'));
        curCtoD.setPosition(positionAt('D'), true);

        TextCursor curDtoC = cursor1;
        curDtoC.setPosition(positionAt('D'));
        curDtoC.setPosition(positionAt('C'), true);

        TextCursor curBefore = cursor1;
        curBefore.setPosition(positionAt('>'));

        TextCursor curAfter = cursor1;
        curAfter.setPosition(positionAt('<'));

        TextCursor curD = cursor1;
        curD.setPosition(positionAt('D'));

        TextCursor curE = cursor1;
        curE.setPosition(positionAt('E'));

        TextCursor curF = cursor1;
        curF.setPosition(positionAt('F'));

        cursor1.setPosition(positionAt('<'));
        const QString insertedText = "XXXXXXXXXXX";
        const int insertedLen = insertedText.size();
        cursor1.insertText(insertedText);



        const auto posA2 = curA.position();
        const auto posB2 = curB.position();
        const auto posC2 = curC.position();
        const auto posBefore2 = curBefore.position();
        const auto posAfter2 = curAfter.position();
        const auto posD2 = curD.position();
        const auto posE2 = curE.position();
        const auto posF2 = curF.position();

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == posAdd(positionAt('D'), insertedLen, 0));
        CHECK(curDtoC.anchor() == posAdd(positionAt('D'), insertedLen, 0));
        CHECK(curDtoC.position() == positionAt('C'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == posAdd(positionAt('<'), insertedLen, 0));
        CHECK(curD.position() == posAdd(positionAt('D'), insertedLen, 0));
        CHECK(curE.position() == positionAt('E'));
        CHECK(curEtoB.anchor() == positionAt('E'));
        CHECK(curEtoB.position() == positionAt('B'));
        CHECK(curF.position() == positionAt('F'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        TextCursor curInside = curBefore;
        curInside.moveCharacterRight();
        CHECK(curInside.position() == posAdd(positionAt('>'), 1, 0));
        CHECK(curInside.hasSelection() == false);

        TextCursor curInside2 = curInside;
        curInside2.moveCharacterRight();
        CHECK(curInside2.position() == posAdd(positionAt('>'), 2, 0));
        CHECK(curInside2.hasSelection() == false);

        TextCursor curInside3 = curAfter;
        curInside3.moveCharacterLeft();
        CHECK(curInside3.position() == posAdd(positionAt('<'), insertedLen - 1, 0));
        CHECK(curInside3.hasSelection() == false);

        TextCursor curInside4 = curInside3;
        curInside4.moveCharacterLeft();
        CHECK(curInside4.position() == posAdd(positionAt('<'), insertedLen - 2, 0));
        CHECK(curInside4.hasSelection() == false);

        TextCursor curInside1ToInside3 = curInside;
        curInside1ToInside3.setPosition(curInside3.position(), true);

        doc.undo(&cursor1);

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == positionAt('D'));
        CHECK(curDtoC.anchor() == positionAt('D'));
        CHECK(curDtoC.position() == positionAt('C'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == positionAt('<'));
        CHECK(curD.position() == positionAt('D'));
        CHECK(curE.position() == positionAt('E'));
        CHECK(curF.position() == positionAt('F'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == positionAt('<'));
        CHECK(curInside2.position() == positionAt('<'));
        CHECK(curInside3.position() == positionAt('<'));
        CHECK(curInside4.position() == positionAt('<'));
        CHECK(curInside1ToInside3.position() == positionAt('<'));
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);

        doc.redo(&cursor1);

        CHECK(curA.position() == posA2);
        CHECK(curB.position() == posB2);
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curC.position() == posC2);
        CHECK(curBefore.position() == posBefore2);
        CHECK(curAfter.position() == posAfter2);
        CHECK(curD.position() == posD2);
        CHECK(curE.position() == posE2);
        CHECK(curF.position() == posF2);

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == posAdd(positionAt('D'), insertedLen, 0));
        CHECK(curDtoC.anchor() == posAdd(positionAt('D'), insertedLen, 0));
        CHECK(curDtoC.position() == positionAt('C'));
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == posAdd(positionAt('<'), insertedLen, 0));
        CHECK(curInside2.position() == posAdd(positionAt('<'), insertedLen, 0));
        CHECK(curInside3.position() == posAdd(positionAt('<'), insertedLen, 0));
        CHECK(curInside4.position() == posAdd(positionAt('<'), insertedLen, 0));
        CHECK(curInside1ToInside3.position() == posAdd(positionAt('<'), insertedLen, 0));
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);
    }

    SECTION("Insert midline two lines") {
        lines = QStringList{
            "A            B",
            "C >< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        TextCursor curA = cursor1;
        curA.setPosition(positionAt('A'));

        TextCursor curB = cursor1;
        curB.setPosition(positionAt('B'));

        TextCursor curBtoE = cursor1;
        curBtoE.setPosition(positionAt('B'));
        curBtoE.setPosition(positionAt('E'), true);

        TextCursor curEtoB = cursor1;
        curEtoB.setPosition(positionAt('E'));
        curEtoB.setPosition(positionAt('B'), true);

        TextCursor curC = cursor1;
        curC.setPosition(positionAt('C'));

        TextCursor curCtoD = cursor1;
        curCtoD.setPosition(positionAt('C'));
        curCtoD.setPosition(positionAt('D'), true);

        TextCursor curDtoC = cursor1;
        curDtoC.setPosition(positionAt('D'));
        curDtoC.setPosition(positionAt('C'), true);

        TextCursor curBefore = cursor1;
        curBefore.setPosition(positionAt('>'));

        TextCursor curAfter = cursor1;
        curAfter.setPosition(positionAt('<'));

        TextCursor curD = cursor1;
        curD.setPosition(positionAt('D'));

        TextCursor curE = cursor1;
        curE.setPosition(positionAt('E'));

        TextCursor curF = cursor1;
        curF.setPosition(positionAt('F'));

        cursor1.setPosition(positionAt('<'));
        const int insertionOff = cursor1.position().codeUnit;
        const QString insertedTextLine1 = "XXX";
        const QString insertedTextLine2 = "XXXXXXXX";
        const QString insertedText = insertedTextLine1 + "\n" + insertedTextLine2;
        const int insertedLen2 = insertedTextLine2.size();
        cursor1.insertText(insertedText);



        const auto posA2 = curA.position();
        const auto posB2 = curB.position();
        const auto posC2 = curC.position();
        const auto posBefore2 = curBefore.position();
        const auto posAfter2 = curAfter.position();
        const auto posD2 = curD.position();
        const auto posE2 = curE.position();
        const auto posF2 = curF.position();

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == posAdd(positionAt('E'), 0, 1));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == posAdd(positionAt('D'), -insertionOff + insertedLen2, 1));
        CHECK(curDtoC.anchor() == posAdd(positionAt('D'), -insertionOff + insertedLen2, 1));
        CHECK(curDtoC.position() == positionAt('C'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == posAdd(positionAt('<'), -insertionOff + insertedLen2, 1));
        CHECK(curD.position() == posAdd(positionAt('D'), -insertionOff + insertedLen2, 1));
        CHECK(curE.position() == posAdd(positionAt('E'), 0, 1));
        CHECK(curEtoB.anchor() == posAdd(positionAt('E'), 0, 1));
        CHECK(curEtoB.position() == positionAt('B'));
        CHECK(curF.position() == posAdd(positionAt('F'), 0, 1));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        TextCursor curInside = curBefore;
        curInside.moveCharacterRight();
        CHECK(curInside.position() == posAdd(positionAt('>'), 1, 0));
        CHECK(curInside.hasSelection() == false);

        TextCursor curInside2 = curInside;
        curInside2.moveCharacterRight();
        CHECK(curInside2.position() == posAdd(positionAt('>'), 2, 0));
        CHECK(curInside2.hasSelection() == false);

        TextCursor curInside3 = curAfter;
        curInside3.moveCharacterLeft();
        CHECK(curInside3.position() == posAdd(positionAt('<'), -insertionOff + insertedLen2 - 1, 1));
        CHECK(curInside3.hasSelection() == false);

        TextCursor curInside4 = curInside3;
        curInside4.moveCharacterLeft();
        CHECK(curInside4.position() == posAdd(positionAt('<'), -insertionOff + insertedLen2 - 2, 1));
        CHECK(curInside4.hasSelection() == false);

        TextCursor curInside1ToInside3 = curInside;
        curInside1ToInside3.setPosition(curInside3.position(), true);

        doc.undo(&cursor1);

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == positionAt('D'));
        CHECK(curDtoC.anchor() == positionAt('D'));
        CHECK(curDtoC.position() == positionAt('C'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == positionAt('<'));
        CHECK(curD.position() == positionAt('D'));
        CHECK(curE.position() == positionAt('E'));
        CHECK(curF.position() == positionAt('F'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == positionAt('<'));
        CHECK(curInside2.position() == positionAt('<'));
        CHECK(curInside3.position() == positionAt('<'));
        CHECK(curInside4.position() == positionAt('<'));
        CHECK(curInside1ToInside3.position() == positionAt('<'));
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);

        doc.redo(&cursor1);

        CHECK(curA.position() == posA2);
        CHECK(curB.position() == posB2);
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == posE2);
        CHECK(curC.position() == posC2);
        CHECK(curBefore.position() == posBefore2);
        CHECK(curAfter.position() == posAfter2);
        CHECK(curD.position() == posD2);
        CHECK(curE.position() == posE2);
        CHECK(curF.position() == posF2);

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == posC2);
        CHECK(curCtoD.position() == posD2);
        CHECK(curDtoC.anchor() == posD2);
        CHECK(curDtoC.position() == posC2);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside1ToInside3.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);
    }


    SECTION("Insert midline three lines") {
        lines = QStringList{
            "A            B",
            "C >< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        TextCursor curA = cursor1;
        curA.setPosition(positionAt('A'));

        TextCursor curB = cursor1;
        curB.setPosition(positionAt('B'));

        TextCursor curBtoE = cursor1;
        curBtoE.setPosition(positionAt('B'));
        curBtoE.setPosition(positionAt('E'), true);

        TextCursor curEtoB = cursor1;
        curEtoB.setPosition(positionAt('E'));
        curEtoB.setPosition(positionAt('B'), true);

        TextCursor curC = cursor1;
        curC.setPosition(positionAt('C'));

        TextCursor curCtoD = cursor1;
        curCtoD.setPosition(positionAt('C'));
        curCtoD.setPosition(positionAt('D'), true);

        TextCursor curBefore = cursor1;
        curBefore.setPosition(positionAt('>'));

        TextCursor curAfter = cursor1;
        curAfter.setPosition(positionAt('<'));

        TextCursor curD = cursor1;
        curD.setPosition(positionAt('D'));

        TextCursor curE = cursor1;
        curE.setPosition(positionAt('E'));

        TextCursor curF = cursor1;
        curF.setPosition(positionAt('F'));

        cursor1.setPosition(positionAt('<'));
        const int insertionOff = cursor1.position().codeUnit;
        const QString insertedTextLine1 = "XXX";
        const QString insertedTextLine2 = "XXXXXX";
        const QString insertedTextLine3 = "XXXXXXXX";
        const QString insertedText = insertedTextLine1 + "\n" + insertedTextLine2 + "\n" + insertedTextLine3;
        const int insertedLen3 = insertedTextLine3.size();
        cursor1.insertText(insertedText);



        const auto posA2 = curA.position();
        const auto posB2 = curB.position();
        const auto posC2 = curC.position();
        const auto posBefore2 = curBefore.position();
        const auto posAfter2 = curAfter.position();
        const auto posD2 = curD.position();
        const auto posE2 = curE.position();
        const auto posF2 = curF.position();

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == posAdd(positionAt('E'), 0, 2));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == posAdd(positionAt('D'), -insertionOff + insertedLen3, 2));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == posAdd(positionAt('<'), -insertionOff + insertedLen3, 2));
        CHECK(curD.position() == posAdd(positionAt('D'), -insertionOff + insertedLen3, 2));
        CHECK(curE.position() == posAdd(positionAt('E'), 0, 2));
        CHECK(curEtoB.anchor() == posAdd(positionAt('E'), 0, 2));
        CHECK(curEtoB.position() == positionAt('B'));
        CHECK(curF.position() == posAdd(positionAt('F'), 0, 2));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        TextCursor curInside = curBefore;
        curInside.moveCharacterRight();
        CHECK(curInside.position() == posAdd(positionAt('>'), 1, 0));
        CHECK(curInside.hasSelection() == false);

        TextCursor curInside2 = curInside;
        curInside2.moveCharacterRight();
        CHECK(curInside2.position() == posAdd(positionAt('>'), 2, 0));
        CHECK(curInside2.hasSelection() == false);

        TextCursor curInside3 = curAfter;
        curInside3.moveCharacterLeft();
        CHECK(curInside3.position() == posAdd(positionAt('<'), -insertionOff + insertedLen3 - 1, 2));
        CHECK(curInside3.hasSelection() == false);

        TextCursor curInside4 = curInside3;
        curInside4.moveCharacterLeft();
        CHECK(curInside4.position() == posAdd(positionAt('<'), -insertionOff + insertedLen3 - 2, 2));
        CHECK(curInside4.hasSelection() == false);

        TextCursor curInside1ToInside3 = curInside;
        curInside1ToInside3.setPosition(curInside3.position(), true);

        doc.undo(&cursor1);

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == positionAt('D'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == positionAt('<'));
        CHECK(curD.position() == positionAt('D'));
        CHECK(curE.position() == positionAt('E'));
        CHECK(curF.position() == positionAt('F'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == positionAt('<'));
        CHECK(curInside2.position() == positionAt('<'));
        CHECK(curInside3.position() == positionAt('<'));
        CHECK(curInside4.position() == positionAt('<'));
        CHECK(curInside1ToInside3.position() == positionAt('<'));
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);

        doc.redo(&cursor1);

        CHECK(curA.position() == posA2);
        CHECK(curB.position() == posB2);
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == posE2);
        CHECK(curC.position() == posC2);
        CHECK(curBefore.position() == posBefore2);
        CHECK(curAfter.position() == posAfter2);
        CHECK(curD.position() == posD2);
        CHECK(curE.position() == posE2);
        CHECK(curF.position() == posF2);

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == posC2);
        CHECK(curCtoD.position() == posD2);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside1ToInside3.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);
    }

    SECTION("Remove midline") {
        lines = QStringList{
            "A            B",
            "C >XXXXXXXXXXXXXXXX< D",
            "E            F"
        };

        const int removedChars = lines[1].count("X");

        cursor1.insertText(lines.join("\n"));

        TextCursor curA = cursor1;
        curA.setPosition(positionAt('A'));

        TextCursor curB = cursor1;
        curB.setPosition(positionAt('B'));

        TextCursor curBtoE = cursor1;
        curBtoE.setPosition(positionAt('B'));
        curBtoE.setPosition(positionAt('E'), true);

        TextCursor curEtoB = cursor1;
        curEtoB.setPosition(positionAt('E'));
        curEtoB.setPosition(positionAt('B'), true);

        TextCursor curC = cursor1;
        curC.setPosition(positionAt('C'));

        TextCursor curCtoD = cursor1;
        curCtoD.setPosition(positionAt('C'));
        curCtoD.setPosition(positionAt('D'), true);

        TextCursor curDtoC = cursor1;
        curDtoC.setPosition(positionAt('D'));
        curDtoC.setPosition(positionAt('C'), true);

        TextCursor curBefore = cursor1;
        curBefore.setPosition(positionAt('>'));

        TextCursor curAfter = cursor1;
        curAfter.setPosition(positionAt('<'));

        TextCursor curD = cursor1;
        curD.setPosition(positionAt('D'));

        TextCursor curE = cursor1;
        curE.setPosition(positionAt('E'));

        TextCursor curF = cursor1;
        curF.setPosition(positionAt('F'));

        TextCursor curInside = curBefore;
        curInside.moveCharacterRight();
        CHECK(curInside.position() == posAdd(positionAt('>'), 1, 0));
        CHECK(curInside.hasSelection() == false);

        TextCursor curInside2 = curInside;
        curInside2.moveCharacterRight();
        CHECK(curInside2.position() == posAdd(positionAt('>'), 2, 0));
        CHECK(curInside2.hasSelection() == false);

        TextCursor curInside3 = curAfter;
        curInside3.moveCharacterLeft();
        CHECK(curInside3.position() == posAdd(positionAt('<'), -1, 0));
        CHECK(curInside3.hasSelection() == false);

        TextCursor curInside4 = curInside3;
        curInside4.moveCharacterLeft();
        CHECK(curInside4.position() == posAdd(positionAt('<'), -2, 0));
        CHECK(curInside4.hasSelection() == false);

        TextCursor curInside1ToInside3 = curInside;
        curInside1ToInside3.setPosition(curInside3.position(), true);

        cursor1.setPosition(positionAt('>'));
        cursor1.moveCharacterRight();
        cursor1.setPosition(positionAt('<'), true);
        cursor1.removeSelectedText();

        const auto posA2 = curA.position();
        const auto posB2 = curB.position();
        const auto posC2 = curC.position();
        const auto posBefore2 = curBefore.position();
        const auto posAfter2 = curAfter.position();
        const auto posD2 = curD.position();
        const auto posE2 = curE.position();
        const auto posF2 = curF.position();

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == posAdd(positionAt('D'), -removedChars, 0));
        CHECK(curDtoC.anchor() == posAdd(positionAt('D'), -removedChars, 0));
        CHECK(curDtoC.position() == positionAt('C'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == posAdd(positionAt('<'), -removedChars, 0));
        CHECK(curD.position() == posAdd(positionAt('D'), -removedChars, 0));
        CHECK(curE.position() == positionAt('E'));
        CHECK(curEtoB.anchor() == positionAt('E'));
        CHECK(curEtoB.position() == positionAt('B'));
        CHECK(curF.position() == positionAt('F'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);

        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside2.hasSelection() == false);

        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside3.hasSelection() == false);

        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside4.hasSelection() == false);

        doc.undo(&cursor1);

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == positionAt('<'));
        CHECK(curD.position() == positionAt('D'));
        CHECK(curE.position() == positionAt('E'));
        CHECK(curEtoB.anchor() == positionAt('E'));
        CHECK(curEtoB.position() == positionAt('B'));
        CHECK(curF.position() == positionAt('F'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == positionAt('D'));
        CHECK(curDtoC.anchor() == positionAt('D'));
        CHECK(curDtoC.position() == positionAt('C'));
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == positionAt('<'));
        CHECK(curInside2.position() == positionAt('<'));
        CHECK(curInside3.position() == positionAt('<'));
        CHECK(curInside4.position() == positionAt('<'));
        CHECK(curInside1ToInside3.position() == positionAt('<'));
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);


        doc.redo(&cursor1);

        CHECK(curA.position() == posA2);
        CHECK(curB.position() == posB2);
        CHECK(curBtoE.anchor() == posB2);
        CHECK(curBtoE.position() == posE2);
        CHECK(curC.position() == posC2);
        CHECK(curBefore.position() == posBefore2);
        CHECK(curAfter.position() == posAfter2);
        CHECK(curD.position() == posD2);
        CHECK(curE.position() == posE2);
        CHECK(curEtoB.anchor() == posE2);
        CHECK(curEtoB.position() == posB2);
        CHECK(curF.position() == posF2);

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == posC2);
        CHECK(curCtoD.position() == posD2);
        CHECK(curDtoC.anchor() == posD2);
        CHECK(curDtoC.position() == posC2);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside1ToInside3.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);

    }

    SECTION("Remove midline two lines") {
        lines = QStringList{
            "A            B",
            "C >XXXXXXXXXXXXXXXX",
            "XXXXXX< D",
            "E            F"
        };

        const int removedChars2 = lines[2].count("X");

        cursor1.insertText(lines.join("\n"));

        TextCursor curA = cursor1;
        curA.setPosition(positionAt('A'));

        TextCursor curB = cursor1;
        curB.setPosition(positionAt('B'));

        TextCursor curBtoE = cursor1;
        curBtoE.setPosition(positionAt('B'));
        curBtoE.setPosition(positionAt('E'), true);

        TextCursor curEtoB = cursor1;
        curEtoB.setPosition(positionAt('E'));
        curEtoB.setPosition(positionAt('B'), true);

        TextCursor curC = cursor1;
        curC.setPosition(positionAt('C'));

        TextCursor curCtoD = cursor1;
        curCtoD.setPosition(positionAt('C'));
        curCtoD.setPosition(positionAt('D'), true);

        TextCursor curBefore = cursor1;
        curBefore.setPosition(positionAt('>'));

        TextCursor curAfter = cursor1;
        curAfter.setPosition(positionAt('<'));

        TextCursor curD = cursor1;
        curD.setPosition(positionAt('D'));

        TextCursor curE = cursor1;
        curE.setPosition(positionAt('E'));

        TextCursor curF = cursor1;
        curF.setPosition(positionAt('F'));

        TextCursor curInside = curBefore;
        curInside.moveCharacterRight();
        CHECK(curInside.position() == posAdd(positionAt('>'), 1, 0));
        CHECK(curInside.hasSelection() == false);

        TextCursor curInside2 = curInside;
        curInside2.moveCharacterRight();
        CHECK(curInside2.position() == posAdd(positionAt('>'), 2, 0));
        CHECK(curInside2.hasSelection() == false);

        TextCursor curInside3 = curAfter;
        curInside3.moveCharacterLeft();
        CHECK(curInside3.position() == posAdd(positionAt('<'), -1, 0));
        CHECK(curInside3.hasSelection() == false);

        TextCursor curInside4 = curInside3;
        curInside4.moveCharacterLeft();
        CHECK(curInside4.position() == posAdd(positionAt('<'), -2, 0));
        CHECK(curInside4.hasSelection() == false);

        TextCursor curInside1ToInside3 = curInside;
        curInside1ToInside3.setPosition(curInside3.position(), true);

        cursor1.setPosition(positionAt('>'));
        cursor1.moveCharacterRight();
        const int deletionStartOff = cursor1.position().codeUnit;
        cursor1.setPosition(positionAt('<'), true);
        cursor1.removeSelectedText();

        const auto posA2 = curA.position();
        const auto posB2 = curB.position();
        const auto posC2 = curC.position();
        const auto posBefore2 = curBefore.position();
        const auto posAfter2 = curAfter.position();
        const auto posD2 = curD.position();
        const auto posE2 = curE.position();
        const auto posF2 = curF.position();

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == posAdd(positionAt('E'), 0, -1));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == posAdd(positionAt('D'), -removedChars2 + deletionStartOff, -1));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == posAdd(positionAt('<'), -removedChars2 + deletionStartOff, -1));
        CHECK(curD.position() == posAdd(positionAt('D'), -removedChars2 + deletionStartOff, -1));
        CHECK(curE.position() == posAdd(positionAt('E'), 0, -1));
        CHECK(curEtoB.anchor() == posAdd(positionAt('E'), 0, -1));
        CHECK(curEtoB.position() == positionAt('B'));
        CHECK(curF.position() == posAdd(positionAt('F'), 0, -1));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);

        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside2.hasSelection() == false);

        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside3.hasSelection() == false);

        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside4.hasSelection() == false);

        doc.undo(&cursor1);

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == positionAt('<'));
        CHECK(curD.position() == positionAt('D'));
        CHECK(curE.position() == positionAt('E'));
        CHECK(curEtoB.anchor() == positionAt('E'));
        CHECK(curEtoB.position() == positionAt('B'));
        CHECK(curF.position() == positionAt('F'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == positionAt('D'));
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == positionAt('<'));
        CHECK(curInside2.position() == positionAt('<'));
        CHECK(curInside3.position() == positionAt('<'));
        CHECK(curInside4.position() == positionAt('<'));
        CHECK(curInside1ToInside3.position() == positionAt('<'));
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);

        doc.redo(&cursor1);

        CHECK(curA.position() == posA2);
        CHECK(curB.position() == posB2);
        CHECK(curBtoE.anchor() == posB2);
        CHECK(curBtoE.position() == posE2);
        CHECK(curC.position() == posC2);
        CHECK(curBefore.position() == posBefore2);
        CHECK(curAfter.position() == posAfter2);
        CHECK(curD.position() == posD2);
        CHECK(curE.position() == posE2);
        CHECK(curEtoB.anchor() == posE2);
        CHECK(curEtoB.position() == posB2);
        CHECK(curF.position() == posF2);

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == posC2);
        CHECK(curCtoD.position() == posD2);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside1ToInside3.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);
    }

    SECTION("Remove midline two lines at end of document") {
        lines = QStringList{
            "A            B",
            "C >XXXXXXXXXXXXXXXX",
            "XXXXXX< D"
        };

        const int removedChars2 = lines[2].count("X");

        cursor1.insertText(lines.join("\n"));

        TextCursor curA = cursor1;
        curA.setPosition(positionAt('A'));

        TextCursor curB = cursor1;
        curB.setPosition(positionAt('B'));

        TextCursor curBtoD = cursor1;
        curBtoD.setPosition(positionAt('B'));
        curBtoD.setPosition(positionAt('D'), true);

        TextCursor curDtoB = cursor1;
        curDtoB.setPosition(positionAt('D'));
        curDtoB.setPosition(positionAt('B'), true);

        TextCursor curC = cursor1;
        curC.setPosition(positionAt('C'));

        TextCursor curCtoD = cursor1;
        curCtoD.setPosition(positionAt('C'));
        curCtoD.setPosition(positionAt('D'), true);

        TextCursor curBefore = cursor1;
        curBefore.setPosition(positionAt('>'));

        TextCursor curAfter = cursor1;
        curAfter.setPosition(positionAt('<'));

        TextCursor curD = cursor1;
        curD.setPosition(positionAt('D'));

        TextCursor curInside = curBefore;
        curInside.moveCharacterRight();
        CHECK(curInside.position() == posAdd(positionAt('>'), 1, 0));
        CHECK(curInside.hasSelection() == false);

        TextCursor curInside2 = curInside;
        curInside2.moveCharacterRight();
        CHECK(curInside2.position() == posAdd(positionAt('>'), 2, 0));
        CHECK(curInside2.hasSelection() == false);

        TextCursor curInside3 = curAfter;
        curInside3.moveCharacterLeft();
        CHECK(curInside3.position() == posAdd(positionAt('<'), -1, 0));
        CHECK(curInside3.hasSelection() == false);

        TextCursor curInside4 = curInside3;
        curInside4.moveCharacterLeft();
        CHECK(curInside4.position() == posAdd(positionAt('<'), -2, 0));
        CHECK(curInside4.hasSelection() == false);

        TextCursor curInside1ToInside3 = curInside;
        curInside1ToInside3.setPosition(curInside3.position(), true);

        cursor1.setPosition(positionAt('>'));
        cursor1.moveCharacterRight();
        const int deletionStartOff = cursor1.position().codeUnit;
        cursor1.setPosition(positionAt('<'), true);
        cursor1.removeSelectedText();

        const auto posA2 = curA.position();
        const auto posB2 = curB.position();
        const auto posC2 = curC.position();
        const auto posBefore2 = curBefore.position();
        const auto posAfter2 = curAfter.position();
        const auto posD2 = curD.position();

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoD.anchor() == positionAt('B'));
        CHECK(curBtoD.position() == posAdd(positionAt('D'), -removedChars2 + deletionStartOff, -1));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == posAdd(positionAt('D'), -removedChars2 + deletionStartOff, -1));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == posAdd(positionAt('<'), -removedChars2 + deletionStartOff, -1));
        CHECK(curD.position() == posAdd(positionAt('D'), -removedChars2 + deletionStartOff, -1));
        CHECK(curDtoB.anchor() == posAdd(positionAt('D'), -removedChars2 + deletionStartOff, -1));
        CHECK(curDtoB.position() == positionAt('B'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);

        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside2.hasSelection() == false);

        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside3.hasSelection() == false);

        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside4.hasSelection() == false);

        doc.undo(&cursor1);

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoD.anchor() == positionAt('B'));
        CHECK(curBtoD.position() == positionAt('D'));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == positionAt('<'));
        CHECK(curD.position() == positionAt('D'));
        CHECK(curDtoB.anchor() == positionAt('D'));
        CHECK(curDtoB.position() == positionAt('B'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == positionAt('D'));
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);

        CHECK(curInside.position() == positionAt('<'));
        CHECK(curInside2.position() == positionAt('<'));
        CHECK(curInside3.position() == positionAt('<'));
        CHECK(curInside4.position() == positionAt('<'));
        CHECK(curInside1ToInside3.position() == positionAt('<'));
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);

        doc.redo(&cursor1);

        CHECK(curA.position() == posA2);
        CHECK(curB.position() == posB2);
        CHECK(curBtoD.anchor() == posB2);
        CHECK(curBtoD.position() == posD2);
        CHECK(curC.position() == posC2);
        CHECK(curBefore.position() == posBefore2);
        CHECK(curAfter.position() == posAfter2);
        CHECK(curD.position() == posD2);
        CHECK(curDtoB.anchor() == posD2);
        CHECK(curDtoB.position() == posB2);

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == posC2);
        CHECK(curCtoD.position() == posD2);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside1ToInside3.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);
    }

    SECTION("Remove midline three lines") {
        lines = QStringList{
            "A            B",
            "C >XXXXXXXXXXXXXXXX",
            "XXXXXXXXXXXXX",
            "XXXXXX< D",
            "E            F"
        };

        const int removedChars3 = lines[3].count("X");

        cursor1.insertText(lines.join("\n"));

        TextCursor curA = cursor1;
        curA.setPosition(positionAt('A'));

        TextCursor curB = cursor1;
        curB.setPosition(positionAt('B'));

        TextCursor curBtoE = cursor1;
        curBtoE.setPosition(positionAt('B'));
        curBtoE.setPosition(positionAt('E'), true);

        TextCursor curEtoB = cursor1;
        curEtoB.setPosition(positionAt('E'));
        curEtoB.setPosition(positionAt('B'), true);

        TextCursor curC = cursor1;
        curC.setPosition(positionAt('C'));

        TextCursor curCtoD = cursor1;
        curCtoD.setPosition(positionAt('C'));
        curCtoD.setPosition(positionAt('D'), true);

        TextCursor curBefore = cursor1;
        curBefore.setPosition(positionAt('>'));

        TextCursor curAfter = cursor1;
        curAfter.setPosition(positionAt('<'));

        TextCursor curD = cursor1;
        curD.setPosition(positionAt('D'));

        TextCursor curE = cursor1;
        curE.setPosition(positionAt('E'));

        TextCursor curF = cursor1;
        curF.setPosition(positionAt('F'));

        TextCursor curInside = curBefore;
        curInside.moveCharacterRight();
        CHECK(curInside.position() == posAdd(positionAt('>'), 1, 0));
        CHECK(curInside.hasSelection() == false);

        TextCursor curInside2 = curInside;
        curInside2.moveCharacterRight();
        CHECK(curInside2.position() == posAdd(positionAt('>'), 2, 0));
        CHECK(curInside2.hasSelection() == false);

        TextCursor curInside3 = curAfter;
        curInside3.moveCharacterLeft();
        CHECK(curInside3.position() == posAdd(positionAt('<'), -1, 0));
        CHECK(curInside3.hasSelection() == false);

        TextCursor curInside4 = curInside3;
        curInside4.moveCharacterLeft();
        CHECK(curInside4.position() == posAdd(positionAt('<'), -2, 0));
        CHECK(curInside4.hasSelection() == false);

        TextCursor curInside1ToInside3 = curInside;
        curInside1ToInside3.setPosition(curInside3.position(), true);

        TextCursor curInside5 = curInside3;
        curInside5.setPosition({4, 2});

        cursor1.setPosition(positionAt('>'));
        cursor1.moveCharacterRight();
        const int deletionStartOff = cursor1.position().codeUnit;
        cursor1.setPosition(positionAt('<'), true);
        cursor1.removeSelectedText();

        const auto posA2 = curA.position();
        const auto posB2 = curB.position();
        const auto posC2 = curC.position();
        const auto posBefore2 = curBefore.position();
        const auto posAfter2 = curAfter.position();
        const auto posD2 = curD.position();
        const auto posE2 = curE.position();
        const auto posF2 = curF.position();

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == posAdd(positionAt('E'), 0, -2));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == posAdd(positionAt('D'), -removedChars3 + deletionStartOff, -2));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == posAdd(positionAt('<'), -removedChars3 + deletionStartOff, -2));
        CHECK(curD.position() == posAdd(positionAt('D'), -removedChars3 + deletionStartOff, -2));
        CHECK(curE.position() == posAdd(positionAt('E'), 0, -2));
        CHECK(curEtoB.anchor() == posAdd(positionAt('E'), 0, -2));
        CHECK(curEtoB.position() == positionAt('B'));
        CHECK(curF.position() == posAdd(positionAt('F'), 0, -2));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);

        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside2.hasSelection() == false);

        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside3.hasSelection() == false);

        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside4.hasSelection() == false);

        CHECK(curInside5.position() == posAfter2);
        CHECK(curInside5.hasSelection() == false);

        doc.undo(&cursor1);

        CHECK(curA.position() == positionAt('A'));
        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curC.position() == positionAt('C'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == positionAt('<'));
        CHECK(curD.position() == positionAt('D'));
        CHECK(curE.position() == positionAt('E'));
        CHECK(curEtoB.anchor() == positionAt('E'));
        CHECK(curEtoB.position() == positionAt('B'));
        CHECK(curF.position() == positionAt('F'));

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == positionAt('C'));
        CHECK(curCtoD.position() == positionAt('D'));
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == positionAt('<'));
        CHECK(curInside2.position() == positionAt('<'));
        CHECK(curInside3.position() == positionAt('<'));
        CHECK(curInside4.position() == positionAt('<'));
        CHECK(curInside1ToInside3.position() == positionAt('<'));
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);

        // inside the line that the redo will delete
        TextCursor curInside6 = curInside3;
        curInside6.setPosition({4, 2});

        doc.redo(&cursor1);

        CHECK(curA.position() == posA2);
        CHECK(curB.position() == posB2);
        CHECK(curBtoE.anchor() == posB2);
        CHECK(curBtoE.position() == posE2);
        CHECK(curC.position() == posC2);
        CHECK(curBefore.position() == posBefore2);
        CHECK(curAfter.position() == posAfter2);
        CHECK(curD.position() == posD2);
        CHECK(curE.position() == posE2);
        CHECK(curEtoB.anchor() == posE2);
        CHECK(curEtoB.position() == posB2);
        CHECK(curF.position() == posF2);

        CHECK(curA.hasSelection() == false);
        CHECK(curB.hasSelection() == false);
        CHECK(curC.hasSelection() == false);
        CHECK(curCtoD.anchor() == posC2);
        CHECK(curCtoD.position() == posD2);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);
        CHECK(curF.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside2.position() == posAfter2);
        CHECK(curInside3.position() == posAfter2);
        CHECK(curInside4.position() == posAfter2);
        CHECK(curInside5.position() == posAfter2);
        CHECK(curInside6.position() == posAfter2);
        CHECK(curInside1ToInside3.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);
        CHECK(curInside2.hasSelection() == false);
        CHECK(curInside3.hasSelection() == false);
        CHECK(curInside4.hasSelection() == false);
        CHECK(curInside5.hasSelection() == false);
        CHECK(curInside6.hasSelection() == false);
        CHECK(curInside1ToInside3.hasSelection() == false);
    }

    SECTION("sort lines") {
        lines = QStringList{
            "dummy line 1",
            "dummy line 2",
            "ghello world",
            "asome test",
            "bcompletly different",
            "hthe little fox",
            "dummy line 3",
            "dummy line 4",
            "dummy line 5"
        };

        // sorted in lines 2-5 this is:
        // "dummy line 1",
        // "dummy line 2",
        // "asome test",
        // "bcompletly different",
        // "ghello world",
        // "hthe little fox",
        // "dummy line 3",
        // "dummy line 4",
        // "dummy line 5"

        cursor1.insertText(lines.join("\n"));

        TextCursor cur0 = cursor1;
        cur0.setPosition({5, 0});
        TextCursor cur1 = cursor1;
        cur1.setPosition({10, 1});
        TextCursor cur2 = cursor1;
        cur2.setPosition({9, 2});
        TextCursor cur3 = cursor1;
        cur3.setPosition({3, 3});
        TextCursor cur4 = cursor1;
        cur4.setPosition({7, 4});
        TextCursor cur5 = cursor1;
        cur5.setPosition({8, 5});
        TextCursor cur6 = cursor1;
        cur6.setPosition({4, 6});
        TextCursor cur7 = cursor1;
        cur7.setPosition({2, 7});
        TextCursor cur8 = cursor1;
        cur8.setPosition({1, 8});

        doc.sortLines(2, 5, &cursor1);

        const auto pos0After = cur0.position();
        const auto pos1After = cur1.position();
        const auto pos2After = cur2.position();
        const auto pos3After = cur3.position();
        const auto pos4After = cur4.position();
        const auto pos5After = cur5.position();
        const auto pos6After = cur6.position();
        const auto pos7After = cur7.position();
        const auto pos8After = cur8.position();

        CHECK(cur0.position() == TextCursor::Position{5, 0});
        CHECK(cur1.position() == TextCursor::Position{10, 1});

        CHECK(cur2.position() == TextCursor::Position{9, 4});
        CHECK(cur3.position() == TextCursor::Position{3, 2});
        CHECK(cur4.position() == TextCursor::Position{7, 3});
        CHECK(cur5.position() == TextCursor::Position{8, 5});

        CHECK(cur6.position() == TextCursor::Position{4, 6});
        CHECK(cur7.position() == TextCursor::Position{2, 7});
        CHECK(cur8.position() == TextCursor::Position{1, 8});

        doc.undo(&cursor1);

        CHECK(cur0.position() == TextCursor::Position{5, 0});
        CHECK(cur1.position() == TextCursor::Position{10, 1});
        CHECK(cur2.position() == TextCursor::Position{9, 2});
        CHECK(cur3.position() == TextCursor::Position{3, 3});
        CHECK(cur4.position() == TextCursor::Position{7, 4});
        CHECK(cur5.position() == TextCursor::Position{8, 5});
        CHECK(cur6.position() == TextCursor::Position{4, 6});
        CHECK(cur7.position() == TextCursor::Position{2, 7});
        CHECK(cur8.position() == TextCursor::Position{1, 8});

        doc.redo(&cursor1);

        CHECK(cur0.position() == pos0After);
        CHECK(cur1.position() == pos1After);
        CHECK(cur2.position() == pos2After);
        CHECK(cur3.position() == pos3After);
        CHECK(cur4.position() == pos4After);
        CHECK(cur5.position() == pos5After);
        CHECK(cur6.position() == pos6After);
        CHECK(cur7.position() == pos7After);
        CHECK(cur8.position() == pos8After);
    }

    SECTION("collapsed inserts") {
        lines = QStringList{
            "A            B",
            "C >< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        TextCursor curB = cursor1;
        curB.setPosition(positionAt('B'));

        TextCursor curBtoE = cursor1;
        curBtoE.setPosition(positionAt('B'));
        curBtoE.setPosition(positionAt('E'), true);

        TextCursor curEtoB = cursor1;
        curEtoB.setPosition(positionAt('E'));
        curEtoB.setPosition(positionAt('B'), true);

        TextCursor curBefore = cursor1;
        curBefore.setPosition(positionAt('>'));

        TextCursor curAfter = cursor1;
        curAfter.setPosition(positionAt('<'));

        TextCursor curD = cursor1;
        curD.setPosition(positionAt('D'));

        TextCursor curE = cursor1;
        curE.setPosition(positionAt('E'));

        cursor1.setPosition(positionAt('<'));
        const int insertionOff = cursor1.position().codeUnit;
        cursor1.insertText("\n");
        cursor1.insertText("a");
        cursor1.insertText("b");
        cursor1.insertText("c");
        const int insertedLen2 = 3;

        const auto posB2 = curB.position();
        const auto posBefore2 = curBefore.position();
        const auto posAfter2 = curAfter.position();
        const auto posD2 = curD.position();
        const auto posE2 = curE.position();

        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == posAdd(positionAt('E'), 0, 1));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == posAdd(positionAt('<'), -insertionOff + insertedLen2, 1));
        CHECK(curD.position() == posAdd(positionAt('D'), -insertionOff + insertedLen2, 1));
        CHECK(curE.position() == posAdd(positionAt('E'), 0, 1));

        CHECK(curB.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);

        TextCursor curInside = curBefore;
        curInside.moveCharacterRight();
        CHECK(curInside.position() == posAdd(positionAt('>'), 1, 0));
        CHECK(curInside.hasSelection() == false);

        doc.undo(&cursor1);

        CHECK(curB.position() == positionAt('B'));
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == positionAt('E'));
        CHECK(curBefore.position() == positionAt('>'));
        CHECK(curAfter.position() == positionAt('<'));
        CHECK(curD.position() == positionAt('D'));
        CHECK(curE.position() == positionAt('E'));

        CHECK(curB.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);

        CHECK(curInside.position() == positionAt('<'));
        CHECK(curInside.hasSelection() == false);

        doc.redo(&cursor1);

        CHECK(curB.position() == posB2);
        CHECK(curBtoE.anchor() == positionAt('B'));
        CHECK(curBtoE.position() == posE2);
        CHECK(curBefore.position() == posBefore2);
        CHECK(curAfter.position() == posAfter2);
        CHECK(curD.position() == posD2);
        CHECK(curE.position() == posE2);

        CHECK(curB.hasSelection() == false);
        CHECK(curBefore.hasSelection() == false);
        CHECK(curAfter.hasSelection() == false);
        CHECK(curD.hasSelection() == false);
        CHECK(curE.hasSelection() == false);

        CHECK(curInside.position() == posAfter2);
        CHECK(curInside.hasSelection() == false);
    }

    SECTION("move line down") {
        lines = QStringList{
            "line 1aaaaaa",
            "line 2b",
            "line 3ccc",
            "line 4dddddd",
            "line 5eeeee",
            "line 6ffffffff",
            "line 7gggg",
            "line 8",
            "line 9jjjjjjj"
        };

        cursor1.insertText(lines.join("\n"));

        TextCursor cur0 = cursor1;
        cur0.setPosition({9, 0});
        TextCursor cur1 = cursor1;
        cur1.setPosition({0, 1});
        TextCursor cur2 = cursor1;
        cur2.setPosition({9, 2});
        TextCursor cur3 = cursor1;
        cur3.setPosition({3, 3});
        TextCursor cur4 = cursor1;
        cur4.setPosition({7, 4});
        TextCursor cur5 = cursor1;
        cur5.setPosition({8, 5});
        TextCursor cur6 = cursor1;
        cur6.setPosition({4, 6});
        TextCursor cur7 = cursor1;
        cur7.setPosition({2, 7});
        TextCursor cur8 = cursor1;
        cur8.setPosition({1, 8});

        doc.tmp_moveLine(2, 5, &cursor1);

        const auto pos0After = cur0.position();
        const auto pos1After = cur1.position();
        const auto pos2After = cur2.position();
        const auto pos3After = cur3.position();
        const auto pos4After = cur4.position();
        const auto pos5After = cur5.position();
        const auto pos6After = cur6.position();
        const auto pos7After = cur7.position();
        const auto pos8After = cur8.position();

        CHECK(cur0.position() == TextCursor::Position{9, 0});
        CHECK(cur1.position() == TextCursor::Position{0, 1});
        CHECK(cur2.position() == TextCursor::Position{9, 4});
        CHECK(cur3.position() == TextCursor::Position{3, 2});
        CHECK(cur4.position() == TextCursor::Position{7, 3});
        CHECK(cur5.position() == TextCursor::Position{8, 5});
        CHECK(cur6.position() == TextCursor::Position{4, 6});
        CHECK(cur7.position() == TextCursor::Position{2, 7});
        CHECK(cur8.position() == TextCursor::Position{1, 8});

        doc.undo(&cursor1);

        CHECK(cur0.position() == TextCursor::Position{9, 0});
        CHECK(cur1.position() == TextCursor::Position{0, 1});
        CHECK(cur2.position() == TextCursor::Position{9, 2});
        CHECK(cur3.position() == TextCursor::Position{3, 3});
        CHECK(cur4.position() == TextCursor::Position{7, 4});
        CHECK(cur5.position() == TextCursor::Position{8, 5});
        CHECK(cur6.position() == TextCursor::Position{4, 6});
        CHECK(cur7.position() == TextCursor::Position{2, 7});
        CHECK(cur8.position() == TextCursor::Position{1, 8});

        doc.redo(&cursor1);

        CHECK(cur0.position() == pos0After);
        CHECK(cur1.position() == pos1After);
        CHECK(cur2.position() == pos2After);
        CHECK(cur3.position() == pos3After);
        CHECK(cur4.position() == pos4After);
        CHECK(cur5.position() == pos5After);
        CHECK(cur6.position() == pos6After);
        CHECK(cur7.position() == pos7After);
        CHECK(cur8.position() == pos8After);
    }


    SECTION("move line up") {
        lines = QStringList{
            "line 1aaaaaa",
            "line 2b",
            "line 3ccc",
            "line 4dddddd",
            "line 5eeeee",
            "line 6ffffffff",
            "line 7gggg",
            "line 8",
            "line 9jjjjjjj"
        };

        cursor1.insertText(lines.join("\n"));

        TextCursor cur0 = cursor1;
        cur0.setPosition({9, 0});
        TextCursor cur1 = cursor1;
        cur1.setPosition({0, 1});
        TextCursor cur2 = cursor1;
        cur2.setPosition({9, 2});
        TextCursor cur3 = cursor1;
        cur3.setPosition({3, 3});
        TextCursor cur4 = cursor1;
        cur4.setPosition({7, 4});
        TextCursor cur5 = cursor1;
        cur5.setPosition({8, 5});
        TextCursor cur6 = cursor1;
        cur6.setPosition({4, 6});
        TextCursor cur7 = cursor1;
        cur7.setPosition({2, 7});
        TextCursor cur8 = cursor1;
        cur8.setPosition({1, 8});

        doc.tmp_moveLine(7, 4, &cursor1);

        const auto pos0After = cur0.position();
        const auto pos1After = cur1.position();
        const auto pos2After = cur2.position();
        const auto pos3After = cur3.position();
        const auto pos4After = cur4.position();
        const auto pos5After = cur5.position();
        const auto pos6After = cur6.position();
        const auto pos7After = cur7.position();
        const auto pos8After = cur8.position();

        CHECK(cur0.position() == TextCursor::Position{9, 0});
        CHECK(cur1.position() == TextCursor::Position{0, 1});
        CHECK(cur2.position() == TextCursor::Position{9, 2});
        CHECK(cur3.position() == TextCursor::Position{3, 3});
        CHECK(cur7.position() == TextCursor::Position{2, 4});
        CHECK(cur4.position() == TextCursor::Position{7, 5});
        CHECK(cur5.position() == TextCursor::Position{8, 6});
        CHECK(cur6.position() == TextCursor::Position{4, 7});
        CHECK(cur8.position() == TextCursor::Position{1, 8});
        doc.undo(&cursor1);

        CHECK(cur0.position() == TextCursor::Position{9, 0});
        CHECK(cur1.position() == TextCursor::Position{0, 1});
        CHECK(cur2.position() == TextCursor::Position{9, 2});
        CHECK(cur3.position() == TextCursor::Position{3, 3});
        CHECK(cur4.position() == TextCursor::Position{7, 4});
        CHECK(cur5.position() == TextCursor::Position{8, 5});
        CHECK(cur6.position() == TextCursor::Position{4, 6});
        CHECK(cur7.position() == TextCursor::Position{2, 7});
        CHECK(cur8.position() == TextCursor::Position{1, 8});

        doc.redo(&cursor1);

        CHECK(cur0.position() == pos0After);
        CHECK(cur1.position() == pos1After);
        CHECK(cur2.position() == pos2After);
        CHECK(cur3.position() == pos3After);
        CHECK(cur4.position() == pos4After);
        CHECK(cur5.position() == pos5After);
        CHECK(cur6.position() == pos6After);
        CHECK(cur7.position() == pos7After);
        CHECK(cur8.position() == pos8After);
    }
}

TEST_CASE("Document line marker adjustments") {
    // Check if modifiation, undo and redo correctly moves line markers on the document.


    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);
    Document doc;

    TextCursor cursor1{&doc, nullptr, [&terminal, &doc](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    QStringList lines;

    auto positionAt = [&] (QChar ch) -> TextCursor::Position {
        int lineNo = 0;
        for (const QString &line: lines) {
            if (line.contains(ch)) {
                return {line.indexOf(ch), lineNo};
            }
            lineNo += 1;
        }
        // should never be reached
        REQUIRE(false);
        return {0, 0};
    };

    SECTION("Insert midline") {
        lines = QStringList{
            "A            B",
            "C >< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        LineMarker marker0{&doc, 0};
        LineMarker marker1{&doc, 1};
        LineMarker marker2{&doc, 2};

        cursor1.setPosition(positionAt('<'));
        const QString insertedText = "XXXXXXXXXXX";
        cursor1.insertText(insertedText);

        CHECK(marker0.line() == 0);
        CHECK(marker1.line() == 1);
        CHECK(marker2.line() == 2);

        doc.undo(&cursor1);

        CHECK(marker0.line() == 0);
        CHECK(marker1.line() == 1);
        CHECK(marker2.line() == 2);

        doc.redo(&cursor1);

        CHECK(marker0.line() == 0);
        CHECK(marker1.line() == 1);
        CHECK(marker2.line() == 2);
    }

    SECTION("Insert midline two lines") {
        lines = QStringList{
            "A            B",
            "C >< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        LineMarker markerA{&doc, 0};
        LineMarker markerC{&doc, 1};
        LineMarker markerE{&doc, 2};

        cursor1.setPosition(positionAt('<'));
        const QString insertedTextLine1 = "XXX";
        const QString insertedTextLine2 = "XXXXXXXX";
        const QString insertedText = insertedTextLine1 + "\n" + insertedTextLine2;
        cursor1.insertText(insertedText);

        const int posA2 = markerA.line();
        const int posC2 = markerC.line();
        const int posE2 = markerE.line();

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerE.line() == 3);

        LineMarker markerD{&doc, 2};

        doc.undo(&cursor1);

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerD.line() == 1);
        CHECK(markerE.line() == 2);


        doc.redo(&cursor1);

        CHECK(markerA.line() == posA2);
        CHECK(markerC.line() == posC2);
        CHECK(markerD.line() == posC2);
        CHECK(markerE.line() == posE2);
    }

    SECTION("Insert midline three lines") {
        lines = QStringList{
            "A            B",
            "C >< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        LineMarker markerA{&doc, 0};
        LineMarker markerC{&doc, 1};
        LineMarker markerE{&doc, 2};

        cursor1.setPosition(positionAt('<'));
        const QString insertedTextLine1 = "XXX";
        const QString insertedTextLine2 = "XXXXXX";
        const QString insertedTextLine3 = "XXXXXXXX";
        const QString insertedText = insertedTextLine1 + "\n" + insertedTextLine2 + "\n" + insertedTextLine3;
        cursor1.insertText(insertedText);

        const int posA2 = markerA.line();
        const int posC2 = markerC.line();
        const int posE2 = markerE.line();

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerE.line() == 4);

        LineMarker markerX{&doc, 2};
        LineMarker markerD{&doc, 3};

        doc.undo(&cursor1);

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerX.line() == 1);
        CHECK(markerD.line() == 1);
        CHECK(markerE.line() == 2);

        doc.redo(&cursor1);

        CHECK(markerA.line() == posA2);
        CHECK(markerC.line() == posC2);
        CHECK(markerX.line() == posC2);
        CHECK(markerD.line() == posC2);
        CHECK(markerE.line() == posE2);

    }

    SECTION("Remove midline") {
        lines = QStringList{
            "A            B",
            "C >XXXXXXXXXXXXXXXX< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        LineMarker markerA{&doc, 0};
        LineMarker markerC{&doc, 1};
        LineMarker markerE{&doc, 2};

        cursor1.setPosition(positionAt('>'));
        cursor1.moveCharacterRight();
        cursor1.setPosition(positionAt('<'), true);
        cursor1.removeSelectedText();


        cursor1.setPosition(positionAt('<'));
        const QString insertedText = "XXXXXXXXXXX";
        cursor1.insertText(insertedText);

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerE.line() == 2);

        doc.undo(&cursor1);

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerE.line() == 2);

        doc.redo(&cursor1);

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerE.line() == 2);


    }

    SECTION("Remove midline two lines") {
        lines = QStringList{
            "A            B",
            "C >XXXXXXXXXXXXXXXX",
            "XXXXXX< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        LineMarker markerA{&doc, 0};
        LineMarker markerC{&doc, 1};
        LineMarker markerD{&doc, 2};
        LineMarker markerE{&doc, 3};

        cursor1.setPosition(positionAt('>'));
        cursor1.moveCharacterRight();
        cursor1.setPosition(positionAt('<'), true);
        cursor1.removeSelectedText();

        const int posA2 = markerA.line();
        const int posC2 = markerC.line();
        const int posE2 = markerE.line();

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerD.line() == 1);
        CHECK(markerE.line() == 2);

        doc.undo(&cursor1);

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerD.line() == 1);
        CHECK(markerE.line() == 3);

        doc.redo(&cursor1);

        CHECK(markerA.line() == posA2);
        CHECK(markerC.line() == posC2);
        CHECK(markerD.line() == posC2);
        CHECK(markerE.line() == posE2);
    }

    SECTION("Remove midline three lines") {
        lines = QStringList{
            "A            B",
            "C >XXXXXXXXXXXXXXXX",
            "XXXXXXXXXXXXX",
            "XXXXXX< D",
            "E            F"
        };

        cursor1.insertText(lines.join("\n"));

        LineMarker markerA{&doc, 0};
        LineMarker markerC{&doc, 1};
        LineMarker markerX{&doc, 2};
        LineMarker markerD{&doc, 3};
        LineMarker markerE{&doc, 4};

        cursor1.setPosition(positionAt('>'));
        cursor1.moveCharacterRight();
        cursor1.setPosition(positionAt('<'), true);
        cursor1.removeSelectedText();

        const int posA2 = markerA.line();
        const int posC2 = markerC.line();
        const int posE2 = markerE.line();

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerX.line() == 1);
        CHECK(markerD.line() == 1);
        CHECK(markerE.line() == 2);

        doc.undo(&cursor1);

        CHECK(markerA.line() == 0);
        CHECK(markerC.line() == 1);
        CHECK(markerX.line() == 1);
        CHECK(markerD.line() == 1);
        CHECK(markerE.line() == 4);

        LineMarker markerX3{&doc, 3};

        doc.redo(&cursor1);

        CHECK(markerA.line() == posA2);
        CHECK(markerC.line() == posC2);
        CHECK(markerX.line() == posC2);
        CHECK(markerX3.line() == posC2);
        CHECK(markerD.line() == posC2);
        CHECK(markerE.line() == posE2);
    }

    SECTION("sort lines") {
        lines = QStringList{
            "dummy line 1",
            "dummy line 2",
            "ghello world",
            "asome test",
            "bcompletly different",
            "hthe little fox",
            "dummy line 3",
            "dummy line 4",
            "dummy line 5"
        };

        // sorted in lines 2-5 this is:
        // "dummy line 1",
        // "dummy line 2",
        // "asome test",
        // "bcompletly different",
        // "ghello world",
        // "hthe little fox",
        // "dummy line 3",
        // "dummy line 4",
        // "dummy line 5"

        cursor1.insertText(lines.join("\n"));

        LineMarker marker0{&doc, 0};
        LineMarker marker1 = marker0;
        marker1.setLine(1);
        LineMarker marker2 = marker0;
        marker2.setLine(2);
        LineMarker marker3 = marker0;
        marker3.setLine(3);
        LineMarker marker4 = marker0;
        marker4.setLine(4);
        LineMarker marker5 = marker0;
        marker5.setLine(5);
        LineMarker marker6 = marker0;
        marker6.setLine(6);
        LineMarker marker7 = marker0;
        marker7.setLine(7);
        LineMarker marker8 = marker0;
        marker8.setLine(8);

        doc.sortLines(2, 5, &cursor1);

        const int pos0After = marker0.line();
        const int pos1After = marker1.line();
        const int pos2After = marker2.line();
        const int pos3After = marker3.line();
        const int pos4After = marker4.line();
        const int pos5After = marker5.line();
        const int pos6After = marker6.line();
        const int pos7After = marker7.line();
        const int pos8After = marker8.line();

        CHECK(marker0.line() == 0);
        CHECK(marker1.line() == 1);

        CHECK(marker2.line() == 4);
        CHECK(marker3.line() == 2);
        CHECK(marker4.line() == 3);
        CHECK(marker5.line() == 5);

        CHECK(marker6.line() == 6);
        CHECK(marker7.line() == 7);
        CHECK(marker8.line() == 8);

        doc.undo(&cursor1);

        CHECK(marker0.line() == 0);
        CHECK(marker1.line() == 1);
        CHECK(marker2.line() == 2);
        CHECK(marker3.line() == 3);
        CHECK(marker4.line() == 4);
        CHECK(marker5.line() == 5);
        CHECK(marker6.line() == 6);
        CHECK(marker7.line() == 7);
        CHECK(marker8.line() == 8);

        doc.redo(&cursor1);

        CHECK(marker0.line() == pos0After);
        CHECK(marker1.line() == pos1After);
        CHECK(marker2.line() == pos2After);
        CHECK(marker3.line() == pos3After);
        CHECK(marker4.line() == pos4After);
        CHECK(marker5.line() == pos5After);
        CHECK(marker6.line() == pos6After);
        CHECK(marker7.line() == pos7After);
        CHECK(marker8.line() == pos8After);
    }

    SECTION("move line down") {
        lines = QStringList{
            "line 1aaaaaa",
            "line 2b",
            "line 3ccc",
            "line 4dddddd",
            "line 5eeeee",
            "line 6ffffffff",
            "line 7gggg",
            "line 8",
            "line 9jjjjjjj"
        };

        cursor1.insertText(lines.join("\n"));

        LineMarker marker0{&doc, 0};
        LineMarker marker1 = marker0;
        marker1.setLine(1);
        LineMarker marker2 = marker0;
        marker2.setLine(2);
        LineMarker marker3 = marker0;
        marker3.setLine(3);
        LineMarker marker4 = marker0;
        marker4.setLine(4);
        LineMarker marker5 = marker0;
        marker5.setLine(5);
        LineMarker marker6 = marker0;
        marker6.setLine(6);
        LineMarker marker7 = marker0;
        marker7.setLine(7);
        LineMarker marker8 = marker0;
        marker8.setLine(8);

        doc.tmp_moveLine(2, 5, &cursor1);

        const int pos0After = marker0.line();
        const int pos1After = marker1.line();
        const int pos2After = marker2.line();
        const int pos3After = marker3.line();
        const int pos4After = marker4.line();
        const int pos5After = marker5.line();
        const int pos6After = marker6.line();
        const int pos7After = marker7.line();
        const int pos8After = marker8.line();

        CHECK(marker0.line() == 0);
        CHECK(marker1.line() == 1);
        CHECK(marker2.line() == 4);
        CHECK(marker3.line() == 2);
        CHECK(marker4.line() == 3);
        CHECK(marker5.line() == 5);
        CHECK(marker6.line() == 6);
        CHECK(marker7.line() == 7);
        CHECK(marker8.line() == 8);

        doc.undo(&cursor1);

        CHECK(marker0.line() == 0);
        CHECK(marker1.line() == 1);
        CHECK(marker2.line() == 2);
        CHECK(marker3.line() == 3);
        CHECK(marker4.line() == 4);
        CHECK(marker5.line() == 5);
        CHECK(marker6.line() == 6);
        CHECK(marker7.line() == 7);
        CHECK(marker8.line() == 8);

        doc.redo(&cursor1);

        CHECK(marker0.line() == pos0After);
        CHECK(marker1.line() == pos1After);
        CHECK(marker2.line() == pos2After);
        CHECK(marker3.line() == pos3After);
        CHECK(marker4.line() == pos4After);
        CHECK(marker5.line() == pos5After);
        CHECK(marker6.line() == pos6After);
        CHECK(marker7.line() == pos7After);
        CHECK(marker8.line() == pos8After);

    }

    SECTION("move line up") {
        lines = QStringList{
            "line 1aaaaaa",
            "line 2b",
            "line 3ccc",
            "line 4dddddd",
            "line 5eeeee",
            "line 6ffffffff",
            "line 7gggg",
            "line 8",
            "line 9jjjjjjj"
        };

        cursor1.insertText(lines.join("\n"));

        LineMarker marker0{&doc, 0};
        LineMarker marker1 = marker0;
        marker1.setLine(1);
        LineMarker marker2 = marker0;
        marker2.setLine(2);
        LineMarker marker3 = marker0;
        marker3.setLine(3);
        LineMarker marker4 = marker0;
        marker4.setLine(4);
        LineMarker marker5 = marker0;
        marker5.setLine(5);
        LineMarker marker6 = marker0;
        marker6.setLine(6);
        LineMarker marker7 = marker0;
        marker7.setLine(7);
        LineMarker marker8 = marker0;
        marker8.setLine(8);

        doc.tmp_moveLine(4, 7, &cursor1);

        const int pos0After = marker0.line();
        const int pos1After = marker1.line();
        const int pos2After = marker2.line();
        const int pos3After = marker3.line();
        const int pos4After = marker4.line();
        const int pos5After = marker5.line();
        const int pos6After = marker6.line();
        const int pos7After = marker7.line();
        const int pos8After = marker8.line();

        CHECK(marker0.line() == 0);
        CHECK(marker1.line() == 1);
        CHECK(marker2.line() == 2);
        CHECK(marker3.line() == 3);
        CHECK(marker5.line() == 4);
        CHECK(marker6.line() == 5);
        CHECK(marker4.line() == 6);
        CHECK(marker7.line() == 7);
        CHECK(marker8.line() == 8);
        doc.undo(&cursor1);

        CHECK(marker0.line() == 0);
        CHECK(marker1.line() == 1);
        CHECK(marker2.line() == 2);
        CHECK(marker3.line() == 3);
        CHECK(marker4.line() == 4);
        CHECK(marker5.line() == 5);
        CHECK(marker6.line() == 6);
        CHECK(marker7.line() == 7);
        CHECK(marker8.line() == 8);

        doc.redo(&cursor1);

        CHECK(marker0.line() == pos0After);
        CHECK(marker1.line() == pos1After);
        CHECK(marker2.line() == pos2After);
        CHECK(marker3.line() == pos3After);
        CHECK(marker4.line() == pos4After);
        CHECK(marker5.line() == pos5After);
        CHECK(marker6.line() == pos6After);
        CHECK(marker7.line() == pos7After);
        CHECK(marker8.line() == pos8After);
    }
}

TEST_CASE("TextCursor::Position") {
    SECTION("default constructor") {
        TextCursor::Position pos;
        CHECK(pos.codeUnit == 0);
        CHECK(pos.line == 0);
    }

    SECTION("value constructor 5 7") {
        TextCursor::Position pos{5, 7};
        CHECK(pos.codeUnit == 5);
        CHECK(pos.line == 7);
    }

    SECTION("value constructor 7 5") {
        TextCursor::Position pos{7, 5};
        CHECK(pos.codeUnit == 7);
        CHECK(pos.line == 5);
    }


    SECTION("value constructor -2 -7") {
        TextCursor::Position pos{-2, -7};
        CHECK(pos.codeUnit == -2);
        CHECK(pos.line == -7);
    }

    SECTION("equals") {
        CHECK(TextCursor::Position{-2, -7} == TextCursor::Position{-2, -7});
        CHECK_FALSE(TextCursor::Position{-2, -7} != TextCursor::Position{-2, -7});

        CHECK_FALSE(TextCursor::Position{-2, -7} == TextCursor::Position{2, 7});
        CHECK(TextCursor::Position{-2, -7} != TextCursor::Position{2, 7});

        CHECK_FALSE(TextCursor::Position{15, 3} == TextCursor::Position{2, 7});
        CHECK(TextCursor::Position{15, 3} != TextCursor::Position{2, 7});
    }

    SECTION("less") {
        CHECK(TextCursor::Position{2, 6} < TextCursor::Position{2, 7});
        CHECK(TextCursor::Position{2, 7} < TextCursor::Position{3, 7});

        CHECK_FALSE(TextCursor::Position{-2, -7} < TextCursor::Position{-2, -7});
        CHECK_FALSE(TextCursor::Position{2, 7} < TextCursor::Position{2, 6});
        CHECK_FALSE(TextCursor::Position{3, 7} < TextCursor::Position{2, 7});
    }

    SECTION("less or equal") {
        CHECK(TextCursor::Position{-2, -7} <= TextCursor::Position{-2, -7});
        CHECK(TextCursor::Position{2, 6} <= TextCursor::Position{2, 7});
        CHECK(TextCursor::Position{2, 7} <= TextCursor::Position{3, 7});

        CHECK_FALSE(TextCursor::Position{2, 7} <= TextCursor::Position{2, 6});
        CHECK_FALSE(TextCursor::Position{3, 7} <= TextCursor::Position{2, 7});
    }

    SECTION("greater") {
        CHECK(TextCursor::Position{2, 7} > TextCursor::Position{2, 6});
        CHECK(TextCursor::Position{3, 7} > TextCursor::Position{2, 7});

        CHECK_FALSE(TextCursor::Position{-2, -7} > TextCursor::Position{-2, -7});
        CHECK_FALSE(TextCursor::Position{2, 6} > TextCursor::Position{2, 7});
        CHECK_FALSE(TextCursor::Position{2, 7} > TextCursor::Position{3, 7});
    }

    SECTION("greater or equal") {
        CHECK(TextCursor::Position{-2, -7} >= TextCursor::Position{-2, -7});
        CHECK(TextCursor::Position{2, 7} >= TextCursor::Position{2, 6});
        CHECK(TextCursor::Position{3, 7} >= TextCursor::Position{2, 7});

        CHECK_FALSE(TextCursor::Position{2, 6} >= TextCursor::Position{2, 7});
        CHECK_FALSE(TextCursor::Position{2, 7} >= TextCursor::Position{3, 7});
    }

}

TEST_CASE("document cross doc") {
    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{1, 1}};
    Tui::ZRoot root;
    terminal.setMainWidget(&root);

    Document doc1;

    TextCursor cursor1{&doc1, nullptr, [&terminal, &doc1](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc1.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    Document doc2;

    TextCursor cursor2{&doc2, nullptr, [&terminal, &doc2](int line, bool wrappingAllowed) {
            Tui::ZTextLayout lay(terminal.textMetrics(), doc2.line(line));
            lay.doLayout(65000);
            return lay;
        }
    };

    SECTION("simple") {
        cursor1.insertText("doc1");
        cursor2.insertText("doc2");

        TextCursor curX = cursor1;
        curX.setPosition({0, 0});
        curX.setPosition({4, 0}, true);
        CHECK(curX.selectedText() == "doc1");

        curX = cursor2;

        curX.setPosition({0, 0});
        curX.setPosition({4, 0}, true);
        CHECK(curX.selectedText() == "doc2");
    }

    SECTION("cursorChanged signal") {
        cursor1.insertText("doc1");
        cursor2.insertText("doc2");

        TextCursor curX = cursor1;
        curX.setPosition({0, 0});
        curX.setPosition({4, 0}, true);
        CHECK(curX.selectedText() == "doc1");

        EventRecorder recorder1;
        auto cursorChangedSignal1 = recorder1.watchSignal(&doc1, RECORDER_SIGNAL(&Document::cursorChanged));

        EventRecorder recorder2;
        auto cursorChangedSignal2 = recorder2.watchSignal(&doc2, RECORDER_SIGNAL(&Document::cursorChanged));

        recorder1.waitForEvent(cursorChangedSignal1);
        CHECK(recorder1.consumeFirst(cursorChangedSignal1, (const TextCursor*)&cursor1));
        CHECK(recorder1.consumeFirst(cursorChangedSignal1, (const TextCursor*)&curX));
        CHECK(recorder1.noMoreEvents());

        recorder2.waitForEvent(cursorChangedSignal2);
        CHECK(recorder2.consumeFirst(cursorChangedSignal2, (const TextCursor*)&cursor2));
        CHECK(recorder2.noMoreEvents());

        curX = cursor2;

        CAPTURE(&cursor1);
        CAPTURE(&cursor2);
        CAPTURE(&curX);

        recorder2.waitForEvent(cursorChangedSignal2);
        CHECK(recorder2.consumeFirst(cursorChangedSignal2, (const TextCursor*)&curX));
        CHECK(recorder2.noMoreEvents());
        CHECK(recorder1.noMoreEvents());

        curX.setPosition({0, 0});
        curX.setPosition({4, 0}, true);
        CHECK(curX.selectedText() == "doc2");

        recorder2.waitForEvent(cursorChangedSignal2);
        CHECK(recorder2.consumeFirst(cursorChangedSignal2, (const TextCursor*)&curX));
        CHECK(recorder2.noMoreEvents());
        CHECK(recorder1.noMoreEvents());

        curX.removeSelectedText();
        recorder2.waitForEvent(cursorChangedSignal2);
        CHECK(recorder2.consumeFirst(cursorChangedSignal2, (const TextCursor*)&cursor2));
        CHECK(recorder2.consumeFirst(cursorChangedSignal2, (const TextCursor*)&curX));
        CHECK(recorder2.noMoreEvents());
        CHECK(recorder1.noMoreEvents());
    }

    SECTION("lineMarkerChanged signal") {
        cursor1.insertText("doc1\nline");
        cursor2.insertText("doc2\nline");

        LineMarker marker1{&doc1, 1};
        LineMarker marker2{&doc2, 1};
        LineMarker marker = marker1;

        EventRecorder recorder1;
        auto lineMarkerChangedSignal1 = recorder1.watchSignal(&doc1, RECORDER_SIGNAL(&Document::lineMarkerChanged));

        EventRecorder recorder2;
        auto lineMarkerChangedSignal2 = recorder2.watchSignal(&doc2, RECORDER_SIGNAL(&Document::lineMarkerChanged));

        QCoreApplication::processEvents(QEventLoop::AllEvents);
        CHECK(recorder1.noMoreEvents());
        CHECK(recorder2.noMoreEvents());

        marker = marker2;

        QCoreApplication::processEvents(QEventLoop::AllEvents);

        CHECK(recorder1.noMoreEvents());
        CHECK(recorder2.consumeFirst(lineMarkerChangedSignal2, (const LineMarker*)&marker));
        CHECK(recorder2.noMoreEvents());

        cursor2.setPosition({0, 0});
        cursor2.insertText("\n");

        QCoreApplication::processEvents(QEventLoop::AllEvents);

        CHECK(recorder2.consumeFirst(lineMarkerChangedSignal2, (const LineMarker*)&marker2));
        CHECK(recorder2.consumeFirst(lineMarkerChangedSignal2, (const LineMarker*)&marker));
        CHECK(recorder2.noMoreEvents());
        CHECK(recorder1.noMoreEvents());
    }
}
