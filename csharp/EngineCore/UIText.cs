using System;
using System.Text;
using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    public enum TextHorizontalAlignment { Left, Center, Right }
    public enum TextVerticalAlignment { Top, Middle, Bottom }

    [StructLayout(LayoutKind.Sequential)]
    public struct TextLayoutOptions
    {
        public float FontSize;
        public TextHorizontalAlignment HorizontalAlignment;
        public TextVerticalAlignment VerticalAlignment;
        private int wordWrap;
        public bool WordWrap { readonly get => wordWrap != 0; set => wordWrap = value ? 1 : 0; }

        public TextLayoutOptions()
        {
            FontSize = 48;
            HorizontalAlignment = TextHorizontalAlignment.Left;
            VerticalAlignment = TextVerticalAlignment.Top;
            wordWrap = 0;
        }
    }

    public unsafe sealed class UIText : UIRenderableComponent
    {
        internal UIText(Canvas canvas, UInt64 handle) : base(canvas, handle) { }

        public TextLayoutOptions LayoutOptions
        {
            get
            {
                TextLayoutOptions value = default;
                RequireSuccess(NativeFunctionRegistry.Functions.GetUITextLayoutOptions(Canvas.Entity, Handle, &value));
                return value;
            }
            set { RequireSuccess(NativeFunctionRegistry.Functions.SetUITextLayoutOptions(Canvas.Entity, Handle, &value)); }
        }

        public float FontSize
        {
            get => LayoutOptions.FontSize;
            set { var options = LayoutOptions; options.FontSize = value; LayoutOptions = options; }
        }
        public TextHorizontalAlignment HorizontalAlignment
        {
            get => LayoutOptions.HorizontalAlignment;
            set { var options = LayoutOptions; options.HorizontalAlignment = value; LayoutOptions = options; }
        }
        public TextVerticalAlignment VerticalAlignment
        {
            get => LayoutOptions.VerticalAlignment;
            set { var options = LayoutOptions; options.VerticalAlignment = value; LayoutOptions = options; }
        }
        public bool WordWrap
        {
            get => LayoutOptions.WordWrap;
            set { var options = LayoutOptions; options.WordWrap = value; LayoutOptions = options; }
        }

        public NVec2 TextSize
        {
            get
            {
                NVec2 value = default;
                RequireSuccess(NativeFunctionRegistry.Functions.GetUITextSize(Canvas.Entity, Handle, &value));
                return value;
            }
        }

        public NVec2 Measure() => measure(-1);

        public NVec2 Measure(float maxWidth)
        {
            if (!float.IsFinite(maxWidth) || maxWidth < 0)
                throw new ArgumentOutOfRangeException(nameof(maxWidth));
            return measure(maxWidth);
        }

        private NVec2 measure(float maxWidth)
        {
            NVec2 value = default;
            RequireSuccess(NativeFunctionRegistry.Functions.MeasureUIText(Canvas.Entity, Handle, maxWidth, &value));
            return value;
        }

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
