// SPDX-License-Identifier: BSL-1.0

#include "statemux.h"

void StateMux::selectInput(void *id) {
    if (_active != id) {
        auto inputIt = _inputs.find(id);
        if (inputIt != _inputs.end()) {
            _active = id;
            auto &input = inputIt->second;
            for (auto &item: input.items) {
                item->push();
            }
        }

    }
}

void StateMux::removeInput(void *id) {
    auto inputIt = _inputs.find(id);
    if (inputIt != _inputs.end()) {
        auto &input = inputIt->second;
        for (auto connection: input.connections) {
            QObject::disconnect(connection);
        }
    }
}
