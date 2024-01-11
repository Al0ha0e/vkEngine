#ifndef EVENT_H
#define EVENT_H

#include <iostream>
#include <functional>
#include <map>

namespace vke_common
{
    enum EventType
    {
        EVENT_WINDOW_RESIZE
    };

    using EventCallback = std::function<void(void *, void *)>;

    class EventSystem
    {
    private:
        static EventSystem *instance;
        EventSystem() {}
        ~EventSystem() {}
        EventSystem(const EventSystem &);
        EventSystem &operator=(const EventSystem);

        using innerCallback = std::function<void(void *)>;
        using callbackTable = std::map<int, innerCallback>;

    public:
        static EventSystem *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("EventSystem not initialized!");
            return instance;
        }

        static EventSystem *Init()
        {
            instance = new EventSystem;
            instance->callbackTables[EVENT_WINDOW_RESIZE] = callbackTable();
            instance->ids[EVENT_WINDOW_RESIZE] = 0;
            return instance;
        }

        static void Dispose()
        {
            delete instance;
        }

        static int AddEventListener(EventType type, void *listener, EventCallback &callback)
        {
            // TODO check type
            int ret = ++(instance->ids[type]);
            (instance->callbackTables[type])[ret] = std::bind(callback, listener, std::placeholders::_1);
            return ret;
        }

        static void RemoveEventListener(EventType type, int id)
        {
            instance->callbackTables[type].erase(id);
        }

        static void DispatchEvent(EventType type, void *info)
        {
            for (auto &kv : instance->callbackTables[type])
                kv.second(info);
        }

    private:
        std::map<EventType, callbackTable> callbackTables;
        std::map<EventType, int> ids;
    };
}

#endif