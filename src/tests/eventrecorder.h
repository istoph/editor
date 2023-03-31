// SPDX-License-Identifier: BSL-1.0

#ifndef EVENTRECORDER_H
#define EVENTRECORDER_H

#include <any>
#include <map>
#include <tuple>
#include <unordered_map>

#include <QObject>
#include <QCoreApplication>

#include "catchwrapper.h"
#include <Tui/ZEvent.h>

#define RECORDER_SIGNAL(signal) signal, std::string(#signal).substr(1)

class EventRecorder : public QObject {
public:
    class RecorderEvent {
    public:
        std::string name;
    };

public:
    template<typename SIGNAL>
    std::shared_ptr<RecorderEvent> watchSignal(const typename QtPrivate::FunctionPointer<SIGNAL>::Object *sender, SIGNAL signal, std::string name) {
        auto event = std::make_shared<RecorderEvent>();
        event->name = "Signal: " + name;
        if (sender->objectName().size()) {
            event->name += " on " + sender->objectName().toStdString();
        }
        QObject::connect(sender, signal, this, [this, event](auto... arguments) {
            std::vector<std::any> args;
            (args.emplace_back(arguments), ...);
            records.push_back(Record{event, std::move(args)});
            if (event == _waitingEvent) {
                _waitingEvent = nullptr;
            }
        });
        return event;
    }

    std::shared_ptr<RecorderEvent> watchEvent(QObject *o, std::string name, std::function<void(std::shared_ptr<EventRecorder::RecorderEvent>, const QEvent*)> translator);

    std::shared_ptr<RecorderEvent> watchCloseEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchEnabledChangeEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchFocusInEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchFocusOutEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchHideEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchHideToParentEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchKeyEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchLayoutRequestEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchMoveEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchPasteEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchPendingRawSequenceEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchQueryAcceptsEnterEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchRawSequenceEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchResizeEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchShowEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchShowToParentEvent(QObject *o, std::string name);
    std::shared_ptr<RecorderEvent> watchTerminalChangeEvent(QObject *o, std::string name);

    std::shared_ptr<RecorderEvent> createEvent(const std::string &name);

    template<typename... ARGUMENTS>
    void recordEvent(std::shared_ptr<RecorderEvent> event, ARGUMENTS... arguments) {
        std::vector<std::any> args;
        (args.emplace_back(arguments), ...);
        records.push_back(Record{ event, std::move(args)});
        if (event == _waitingEvent) {
            _waitingEvent = nullptr;
        }
    }

    template<typename... ARGS>
    [[nodiscard]]
    bool consumeFirst(std::shared_ptr<RecorderEvent> event, ARGS... args) {
        if (!records.size()) {
            UNSCOPED_INFO("No more events recorded");
            return false;
        }
        auto actualEvent = records[0].event;
        auto actualArgs = records[0].args;
        records.erase(records.begin());
        if (event != actualEvent) {
            std::string expectedName = event->name;
            UNSCOPED_INFO("Event does not match. Called was " << actualEvent->name << " expected was " << expectedName);
            return false;
        }
        return checkArgs(0, actualArgs, args...);
    }

    [[nodiscard]]
    bool noMoreEvents();

    void waitForEvent(std::shared_ptr<RecorderEvent> event);

    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    bool checkArgs(size_t idx, std::vector<std::any> actual) {
        (void)idx; (void)actual;
        // end of recursion
        return true;
    }

    template<typename T, typename... ARGS>
    bool checkArgs(size_t idx, const std::vector<std::any> &actualArgs, T expected, ARGS... expectedRest) {
        if (idx >= actualArgs.size()) {
            UNSCOPED_INFO("More arguments specified than available");
            return false;
        }
        bool ok = true;
        if (actualArgs[idx].type() == typeid(expected)) {
            if (std::any_cast<T>(actualArgs[idx]) != expected) {
                ok = false;
                UNSCOPED_INFO("argument " << idx + 1 << " does not match expected value");
                auto actual = std::any_cast<T>(actualArgs[idx]);
                CHECK(actual == expected);
            }
        } else {
            UNSCOPED_INFO("Argument type mismatch on argument " << idx + 1 << " actual type " << actualArgs[idx].type().name());
            ok = false;
        }
        return ok && checkArgs(idx + 1, actualArgs, expectedRest...);
    }

    struct Record {
        std::shared_ptr<RecorderEvent> event;
        std::vector<std::any> args;
    };

    std::vector<Record> records;
    std::map<const QObject*, std::vector<std::tuple<std::function<void(std::shared_ptr<EventRecorder::RecorderEvent>, const QEvent*)>,
                                         std::shared_ptr<EventRecorder::RecorderEvent>>>> registeredQObjects;

    std::shared_ptr<RecorderEvent> _waitingEvent;
};

using RecorderEvent = std::shared_ptr<EventRecorder::RecorderEvent>;

#endif // EVENTRECORDER_H
