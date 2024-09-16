#ifndef TIME_H
#define TIME_H

#include <common.hpp>

namespace vke_common
{
    class TimeManager
    {
    private:
        static TimeManager *instance;
        TimeManager() : frameStartTime(glfwGetTime()) {}
        ~TimeManager() {}
        TimeManager(const TimeManager &);
        TimeManager &operator=(const TimeManager);

    public:
        float frameStartTime;
        float prevFrameStartTime;
        float deltaTime;

        static TimeManager *GetInstance()
        {
            return instance;
        }

        static TimeManager *Init()
        {
            instance = new TimeManager();
            return instance;
        }

        static void Dispose()
        {
            delete instance;
        }

        static void Update()
        {
            instance->prevFrameStartTime = instance->frameStartTime;
            instance->frameStartTime = glfwGetTime();
            instance->deltaTime = instance->frameStartTime - instance->prevFrameStartTime;
        }
    };
}

#endif