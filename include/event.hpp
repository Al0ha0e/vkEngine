#ifndef EVENT_H
#define EVENT_H

#include <iostream>
#include <functional>
#include <map>

#include <ds/id_allocator.hpp>

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
        using callbackTable_t = std::map<vke_ds::id32_t, innerCallback_t>;

    public:
        using callback_t = std::function<void(void *, MT *)>;

        EventHub() : idAllocator(0) {}
        ~EventHub() {}
        EventHub(const EventHub<MT> &) = delete;
        EventHub &operator=(const EventHub<MT> &) = delete;
        EventHub(const EventHub<MT> &&ano) : idAllocator(ano.idAllocator), callbackTable(std::forward<callbackTable_t>(ano.callbackTable)) {}
        EventHub &operator=(EventHub<MT> &&ano)
        {
            idAllocator = ano.idAllocator;
            callbackTable = std::forward<callbackTable_t>(ano.callbackTable);
            return *this;
        }

        vke_ds::id32_t AddEventListener(void *listener, callback_t &callback)
        {
            // TODO check type
            vke_ds::id32_t ret = idAllocator.Alloc();
            callbackTable[ret] = std::bind(callback, listener, std::placeholders::_1);
            return ret;
        }

        void RemoveEventListener(vke_ds::id32_t id)
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
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> idAllocator;
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

        static vke_ds::id32_t AddEventListener(GlobalEventType type, void *listener, EventCallback &callback)
        {
            // TODO check type
            return instance->eventHubs[type].AddEventListener(listener, callback);
        }

        static void RemoveEventListener(GlobalEventType type, vke_ds::id32_t id)
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