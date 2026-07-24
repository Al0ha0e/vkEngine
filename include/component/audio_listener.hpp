#ifndef AUDIO_LISTENER_H
#define AUDIO_LISTENER_H

#include <cstdint>
#include <nlohmann/json.hpp>

namespace vke_component
{
    struct AudioListenerData
    {
        bool enabled = true;

        AudioListenerData() = default;
        AudioListenerData(const nlohmann::json &json) : enabled(json.value("enabled", true)) {}

        nlohmann::json ToJSON() const
        {
            return {{"type", "audioListener"}, {"enabled", enabled}};
        }
    };

    class AudioListener
    {
    public:
        int32_t listenerIndex = 0;
        bool enabled = true;

        bool listenerLoaded = false;

        AudioListener() = default;

        AudioListener(const AudioListenerData &componentData) : enabled(componentData.enabled) {}

        void FillData(AudioListenerData &data) const
        {
            data.enabled = enabled;
        }

        void LoadToEngine()
        {
            listenerLoaded = true;
        }

        void UnloadFromEngine()
        {
            listenerLoaded = false;
        }
    };
}

#endif
