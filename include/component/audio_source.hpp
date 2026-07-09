#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H

#include <memory>
#include <cstdint>
#include <common.hpp>
#include <nlohmann/json.hpp>
#include <miniaudio/miniaudio.h>
#include <asset.hpp>
#include <audio/audio_clip.hpp>
#include <audio/audio_manager.hpp>
#include <logger.hpp>

namespace vke_component
{
    class AudioSource
    {
    public:
        bool playOnStart = true;
        bool looping = false;
        float volume = 1.0f;
        float pitch = 1.0f;
        bool spatializationEnabled = true;
        int attenuationModel = 1; // 0=None, 1=Inverse, 2=Linear, 3=Exponential
        float rolloff = 1.0f;
        float minDistance = 1.0f;
        float maxDistance = 100.0f;
        float dopplerFactor = 1.0f;

        std::shared_ptr<vke_audio::AudioClip> clip;
        ma_sound *sound = nullptr;
        bool soundInitialized = false;

        AudioSource() = default;

        AudioSource(const nlohmann::json &json)
        {
            auto clipHandle = json.value("clip", 0);
            if (clipHandle != 0)
                clip = vke_common::AssetManager::LoadAudioClip(clipHandle);

            playOnStart = json.value("playOnStart", true);
            looping = json.value("looping", false);
            volume = json.value("volume", 1.0f);
            pitch = json.value("pitch", 1.0f);
            spatializationEnabled = json.value("spatializationEnabled", true);
            attenuationModel = json.value("attenuationModel", 1);
            rolloff = json.value("rolloff", 1.0f);
            minDistance = json.value("minDistance", 1.0f);
            maxDistance = json.value("maxDistance", 100.0f);
            dopplerFactor = json.value("dopplerFactor", 1.0f);
        }

        void LoadToEngine(uint32_t entity)
        {
            if (soundInitialized)
                return;

            if (!clip || !clip->IsValid())
            {
                VKE_LOG_WARN("AudioSource::LoadToEngine: invalid clip (asset {})", clip ? clip->handle : 0);
                return;
            }

            ma_engine *engine = vke_audio::AudioManager::GetEngine();
            if (engine == nullptr)
            {
                VKE_LOG_ERROR("AudioSource::LoadToEngine: AudioManager not initialized");
                return;
            }

            sound = new ma_sound;
            ma_uint32 flags = MA_SOUND_FLAG_DECODE;
            if (!spatializationEnabled)
                flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

            ma_result result = ma_sound_init_from_file(engine, clip->path.c_str(), flags, nullptr, nullptr, sound);
            if (result != MA_SUCCESS)
            {
                VKE_LOG_ERROR("AudioSource::LoadToEngine: failed to load sound from '{}' (error {})", clip->path, (int)result);
                delete sound;
                sound = nullptr;
                soundInitialized = false;
                return;
            }

            ma_sound_set_looping(sound, looping);
            ma_sound_set_volume(sound, volume);
            ma_sound_set_pitch(sound, pitch);

            if (spatializationEnabled)
            {
                ma_sound_set_attenuation_model(sound, (ma_attenuation_model)attenuationModel);
                ma_sound_set_rolloff(sound, rolloff);
                ma_sound_set_min_distance(sound, minDistance);
                ma_sound_set_max_distance(sound, maxDistance);
                ma_sound_set_doppler_factor(sound, dopplerFactor);
            }

            if (playOnStart)
                ma_sound_start(sound);

            soundInitialized = true;
        }

        void UnloadFromEngine()
        {
            if (soundInitialized && sound != nullptr)
            {
                ma_sound_uninit(sound);
                delete sound;
                sound = nullptr;
                soundInitialized = false;
            }
        }

        nlohmann::json ToJSON()
        {
            return {
                {"type", "audioSource"},
                {"clip", clip ? clip->handle : 0},
                {"playOnStart", playOnStart},
                {"looping", looping},
                {"volume", volume},
                {"pitch", pitch},
                {"spatializationEnabled", spatializationEnabled},
                {"attenuationModel", attenuationModel},
                {"rolloff", rolloff},
                {"minDistance", minDistance},
                {"maxDistance", maxDistance},
                {"dopplerFactor", dopplerFactor}};
        }

        void Play()
        {
            if (soundInitialized && sound != nullptr)
                ma_sound_start(sound);
        }

        void Replay()
        {
            if (soundInitialized && sound != nullptr)
            {
                ma_sound_stop(sound);
                ma_sound_seek_to_pcm_frame(sound, 0);
                ma_sound_start(sound);
            }
        }

        void Stop()
        {
            if (soundInitialized && sound != nullptr)
            {
                ma_sound_stop(sound);
                ma_sound_seek_to_pcm_frame(sound, 0);
            }
        }

        void Pause()
        {
            if (soundInitialized && sound != nullptr)
                ma_sound_stop(sound);
        }

        bool IsPlaying() const
        {
            return soundInitialized && sound != nullptr && ma_sound_is_playing(sound);
        }

        void SetLooping(bool loop)
        {
            looping = loop;
            if (soundInitialized && sound != nullptr)
                ma_sound_set_looping(sound, loop);
        }

        void SetVolume(float vol)
        {
            volume = vol;
            if (soundInitialized && sound != nullptr)
                ma_sound_set_volume(sound, vol);
        }

        void SetPitch(float p)
        {
            pitch = p;
            if (soundInitialized && sound != nullptr)
                ma_sound_set_pitch(sound, p);
        }

        void SetSpatializationEnabled(bool enabled)
        {
            spatializationEnabled = enabled;
            if (soundInitialized && sound != nullptr)
                ma_sound_set_spatialization_enabled(sound, enabled);
        }

        void SetAttenuationModel(int model)
        {
            attenuationModel = model;
            if (soundInitialized && sound != nullptr)
                ma_sound_set_attenuation_model(sound, (ma_attenuation_model)model);
        }

        void SetRolloff(float factor)
        {
            rolloff = factor;
            if (soundInitialized && sound != nullptr)
                ma_sound_set_rolloff(sound, factor);
        }

        void SetMinDistance(float distance)
        {
            minDistance = distance;
            if (soundInitialized && sound != nullptr)
                ma_sound_set_min_distance(sound, distance);
        }

        void SetMaxDistance(float distance)
        {
            maxDistance = distance;
            if (soundInitialized && sound != nullptr)
                ma_sound_set_max_distance(sound, distance);
        }

        void SetDopplerFactor(float factor)
        {
            dopplerFactor = factor;
            if (soundInitialized && sound != nullptr)
                ma_sound_set_doppler_factor(sound, factor);
        }

        float GetTime() const
        {
            if (!soundInitialized || sound == nullptr)
                return 0.0f;

            ma_uint64 frames;
            if (ma_sound_get_cursor_in_pcm_frames(sound, &frames) == MA_SUCCESS)
            {
                ma_engine *eng = ma_sound_get_engine(sound);
                if (eng == nullptr)
                    return 0.0f;
                float sampleRate = (float)ma_engine_get_sample_rate(eng);
                return sampleRate > 0.0f ? (float)frames / sampleRate : 0.0f;
            }
            return 0.0f;
        }

        void SetTime(float seconds)
        {
            if (!soundInitialized || sound == nullptr || seconds < 0.0f)
                return;

            ma_engine *eng = ma_sound_get_engine(sound);
            if (eng == nullptr)
                return;
            float sampleRate = (float)ma_engine_get_sample_rate(eng);
            if (sampleRate > 0.0f)
            {
                ma_uint64 frame = (ma_uint64)(seconds * sampleRate);
                ma_sound_seek_to_pcm_frame(sound, frame);
            }
        }
    };
}

#endif
