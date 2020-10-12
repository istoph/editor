#include "clipboard.h"

Clipboard::Clipboard() {

}

void Clipboard::append(QString append) {
    _clipboard.append(append);
}
void Clipboard::append(QVector<QString> append) {
    _clipboard.append(append);
}

void Clipboard::clear() {
    _clipboard.clear();
}

QVector<QString> Clipboard::getClipboard() {
    return _clipboard;
}

void Clipboard::setClipboard(QVector<QString> clipboard) {
    _clipboard = clipboard;
}

