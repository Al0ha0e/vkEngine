using System;
using System.Text;

namespace vkEngine.EngineCore
{
    public unsafe sealed class UIText
    {
        private readonly UInt32 entity;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32> getLength;
        private static delegate* unmanaged[Cdecl]<UInt32, byte*, UInt32, UInt32> getText;
        private static delegate* unmanaged[Cdecl]<UInt32, byte*, UInt32, void> setText;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec4*, void> getColor;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec4*, void> setColor;

        public UIText(UInt32 entity)
        {
            this.entity = entity;
        }

        public UInt32 Entity => entity;

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            getLength = functions->GetUITextLength;
            getText = functions->GetUITextText;
            setText = functions->SetUITextText;
            getColor = functions->GetUITextColor;
            setColor = functions->SetUITextColor;
        }

        public string Text
        {
            get
            {
                UInt32 byteLength = getLength(Entity);
                if (byteLength == 0)
                    return string.Empty;

                byte[] bytes = new byte[byteLength];
                fixed (byte* buffer = bytes)
                {
                    UInt32 written = getText(Entity, buffer, byteLength);
                    return Encoding.UTF8.GetString(bytes, 0, (int)written);
                }
            }
            set
            {
                byte[] bytes = Encoding.UTF8.GetBytes(value ?? string.Empty);
                fixed (byte* buffer = bytes)
                {
                    setText(Entity, buffer, (UInt32)bytes.Length);
                }
            }
        }

        public NVec4 Color
        {
            get
            {
                NVec4 value = default;
                getColor(Entity, &value);
                return value;
            }
            set
            {
                NVec4 nativeValue = value;
                setColor(Entity, &nativeValue);
            }
        }
    }
}
