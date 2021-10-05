#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include <QVector>

class Document {
public:
    QString _filename;
    QVector<QString> _text;
};

#endif // DOCUMENT_H
