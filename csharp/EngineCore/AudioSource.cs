using System;
using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    public unsafe sealed class AudioSource
    {
        private readonly UInt32 entity;

        private static delegate* unmanaged[Cdecl]<UInt32, void> play;
        private static delegate* unmanaged[Cdecl]<UInt32, void> replay;
        private static delegate* unmanaged[Cdecl]<UInt32, void> stop;
        private static delegate* unmanaged[Cdecl]<UInt32, void> pause;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> getIsPlaying;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32, void> setLooping;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> getLooping;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setVolume;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getVolume;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setPitch;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getPitch;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setTime;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getTime;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32, void> setSpatializationEnabled;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> getSpatializationEnabled;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32, void> setAttenuationModel;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> getAttenuationModel;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setRolloff;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getRolloff;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setMinDistance;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getMinDistance;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setMaxDistance;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getMaxDistance;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setDopplerFactor;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getDopplerFactor;

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            play = functions->AudioSourcePlay;
            replay = functions->AudioSourceReplay;
            stop = functions->AudioSourceStop;
            pause = functions->AudioSourcePause;
            getIsPlaying = functions->AudioSourceGetIsPlaying;
            setLooping = functions->AudioSourceSetLooping;
            getLooping = functions->AudioSourceGetLooping;
            setVolume = functions->AudioSourceSetVolume;
            getVolume = functions->AudioSourceGetVolume;
            setPitch = functions->AudioSourceSetPitch;
            getPitch = functions->AudioSourceGetPitch;
            setTime = functions->AudioSourceSetTime;
            getTime = functions->AudioSourceGetTime;
            setSpatializationEnabled = functions->AudioSourceSetSpatializationEnabled;
            getSpatializationEnabled = functions->AudioSourceGetSpatializationEnabled;
            setAttenuationModel = functions->AudioSourceSetAttenuationModel;
            getAttenuationModel = functions->AudioSourceGetAttenuationModel;
            setRolloff = functions->AudioSourceSetRolloff;
            getRolloff = functions->AudioSourceGetRolloff;
            setMinDistance = functions->AudioSourceSetMinDistance;
            getMinDistance = functions->AudioSourceGetMinDistance;
            setMaxDistance = functions->AudioSourceSetMaxDistance;
            getMaxDistance = functions->AudioSourceGetMaxDistance;
            setDopplerFactor = functions->AudioSourceSetDopplerFactor;
            getDopplerFactor = functions->AudioSourceGetDopplerFactor;
        }

        public AudioSource(UInt32 entity)
        {
            this.entity = entity;
        }

        public void Play() => play(entity);
        public void Replay() => replay(entity);
        public void Stop() => stop(entity);
        public void Pause() => pause(entity);

        public bool IsPlaying => getIsPlaying(entity) != 0;
        public bool IsLooping
        {
            get => getLooping(entity) != 0;
            set => setLooping(entity, value ? 1 : 0);
        }

        public float Volume
        {
            get => getVolume(entity);
            set => setVolume(entity, value);
        }

        public float Pitch
        {
            get => getPitch(entity);
            set => setPitch(entity, value);
        }

        public float Time
        {
            get => getTime(entity);
            set => setTime(entity, value);
        }

        public bool SpatializationEnabled
        {
            get => getSpatializationEnabled(entity) != 0;
            set => setSpatializationEnabled(entity, value ? 1 : 0);
        }

        public int AttenuationModel
        {
            get => getAttenuationModel(entity);
            set => setAttenuationModel(entity, value);
        }

        public float Rolloff
        {
            get => getRolloff(entity);
            set => setRolloff(entity, value);
        }

        public float MinDistance
        {
            get => getMinDistance(entity);
            set => setMinDistance(entity, value);
        }

        public float MaxDistance
        {
            get => getMaxDistance(entity);
            set => setMaxDistance(entity, value);
        }

        public float DopplerFactor
        {
            get => getDopplerFactor(entity);
            set => setDopplerFactor(entity, value);
        }
    }
}
