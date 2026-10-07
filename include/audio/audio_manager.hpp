#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

struct ma_engine;
struct ma_sound;

#include <cstdint>
#include <unordered_set>

namespace vke_audio
{
    class AudioManager
    {
    private:
        static AudioManager *instance;
        ma_engine *engine;
        std::unordered_set<ma_sound *> sounds;

        AudioManager();
        ~AudioManager();
        AudioManager(const AudioManager &) = delete;
        AudioManager &operator=(const AudioManager &) = delete;

    public:
        static AudioManager *GetInstance() { return instance; }
        static AudioManager *Init();
        static void Dispose();
        static void Reset();

        // The manager owns sound allocations; components keep borrowed pointers.
        static ma_sound *LoadSound(const char *path, uint32_t flags);
        static void ReleaseSound(ma_sound *sound);

        static void Update(float deltaTime);

        static ma_engine *GetEngine() { return instance ? instance->engine : nullptr; }
    };
}

#endif
