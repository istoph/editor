// SPDX-License-Identifier: BSL-1.0

#include "eventrecorder.h"

#include <QElapsedTimer>
#include <QSet>
#include <Tui/ZSymbol.h>

RecorderEvent EventRecorder::watchEvent(QObject *o, std::string name, std::function<void (std::shared_ptr<EventRecorder::RecorderEvent>, const QEvent *)> translator) {
    auto event = std::make_shared<RecorderEvent>();
    event->name = name;

    if (o->objectName().size()) {
        event->name += " on " + o->objectName().toStdString();
    }

    auto it = registeredQObjects.find(o);
    if (it == registeredQObjects.end()) {
        o->installEventFilter(this);
        registeredQObjects[o];
    }
    registeredQObjects[o].emplace_back(translator, event);

    QObject::connect(o, &QObject::destroyed, this, [this, o] {
        o->removeEventFilter(this);
        registeredQObjects.erase(o);
    });

    return event;
}


RecorderEvent EventRecorder::watchCloseEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::close()) {
            auto &event = dynamic_cast<const Tui::ZCloseEvent&>(*ev);
            recordEvent(eventRef, event.skipChecks());
        }
    });
}

RecorderEvent EventRecorder::watchEnabledChangeEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *event) {
        if (event->type() == QEvent::EnabledChange) {
            recordEvent(eventRef);
        }
    });
}

RecorderEvent EventRecorder::watchFocusInEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::focusIn()) {
            auto &event = dynamic_cast<const Tui::ZFocusEvent&>(*ev);
            recordEvent(eventRef, event.reason());
        }
    });
}

RecorderEvent EventRecorder::watchFocusOutEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::focusOut()) {
            auto &event = dynamic_cast<const Tui::ZFocusEvent&>(*ev);
            recordEvent(eventRef, event.reason());
        }
    });
}

RecorderEvent EventRecorder::watchHideEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *event) {
        if (event->type() == Tui::ZEventType::hide()) {
            recordEvent(eventRef);
        }
    });
}

RecorderEvent EventRecorder::watchHideToParentEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *event) {
        if (event->type() == QEvent::HideToParent) {
            recordEvent(eventRef);
        }
    });
}

RecorderEvent EventRecorder::watchKeyEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::key()) {
            auto &event = dynamic_cast<const Tui::ZKeyEvent&>(*ev);
            recordEvent(eventRef, event.key(), event.text(), event.modifiers());
        }
    });
}

RecorderEvent EventRecorder::watchLayoutRequestEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *event) {
        if (event->type() == QEvent::LayoutRequest) {
            recordEvent(eventRef);
        }
    });
}

RecorderEvent EventRecorder::watchMoveEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::move()) {
            auto &event = dynamic_cast<const Tui::ZMoveEvent&>(*ev);
            recordEvent(eventRef, event.pos(), event.oldPos());
        }
    });
}

RecorderEvent EventRecorder::watchPasteEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::paste()) {
            auto &event = dynamic_cast<const Tui::ZPasteEvent&>(*ev);
            recordEvent(eventRef, event.text());
        }
    });
}

RecorderEvent EventRecorder::watchPendingRawSequenceEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::pendingRawSequence()) {
            auto &event = dynamic_cast<const Tui::ZRawSequenceEvent&>(*ev);
            recordEvent(eventRef, event.sequence());
        }
    });
}

RecorderEvent EventRecorder::watchQueryAcceptsEnterEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *event) {
        if (event->type() == Tui::ZEventType::queryAcceptsEnter()) {
            recordEvent(eventRef);
        }
    });
}

RecorderEvent EventRecorder::watchRawSequenceEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::rawSequence()) {
            auto &event = dynamic_cast<const Tui::ZRawSequenceEvent&>(*ev);
            recordEvent(eventRef, event.sequence());
        }
    });
}

RecorderEvent EventRecorder::watchResizeEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::resize()) {
            auto &event = dynamic_cast<const Tui::ZResizeEvent&>(*ev);
            recordEvent(eventRef, event.size(), event.oldSize());
        }
    });
}

RecorderEvent EventRecorder::watchShowEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *event) {
        if (event->type() == Tui::ZEventType::show()) {
            recordEvent(eventRef);
        }
    });
}

RecorderEvent EventRecorder::watchShowToParentEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *event) {
        if (event->type() == QEvent::ShowToParent) {
            recordEvent(eventRef);
        }
    });
}

RecorderEvent EventRecorder::watchTerminalChangeEvent(QObject *o, std::string name) {
    return watchEvent(o, name, [this](std::shared_ptr<EventRecorder::RecorderEvent> eventRef, const QEvent *ev) {
        if (ev->type() == Tui::ZEventType::terminalChange()) {
            recordEvent(eventRef);
        }
    });
}


RecorderEvent EventRecorder::createEvent(const std::string &name) {
    auto event = std::make_shared<RecorderEvent>();
    event->name = name;
    return event;
}

bool EventRecorder::noMoreEvents() {
    if (!records.empty()) {
        UNSCOPED_INFO("Next event would be " << records.front().event->name);
    }
    return records.empty();
}

bool EventRecorder::eventFilter(QObject *watched, QEvent *event) {
    auto it = registeredQObjects.find(watched);
    if (it != registeredQObjects.end()) {
        auto &vec = it->second;
        for (auto &item: vec) {
            std::get<0>(item)(std::get<1>(item), event);
        }
    }
    return false;
}

void EventRecorder::waitForEvent(std::shared_ptr<RecorderEvent> event) {
    for (const auto &record: records) {
        if (record.event == event) {
            return;
        }
    }

    _waitingEvent = event;
    QElapsedTimer timer;
    timer.start();
    while (_waitingEvent != nullptr) {
        if (timer.hasExpired(10000)) {
            FAIL_CHECK("EventRecorder::waitForEvent: Timeout");
            return;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }
}

void EventRecorder::clearEvents() {
    records.clear();
}
