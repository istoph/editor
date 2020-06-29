#define CATCH_CONFIG_MAIN

#include "third-party/catch.hpp"
#include "file.h"
#include <QCryptographicHash>

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
    QString out = in + "_dub";

    File *f = new File(nullptr);
    f->setFilename(in);
    CHECK(f->openText());
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

TEST_CASE("read write") {

    readWrite("editor");

}
