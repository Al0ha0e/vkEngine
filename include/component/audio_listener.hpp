#ifndef AUDIO_LISTENER_H
#define AUDIO_LISTENER_H

#include <cstdint>
#include <nlohmann/json.hpp>

namespace vke_component
{
    class AudioListener
    {
    public:
        int32_t listenerIndex = 0;
        bool enabled = true;

        bool listenerLoaded = false;

        AudioListener() = default;

        AudioListener(const nlohmann::json &json)
        {
            enabled = json.value("enabled", true);
        }

        void LoadToEngine(uint32_t entity)
        {
            listenerLoaded = true;
        }

        void UnloadFromEngine()
        {
            listenerLoaded = false;
        }

        nlohmann::json ToJSON()
        {
            return {
                {"type", "audioListener"},
                {"enabled", enabled}};
        }
    };
}

#endif
