// SPDX-License-Identifier: BSL-1.0

#include "attributes.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>

Attributes::Attributes() {
    Attributes("");
}

Attributes::Attributes(QString attributesFile) {
    setAttributesFile(attributesFile);
}

bool Attributes::readAttributes() {
    if (_attributesFile.isEmpty()) {
        return false;
    }
    if (_loaded) {
        return true;
    }
    QFile fileRead(_attributesFile);
    if (!fileRead.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //qDebug() << "File open error";
        return false;
    }
    QString val = fileRead.readAll();
    fileRead.close();

    QJsonDocument jd = QJsonDocument::fromJson(val.toUtf8());
    _attributeObject = jd.object();
    _loaded = true;
    return true;
}

Tui::ZDocumentCursor::Position Attributes::getAttributesCursorPosition(QString filename) {
    if (readAttributes()) {
        QJsonObject data = _attributeObject.value(filename).toObject();

        if (data.contains("cursorPositionX") && data.contains("cursorPositionY")) {
            return {data.value("cursorPositionX").toInt(), data.value("cursorPositionY").toInt()};
        }
        return {data.value("curOff").toInt(), data.value("curLine").toInt()};
    }
    return {0, 0};
}

int Attributes::getAttributesScrollCol(QString filename) {
    if (readAttributes()) {
        if (!_attributeObject.isEmpty() && _attributeObject.contains(filename)) {
            QJsonObject data = _attributeObject.value(filename).toObject();
            return data.value("sCol").toInt();
        }
    }
    return 0;
}

int Attributes::getAttributesScrollLine(QString filename) {
    if (readAttributes()) {
        if (!_attributeObject.isEmpty() && _attributeObject.contains(filename)) {
            QJsonObject data = _attributeObject.value(filename).toObject();
            return data.value("sLine").toInt();
        }
    }
    return 0;
}

int Attributes::getAttributesScrollFine(QString filename) {
    if (readAttributes()) {
        if (!_attributeObject.isEmpty() && _attributeObject.contains(filename)) {
            QJsonObject data = _attributeObject.value(filename).toObject();
            return data.value("sFine").toInt();
        }
    }
    return 0;
}

bool Attributes::writeAttributes(QString filename, Tui::ZDocumentCursor::Position cursorPosition, int scrollPositionColumn, int scrollPositionLine, int scrollPositionFineLine) {
    QFileInfo filenameInfo(filename);
    if (!filenameInfo.exists() || _attributesFile.isEmpty()) {
        return false;
    }
    readAttributes();

    const auto [cursorCodeUnit, cursorLine] = cursorPosition;

    QJsonObject data;
    data.insert("curOff", cursorCodeUnit);
    data.insert("curLine", cursorLine);
    data.insert("sCol", scrollPositionColumn);
    data.insert("sLine", scrollPositionLine);
    data.insert("sFine", scrollPositionFineLine);

    _attributeObject.insert(filenameInfo.absoluteFilePath(), data);

    QJsonDocument jsonDoc;
    jsonDoc.setObject(_attributeObject);

    //Save
    QDir d = QFileInfo(_attributesFile).absoluteDir();
    QString absolute=d.absolutePath();
    if (!QDir(absolute).exists()) {
        if (QDir().mkdir(absolute)) {
            qWarning("%s%s", "can not create directory: ", absolute.toUtf8().data());
            return false;
        }
    }

    QSaveFile file(_attributesFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("%s%s", "can not save attributes file: ", _attributesFile.toUtf8().data());
        return false;
    }
    file.write(jsonDoc.toJson());
    file.commit();
    return true;
}

void Attributes::setAttributesFile(QString attributesFile) {
    if (!attributesFile.isEmpty() && !attributesFile.isNull()) {
        QFileInfo file(attributesFile);
        if (file.isFile() && file.isWritable()) {
            _attributesFile = file.absoluteFilePath();
        }
    } else {
        _attributesFile = "";
    }
}

QString Attributes::attributesFile() {
    return _attributesFile;
}
