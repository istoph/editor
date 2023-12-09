// SPDX-License-Identifier: BSL-1.0

#define CATCH_CONFIG_RUNNER
#include "catchwrapper.h"

#include "file.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>

#include <Tui/ZTerminal.h>

int main( int argc, char* argv[] ) {
  QCoreApplication app(argc, argv);

  int result = Catch::Session().run( argc, argv );

  return result;
}

// Returns empty QByteArray() on failure.
QByteArray fileChecksum(const QString &fileName,
                        QCryptographicHash::Algorithm hashAlgorithm=QCryptographicHash::Sha256)
{
    QFile f(fileName);
    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(hashAlgorithm);
        if (hash.addData(&f)) {
            return hash.result();
        }
    }
    return QByteArray();
}

void readWrite(QString in) {
    Tui::ZTerminal::OffScreen of(80, 24);
    Tui::ZTerminal terminal(of);

    QString out = in + "_dub";

    File *f = new File(terminal.textMetrics(), nullptr);
    //f->setFilename(in);
    CHECK(f->openText(in));
    f->setFilename(out);
    CHECK(f->saveText());

    CHECK(fileChecksum(in) == fileChecksum(out));

    QFile fp(out);
    fp.remove();
}

TEST_CASE("read write zero byte") {

    QFile fp("zero");
    CHECK(fp.open(QFile::WriteOnly));
    fp.close();

    readWrite("zero");

    fp.remove();
}

TEST_CASE("read write 16k byte") {

    QFile fp("sixteen");
    CHECK(fp.open(QFile::WriteOnly));
    fp.write(QByteArray(16382,'0'));
    fp.close();

    readWrite("sixteen");

    fp.remove();
}

TEST_CASE("read write 64k byte") {

    QFile fp("64k");
    CHECK(fp.open(QFile::WriteOnly));
    fp.write(QByteArray(16382*4,'\xFF'));
    fp.close();

    readWrite("64k");

    fp.remove();
}

TEST_CASE("read write text") {

    QFile fp("text");
    CHECK(fp.open(QFile::WriteOnly));
    fp.write(QByteArray("Hier verewigen sich die Entwickler des Editors\nChristoph Hüffelmann und Martin Hostettler\n"));
    fp.close();

    readWrite("text");

    fp.remove();
}

TEST_CASE("read write text one newline") {

    QFile fp("text");
    CHECK(fp.open(QFile::WriteOnly));
    fp.write(QByteArray("Hier verewigen sich die Entwickler des Editors\nChristoph Hüffelmann und Martin Hostettler"));
    fp.close();

    readWrite("text");

    fp.remove();
}

TEST_CASE("read write dos text") {

    QFile fp("text");
    CHECK(fp.open(QFile::WriteOnly));
    fp.write(QByteArray("test\r\nblub\r\n"));
    fp.close();

    readWrite("text");

    fp.remove();
}

TEST_CASE("read write dos text one newline") {

    QFile fp("text");
    CHECK(fp.open(QFile::WriteOnly));
    fp.write(QByteArray("Hier verewigen sich \r\ndie Entwickler des Editors\r\nChristoph Hüffelmann und\r\nMartin Hostettler"));
    fp.close();

    readWrite("text");

    fp.remove();
}

TEST_CASE("all chars") {

    QFile fp("text");
    CHECK(fp.open(QFile::WriteOnly));
    for (int i = 1; i <= 0xFFFF; i++) {
        fp.write(QString(1,QChar(i)).toUtf8());
        if(i % 80 == 0) {
            fp.write("\n");
        }
    }
    for (int i = 0xFFFF; i <= 0x10FFFF; i++) {
        fp.write((QString(1,QChar::highSurrogate(i))+QString(1,QChar::lowSurrogate(i))).toUtf8());
        if(i % 80 == 0) {
            fp.write("\n");
        }
    }
    fp.close();

    readWrite("text");

    fp.remove();
}

TEST_CASE("read write") {

    readWrite("chr");

}
