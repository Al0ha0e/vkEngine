using System;

namespace vkEngine.EngineCore
{
    public sealed unsafe class UIButton : UIImage
    {
        internal UIButton(Canvas canvas, UInt64 handle) : base(canvas, handle) {}

        private NVec4 getColor(int state)
        {
            NVec4 value = default;
            RequireSuccess(NativeFunctionRegistry.Functions.GetUIButtonColor(Canvas.Entity, Handle, state, &value));
            return value;
        }
        private void setColor(int state, NVec4 value) =>
            RequireSuccess(NativeFunctionRegistry.Functions.SetUIButtonColor(Canvas.Entity, Handle, state, &value));

        public NVec4 HoverColor { get => getColor(1); set => setColor(1, value); }
        public NVec4 PressedColor { get => getColor(2); set => setColor(2, value); }
        public NVec4 DisabledColor { get => getColor(3); set => setColor(3, value); }
    }
}
