#ifndef EVENT_H
#define EVENT_H

#include <iostream>
#include <functional>
#include <map>

namespace vke_common
{
    enum GlobalEventType
    {
        EVENT_WINDOW_RESIZE,
        EVENT_MOUSE_CLICK,
    };

    using EventType = int;
    using EventCallback = std::function<void(void *, void *)>;

    template <typename MT>
    class EventHub
    {
    private:
        using innerCallback_t = std::function<void(MT *)>;
        using callbackTable_t = std::map<int, innerCallback_t>;

    public:
        using callback_t = std::function<void(void *, MT *)>;

        EventHub() : id(0)
        {
        }
        ~EventHub() {}
        EventHub(const EventHub<MT> &) = delete;
        EventHub &operator=(const EventHub<MT> &) = delete;
        EventHub(const EventHub<MT> &&ano) : id(ano.id), callbackTable(std::forward<callbackTable_t>(ano.callbackTable)) {}
        EventHub &operator=(EventHub<MT> &&ano)
        {
            id = ano.id;
            callbackTable = std::forward<callbackTable_t>(ano.callbackTable);
            return *this;
        }

        int AddEventListener(void *listener, callback_t &callback)
        {
            // TODO check type
            int ret = ++id;
            callbackTable[ret] = std::bind(callback, listener, std::placeholders::_1);
            return ret;
        }

        void RemoveEventListener(int id)
        {
            callbackTable.erase(id);
        }

        void DispatchEvent(MT *info)
        {
            for (auto &kv : callbackTable)
                kv.second(info);
        }

    private:
        callbackTable_t callbackTable;
        int id;
    };

    class EventSystem
    {
    private:
        static EventSystem *instance;
        EventSystem() {}
        ~EventSystem() {}
        EventSystem(const EventSystem &);
        EventSystem &operator=(const EventSystem);

        using eventHubType = EventHub<void>;

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
            instance->eventHubs[EVENT_WINDOW_RESIZE] = std::move(eventHubType());
            return instance;
        }

        static void Dispose()
        {
            delete instance;
        }

        static int AddEventListener(GlobalEventType type, void *listener, EventCallback &callback)
        {
            // TODO check type
            return instance->eventHubs[type].AddEventListener(listener, callback);
        }

        static void RemoveEventListener(GlobalEventType type, int id)
        {
            instance->eventHubs[type].RemoveEventListener(id);
        }

        static void DispatchEvent(GlobalEventType type, void *info)
        {
            instance->eventHubs[type].DispatchEvent(info);
        }

    private:
        std::map<GlobalEventType, eventHubType> eventHubs;
    };
}

#endif