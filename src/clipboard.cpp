// SPDX-License-Identifier: BSL-1.0

#include "clipboard.h"

Clipboard::Clipboard() {
}

void Clipboard::clear() {
    _clipboard.clear();
}

QString Clipboard::getClipboard() {
    return _clipboard;
}

void Clipboard::setClipboard(const QString &clipboard) {
    _clipboard = clipboard;
}

