using System;
using System.Collections.Generic;
using System.Text;

namespace vkEngine.EngineCore
{
    public sealed unsafe class Canvas
    {
        public Canvas(UInt32 entity) { Entity = entity; }
        public UInt32 Entity { get; }
        public IReadOnlyList<UIComponent> Children => GetChildren();

        private static void requireSuccess(int result)
        {
            if (result == 0) throw new InvalidOperationException("Canvas is invalid or the requested operation is not valid.");
        }

        internal UInt64 ParentHandle(UIComponent? parent)
        {
            if (parent != null && parent.Canvas.Entity != Entity)
                throw new ArgumentException("UI nodes must belong to the same Canvas.", nameof(parent));
            return parent?.Handle ?? 0;
        }

        private UIComponent? wrap(UInt64 handle) => NativeFunctionRegistry.Functions.GetUIType(Entity, handle) switch
        {
            1 => new UIContainer(this, handle),
            2 => new UIText(this, handle),
            3 => new UIImage(this, handle),
            4 => new UIButton(this, handle),
            _ => null
        };

        public UIComponent? Get(UInt64 handle) => wrap(handle);

        public UIComponent? Find(string name, UIComponent? parent = null)
        {
            ArgumentNullException.ThrowIfNull(name);
            byte[] bytes = Encoding.UTF8.GetBytes(name);
            fixed (byte* buffer = bytes)
                return wrap(NativeFunctionRegistry.Functions.FindUIChild(Entity, ParentHandle(parent), buffer, (uint)bytes.Length));
        }

        public IReadOnlyList<UIComponent> GetChildren(UIComponent? parent = null)
        {
            UInt64 handle = ParentHandle(parent);
            int count = NativeFunctionRegistry.Functions.GetUIChildCount(Entity, handle);
            if (count < 0) throw new InvalidOperationException("Canvas or UI parent is invalid.");
            var children = new UIComponent[count];
            for (uint i = 0; i < count; ++i)
                children[i] = wrap(NativeFunctionRegistry.Functions.GetUIChild(Entity, handle, i))!;
            return children;
        }

        private UIComponent create(int type, string name, UIComponent? parent)
        {
            ArgumentNullException.ThrowIfNull(name);
            byte[] bytes = Encoding.UTF8.GetBytes(name);
            fixed (byte* buffer = bytes)
            {
                UInt64 handle = NativeFunctionRegistry.Functions.CreateUINode(Entity, ParentHandle(parent), type, buffer, (uint)bytes.Length);
                return wrap(handle) ?? throw new InvalidOperationException("UI creation failed: invalid Canvas/parent or unavailable assets.");
            }
        }

        public UIContainer CreateContainer(string name, UIComponent? parent = null) => (UIContainer)create(1, name, parent);
        public UIText CreateText(string name, UIComponent? parent = null) => (UIText)create(2, name, parent);
        public UIImage CreateImage(string name, UIComponent? parent = null) => (UIImage)create(3, name, parent);
        public UIButton CreateButton(string name, UIComponent? parent = null) => (UIButton)create(4, name, parent);

        public void Remove(UIComponent node)
        {
            ArgumentNullException.ThrowIfNull(node);
            if (node.Canvas.Entity != Entity)
                throw new ArgumentException("UI nodes must belong to the same Canvas.", nameof(node));
            requireSuccess(NativeFunctionRegistry.Functions.RemoveUINode(Entity, node.Handle));
        }

        public NVec2 ReferenceResolution
        {
            get
            {
                NVec2 value = default;
                requireSuccess(NativeFunctionRegistry.Functions.GetCanvasReferenceResolution(Entity, &value));
                return value;
            }
            set { requireSuccess(NativeFunctionRegistry.Functions.SetCanvasReferenceResolution(Entity, &value)); }
        }

        public NVec2 ResolvedSize
        {
            get
            {
                NVec2 value = default;
                requireSuccess(NativeFunctionRegistry.Functions.GetUIResolvedSize(Entity, 0, &value));
                return value;
            }
        }

        public int SortOrder
        {
            get
            {
                int value = 0;
                requireSuccess(NativeFunctionRegistry.Functions.GetCanvasSortOrder(Entity, &value));
                return value;
            }
            set { requireSuccess(NativeFunctionRegistry.Functions.SetCanvasSortOrder(Entity, value)); }
        }
    }
}
