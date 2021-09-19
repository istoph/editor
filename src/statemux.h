#ifndef STATEMUX_H
#define STATEMUX_H

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <QObject>

class StateMux {
public:
    template<typename SENDER, typename SIGNAL, typename RECEIVER, typename... ARGS>
    void connect(void *id, SENDER *sender, SIGNAL signal, RECEIVER *receiver, void (RECEIVER::*slot)(ARGS...),
                 ARGS... initial) {
        auto itemUniquePtr = std::make_unique<Item<RECEIVER, void (RECEIVER::*)(ARGS...), ARGS...>>(receiver, slot, std::make_tuple(initial...));
        auto item = itemUniquePtr.get();
        _inputs[id].items.push_back(move(itemUniquePtr));

        auto connection = QObject::connect(sender, signal, [this, item, receiver, slot, id] (ARGS... args) {
            item->data = std::make_tuple(args...);
            if (_active == id) {
                (receiver->*slot)(args...);
            }
        });
        _inputs[id].connections.push_back(connection);
    }

    void selectInput(void *id);

    void removeInput(void *id);

private:
    struct ItemBase {
        virtual ~ItemBase() {};
        virtual void push() = 0;
    };

    template <typename RECEIVER, typename SLOT, typename... ARGS>
    struct Item : public ItemBase {
        Item(RECEIVER *receiver, SLOT slot, std::tuple<ARGS...> initial) : receiver(receiver), slot(slot), data(initial) {};

        template <std::size_t... Is>
        void pushImpl(std::index_sequence<Is...>) {
            (receiver->*slot)(std::get<Is>(data)...);

        };

        void push() override {
            pushImpl(std::index_sequence_for<ARGS...>{});
        };

        RECEIVER *receiver = nullptr;
        SLOT slot = nullptr;
        std::tuple<ARGS...> data;
    };

    struct Input {
        std::vector<QMetaObject::Connection> connections;
        std::vector<std::unique_ptr<ItemBase>> items;
    };

    void *_active = nullptr;
    std::map<void*, Input> _inputs;
};

#endif // STATEMUX_H
