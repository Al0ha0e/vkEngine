using System;
using System.Text;

namespace vkEngine.EngineCore
{
    public unsafe sealed class UIText : UIRenderableComponent
    {
        internal UIText(Canvas canvas, UInt64 handle) : base(canvas, handle) {}

        public string Text
        {
            get
            {
                int length = NativeFunctionRegistry.Functions.GetUITextLength(Canvas.Entity, Handle);
                if (length < 0) throw new InvalidOperationException("UI text is invalid.");
                if (length == 0) return string.Empty;
                byte[] bytes = new byte[length];
                fixed (byte* buffer = bytes)
                {
                    int written = NativeFunctionRegistry.Functions.GetUITextText(Canvas.Entity, Handle, buffer, (uint)length);
                    if (written < 0) throw new InvalidOperationException("UI text is invalid.");
                    return Encoding.UTF8.GetString(bytes, 0, written);
                }
            }
            set
            {
                byte[] bytes = Encoding.UTF8.GetBytes(value ?? string.Empty);
                fixed (byte* buffer = bytes)
                    RequireSuccess(NativeFunctionRegistry.Functions.SetUITextText(Canvas.Entity, Handle, buffer, (uint)bytes.Length));
            }
        }

        public NVec4 Color
        {
            get
            {
                NVec4 value = default;
                RequireSuccess(NativeFunctionRegistry.Functions.GetUITextColor(Canvas.Entity, Handle, &value));
                return value;
            }
            set { RequireSuccess(NativeFunctionRegistry.Functions.SetUITextColor(Canvas.Entity, Handle, &value)); }
        }
    }
}
