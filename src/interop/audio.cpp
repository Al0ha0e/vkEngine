#include <interop/audio.hpp>
#include <scene.hpp>
#include <component/audio_source.hpp>
#include <component/audio_listener.hpp>

namespace vke_interop
{
    static vke_common::Scene *GetCurrentScene()
    {
        return vke_common::SceneManager::GetInstance()->currentScene.get();
    }

    static vke_component::AudioSource *GetAudioSource(vke_common::Scene *scene, uint32_t entity)
    {
        if (scene == nullptr) return nullptr;
        entt::entity ent = static_cast<entt::entity>(entity);
        if (!scene->registry.valid(ent) || !scene->registry.all_of<vke_component::AudioSource>(ent))
            return nullptr;
        return &scene->registry.get<vke_component::AudioSource>(ent);
    }

    static vke_component::AudioListener *GetAudioListener(vke_common::Scene *scene, uint32_t entity)
    {
        if (scene == nullptr) return nullptr;
        entt::entity ent = static_cast<entt::entity>(entity);
        if (!scene->registry.valid(ent) || !scene->registry.all_of<vke_component::AudioListener>(ent))
            return nullptr;
        return &scene->registry.get<vke_component::AudioListener>(ent);
    }

    void VKE_INTEROP_CDECL AudioSourcePlay(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->Play();
    }

    void VKE_INTEROP_CDECL AudioSourceStop(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->Stop();
    }

    void VKE_INTEROP_CDECL AudioSourcePause(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->Pause();
    }

    int32_t VKE_INTEROP_CDECL AudioSourceGetIsPlaying(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return (src && src->IsPlaying()) ? 1 : 0;
    }

    void VKE_INTEROP_CDECL AudioSourceSetLooping(uint32_t entity, int32_t looping)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetLooping(looping != 0);
    }

    int32_t VKE_INTEROP_CDECL AudioSourceGetLooping(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return (src && src->looping) ? 1 : 0;
    }

    void VKE_INTEROP_CDECL AudioSourceSetVolume(uint32_t entity, float volume)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetVolume(volume);
    }

    float VKE_INTEROP_CDECL AudioSourceGetVolume(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return src ? src->volume : 0.0f;
    }

    void VKE_INTEROP_CDECL AudioSourceSetPitch(uint32_t entity, float pitch)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetPitch(pitch);
    }

    float VKE_INTEROP_CDECL AudioSourceGetPitch(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return src ? src->pitch : 1.0f;
    }

    void VKE_INTEROP_CDECL AudioSourceSetTime(uint32_t entity, float time)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetTime(time);
    }

    float VKE_INTEROP_CDECL AudioSourceGetTime(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return src ? src->GetTime() : 0.0f;
    }

    void VKE_INTEROP_CDECL AudioSourceSetSpatializationEnabled(uint32_t entity, int32_t enabled)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetSpatializationEnabled(enabled != 0);
    }

    int32_t VKE_INTEROP_CDECL AudioSourceGetSpatializationEnabled(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return (src && src->spatializationEnabled) ? 1 : 0;
    }

    void VKE_INTEROP_CDECL AudioSourceSetAttenuationModel(uint32_t entity, int32_t model)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetAttenuationModel(model);
    }

    int32_t VKE_INTEROP_CDECL AudioSourceGetAttenuationModel(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return src ? src->attenuationModel : 1;
    }

    void VKE_INTEROP_CDECL AudioSourceSetRolloff(uint32_t entity, float factor)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetRolloff(factor);
    }

    float VKE_INTEROP_CDECL AudioSourceGetRolloff(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return src ? src->rolloff : 1.0f;
    }

    void VKE_INTEROP_CDECL AudioSourceSetMinDistance(uint32_t entity, float distance)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetMinDistance(distance);
    }

    float VKE_INTEROP_CDECL AudioSourceGetMinDistance(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return src ? src->minDistance : 1.0f;
    }

    void VKE_INTEROP_CDECL AudioSourceSetMaxDistance(uint32_t entity, float distance)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetMaxDistance(distance);
    }

    float VKE_INTEROP_CDECL AudioSourceGetMaxDistance(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return src ? src->maxDistance : 100.0f;
    }

    void VKE_INTEROP_CDECL AudioSourceSetDopplerFactor(uint32_t entity, float factor)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        if (src) src->SetDopplerFactor(factor);
    }

    float VKE_INTEROP_CDECL AudioSourceGetDopplerFactor(uint32_t entity)
    {
        auto *src = GetAudioSource(GetCurrentScene(), entity);
        return src ? src->dopplerFactor : 1.0f;
    }

    void VKE_INTEROP_CDECL AudioListenerSetEnabled(uint32_t entity, int32_t enabled)
    {
        auto *lis = GetAudioListener(GetCurrentScene(), entity);
        if (lis) lis->enabled = (enabled != 0);
    }

    int32_t VKE_INTEROP_CDECL AudioListenerGetEnabled(uint32_t entity)
    {
        auto *lis = GetAudioListener(GetCurrentScene(), entity);
        return (lis && lis->enabled) ? 1 : 0;
    }
}
