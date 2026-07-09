using System;
using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    public unsafe sealed class AudioListener
    {
        private readonly UInt32 entity;

        private static delegate* unmanaged[Cdecl]<UInt32, Int32, void> setEnabled;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> getEnabled;

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            setEnabled = functions->AudioListenerSetEnabled;
            getEnabled = functions->AudioListenerGetEnabled;
        }

        public AudioListener(UInt32 entity)
        {
            this.entity = entity;
        }

        public bool Enabled
        {
            get => getEnabled(entity) != 0;
            set => setEnabled(entity, value ? 1 : 0);
        }
    }
}
