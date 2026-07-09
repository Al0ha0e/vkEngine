#include <audio/audio_manager.hpp>
#include <audio/audio_clip.hpp>
#include <miniaudio/miniaudio.h>
#include <scene.hpp>
#include <component/audio_source.hpp>
#include <component/audio_listener.hpp>
#include <logger.hpp>

namespace vke_audio
{
    AudioManager *AudioManager::instance = nullptr;

    AudioManager::AudioManager()
        : engine(nullptr)
    {
        ma_engine_config config = ma_engine_config_init();
        config.channels = 2;
        config.sampleRate = 48000;

        engine = new ma_engine;
        ma_result result = ma_engine_init(&config, engine);
        if (result != MA_SUCCESS)
        {
            VKE_LOG_ERROR("AudioManager: failed to initialize ma_engine (error code {})", (int)result);
            delete engine;
            engine = nullptr;
        }
        else
        {
            VKE_LOG_INFO("AudioManager initialized (channels=2, sampleRate=48000)");
        }
    }

    AudioManager::~AudioManager()
    {
        if (engine)
        {
            ma_engine_uninit(engine);
            delete engine;
            engine = nullptr;
        }
    }

    AudioManager *AudioManager::Init()
    {
        if (instance == nullptr)
            instance = new AudioManager();
        return instance;
    }

    void AudioManager::Dispose()
    {
        delete instance;
        instance = nullptr;
    }

    void AudioManager::Update(float deltaTime)
    {
        if (instance == nullptr || instance->engine == nullptr)
            return;

        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        if (scene == nullptr || !scene->loadedToEngine)
            return;

        auto srcView = scene->registry.view<vke_common::Transform, vke_component::AudioSource>();
        for (auto [entity, transform, src] : srcView.each())
        {
            if (!src.soundInitialized)
                continue;

            if (src.spatializationEnabled)
            {
                glm::vec3 pos = transform.GetGlobalPosition();
                ma_sound_set_position(src.sound, pos.x, pos.y, pos.z);
            }
        }

        auto lisView = scene->registry.view<vke_common::Transform, vke_component::AudioListener>();
        for (auto [entity, transform, listener] : lisView.each())
        {
            if (!listener.enabled || !listener.listenerLoaded)
                continue;

            glm::vec3 pos = transform.GetGlobalPosition();
            glm::quat rot = transform.GetGlobalRotation();
            glm::vec3 forward = rot * glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 up = rot * glm::vec3(0.0f, 1.0f, 0.0f);

            ma_engine_listener_set_position(instance->engine, listener.listenerIndex, pos.x, pos.y, pos.z);
            ma_engine_listener_set_direction(instance->engine, listener.listenerIndex, forward.x, forward.y, forward.z);
            ma_engine_listener_set_world_up(instance->engine, listener.listenerIndex, up.x, up.y, up.z);
        }
    }
}
