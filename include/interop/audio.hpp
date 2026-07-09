#ifndef INTEROP_AUDIO_H
#define INTEROP_AUDIO_H

#include <cstdint>
#include <interop/interop.hpp>

namespace vke_interop
{
    using AudioSourcePlayFn = void(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceStopFn = void(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourcePauseFn = void(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceGetIsPlayingFn = int32_t(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetLoopingFn = void(VKE_INTEROP_CDECL *)(uint32_t, int32_t);
    using AudioSourceGetLoopingFn = int32_t(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetVolumeFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using AudioSourceGetVolumeFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetPitchFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using AudioSourceGetPitchFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetTimeFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using AudioSourceGetTimeFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetSpatializationEnabledFn = void(VKE_INTEROP_CDECL *)(uint32_t, int32_t);
    using AudioSourceGetSpatializationEnabledFn = int32_t(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetAttenuationModelFn = void(VKE_INTEROP_CDECL *)(uint32_t, int32_t);
    using AudioSourceGetAttenuationModelFn = int32_t(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetRolloffFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using AudioSourceGetRolloffFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetMinDistanceFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using AudioSourceGetMinDistanceFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetMaxDistanceFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using AudioSourceGetMaxDistanceFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceSetDopplerFactorFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using AudioSourceGetDopplerFactorFn = float(VKE_INTEROP_CDECL *)(uint32_t);

    using AudioListenerSetEnabledFn = void(VKE_INTEROP_CDECL *)(uint32_t, int32_t);
    using AudioListenerGetEnabledFn = int32_t(VKE_INTEROP_CDECL *)(uint32_t);
    using AudioSourceReplayFn = void(VKE_INTEROP_CDECL *)(uint32_t);

    void VKE_INTEROP_CDECL AudioSourcePlay(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceReplay(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceStop(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourcePause(uint32_t entity);
    int32_t VKE_INTEROP_CDECL AudioSourceGetIsPlaying(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetLooping(uint32_t entity, int32_t looping);
    int32_t VKE_INTEROP_CDECL AudioSourceGetLooping(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetVolume(uint32_t entity, float volume);
    float VKE_INTEROP_CDECL AudioSourceGetVolume(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetPitch(uint32_t entity, float pitch);
    float VKE_INTEROP_CDECL AudioSourceGetPitch(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetTime(uint32_t entity, float time);
    float VKE_INTEROP_CDECL AudioSourceGetTime(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetSpatializationEnabled(uint32_t entity, int32_t enabled);
    int32_t VKE_INTEROP_CDECL AudioSourceGetSpatializationEnabled(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetAttenuationModel(uint32_t entity, int32_t model);
    int32_t VKE_INTEROP_CDECL AudioSourceGetAttenuationModel(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetRolloff(uint32_t entity, float factor);
    float VKE_INTEROP_CDECL AudioSourceGetRolloff(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetMinDistance(uint32_t entity, float distance);
    float VKE_INTEROP_CDECL AudioSourceGetMinDistance(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetMaxDistance(uint32_t entity, float distance);
    float VKE_INTEROP_CDECL AudioSourceGetMaxDistance(uint32_t entity);
    void VKE_INTEROP_CDECL AudioSourceSetDopplerFactor(uint32_t entity, float factor);
    float VKE_INTEROP_CDECL AudioSourceGetDopplerFactor(uint32_t entity);
    void VKE_INTEROP_CDECL AudioListenerSetEnabled(uint32_t entity, int32_t enabled);
    int32_t VKE_INTEROP_CDECL AudioListenerGetEnabled(uint32_t entity);
}

#endif
