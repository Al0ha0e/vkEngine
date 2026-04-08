#ifndef ENGINE_STATE_H
#define ENGINE_STATE_H

#include <common.hpp>
#include <cstdint>

namespace vke_common
{
    enum class EngineState : int32_t
    {
        Running = 0,
        Paused = 1,
        Terminated = 2,
    };

    class EngineStateManager
    {
    private:
        static EngineStateManager *instance;
        EngineStateManager()
            : state(EngineState::Running)
        {
        }
        ~EngineStateManager() {}
        EngineStateManager(const EngineStateManager &);
        EngineStateManager &operator=(const EngineStateManager &);

    public:
        EngineState state;

        static EngineStateManager *GetInstance()
        {
            return instance;
        }

        static EngineStateManager *Init()
        {
            if (instance == nullptr)
                instance = new EngineStateManager();
            return instance;
        }

        static void Dispose()
        {
            if (instance == nullptr)
                return;
            delete instance;
            instance = nullptr;
        }

        static void SetState(EngineState nextState)
        {
            instance->state = nextState;
        }

        static EngineState GetState()
        {
            return instance->state;
        }
    };
}

#endif
