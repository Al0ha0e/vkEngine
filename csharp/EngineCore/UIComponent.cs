using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    [StructLayout(LayoutKind.Sequential)]
    public struct RectTransform
    {
        public NVec2 AnchorMin, AnchorMax, OffsetMin, OffsetMax, Pivot, Scale;
        public float Rotation;

        public RectTransform()
        {
            AnchorMin = AnchorMax = OffsetMin = OffsetMax = NVec2.Zero;
            Pivot = new NVec2(0.5f, 0.5f);
            Scale = new NVec2(1, 1);
            Rotation = 0;
        }
    }

    public abstract unsafe class UIComponent
    {
        internal UIComponent(Canvas canvas, UInt64 handle) { Canvas = canvas; Handle = handle; }
        public Canvas Canvas { get; }
        public UInt64 Handle { get; }
        public bool IsValid => NativeFunctionRegistry.Functions.GetUIType(Canvas.Entity, Handle) != 0;
        public IReadOnlyList<UIComponent> Children => Canvas.GetChildren(this);

        protected static void RequireSuccess(int result)
        {
            if (result == 0) throw new InvalidOperationException("UI node is invalid or the requested operation is not valid.");
        }

        public RectTransform RectTransform
        {
            get
            {
                RectTransform value = default;
                RequireSuccess(NativeFunctionRegistry.Functions.GetUIRectTransform(Canvas.Entity, Handle, &value));
                return value;
            }
            set { RequireSuccess(NativeFunctionRegistry.Functions.SetUIRectTransform(Canvas.Entity, Handle, &value)); }
        }

        public NVec2 ResolvedSize
        {
            get
            {
                NVec2 value = default;
                RequireSuccess(NativeFunctionRegistry.Functions.GetUIResolvedSize(Canvas.Entity, Handle, &value));
                return value;
            }
        }

        public void Remove() => RequireSuccess(NativeFunctionRegistry.Functions.RemoveUINode(Canvas.Entity, Handle));
        public void SetParent(UIComponent? parent) =>
            RequireSuccess(NativeFunctionRegistry.Functions.SetUIParent(Canvas.Entity, Handle, Canvas.ParentHandle(parent)));
        public void SetSiblingIndex(int index)
        {
            if (index < 0) throw new ArgumentOutOfRangeException(nameof(index));
            RequireSuccess(NativeFunctionRegistry.Functions.SetUISiblingIndex(Canvas.Entity, Handle, (uint)index));
        }
    }

    public sealed class UIContainer : UIComponent
    {
        internal UIContainer(Canvas canvas, UInt64 handle) : base(canvas, handle) {}
    }

    public abstract class UIRenderableComponent : UIComponent
    {
        internal UIRenderableComponent(Canvas canvas, UInt64 handle) : base(canvas, handle) {}
    }
}
