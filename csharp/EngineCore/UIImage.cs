using System;

namespace vkEngine.EngineCore
{
    public unsafe sealed class UIImage : UIRenderableComponent
    {
        internal UIImage(Canvas canvas, UInt64 handle) : base(canvas, handle) {}

        public UInt64 Texture
        {
            get
            {
                UInt64 value = 0;
                RequireSuccess(NativeFunctionRegistry.Functions.GetUIImageTexture(Canvas.Entity, Handle, &value));
                return value;
            }
            set { RequireSuccess(NativeFunctionRegistry.Functions.SetUIImageTexture(Canvas.Entity, Handle, value)); }
        }

        public NVec4 Color
        {
            get
            {
                NVec4 value = default;
                RequireSuccess(NativeFunctionRegistry.Functions.GetUIImageColor(Canvas.Entity, Handle, &value));
                return value;
            }
            set { RequireSuccess(NativeFunctionRegistry.Functions.SetUIImageColor(Canvas.Entity, Handle, &value)); }
        }

        public NVec4 UVRect
        {
            get
            {
                NVec4 value = default;
                RequireSuccess(NativeFunctionRegistry.Functions.GetUIImageUVRect(Canvas.Entity, Handle, &value));
                return value;
            }
            set { RequireSuccess(NativeFunctionRegistry.Functions.SetUIImageUVRect(Canvas.Entity, Handle, &value)); }
        }

        public bool PreserveAspect
        {
            get
            {
                int value = 0;
                RequireSuccess(NativeFunctionRegistry.Functions.GetUIImagePreserveAspect(Canvas.Entity, Handle, &value));
                return value != 0;
            }
            set { RequireSuccess(NativeFunctionRegistry.Functions.SetUIImagePreserveAspect(Canvas.Entity, Handle, value ? 1 : 0)); }
        }
    }
}
