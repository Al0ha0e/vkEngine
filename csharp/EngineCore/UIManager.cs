using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct UIFunctions
    {
        public delegate* unmanaged<UInt32, UInt64, Int32, float, float, void> DispatchPointerEvent;
        public delegate* unmanaged<UInt64, void> UnregisterNodeCallbacks;
    }

    public enum UIPointerEventType { Enter, Exit, Down, Up, Click, Cancel, Count }

    public readonly record struct UIPointerEvent(UIComponent Target, UIPointerEventType Type, NVec2 Position);

    internal static unsafe class UIManager
    {
        // Handles are unique across canvases and scene resets; wrappers share subscriptions.
        private static readonly Dictionary<UInt64, Action<UIPointerEvent>?[]> handlers = new();

        internal static UIFunctions GetFunctions() => new()
        {
            DispatchPointerEvent = &dispatch,
            UnregisterNodeCallbacks = &unregisterNodeCallbacks
        };

        internal static void Add(UIComponent node, UIPointerEventType type, Action<UIPointerEvent> handler)
        {
            if (!node.IsValid) throw new InvalidOperationException("UI node is invalid.");
            if (!handlers.TryGetValue(node.Handle, out var events))
                handlers.Add(node.Handle, events = new Action<UIPointerEvent>?[(int)UIPointerEventType.Count]);
            events[(int)type] += handler;
        }

        internal static void Remove(UInt64 handle, UIPointerEventType type, Action<UIPointerEvent> handler)
        {
            if (!handlers.TryGetValue(handle, out var events)) return;
            events[(int)type] -= handler;
            foreach (var entry in events) if (entry != null) return;
            handlers.Remove(handle);
        }

        [UnmanagedCallersOnly]
        private static void unregisterNodeCallbacks(UInt64 handle) => handlers.Remove(handle);

        internal static void ClearRegistered() => handlers.Clear();

        [UnmanagedCallersOnly]
        private static void dispatch(UInt32 entity, UInt64 handle, int type, float x, float y)
        {
            try
            {
                if (!handlers.TryGetValue(handle, out var events) || events[type] is not { } callback) return;
                var node = new Canvas(entity).Get(handle);
                if (node == null) return;
                var value = new UIPointerEvent(node, (UIPointerEventType)type, new NVec2(x, y));
                foreach (Action<UIPointerEvent> handler in callback.GetInvocationList())
                {
                    if (!node.IsValid) break;
                    try { handler(value); }
                    catch (Exception exception) { Console.Error.WriteLine($"UI {value.Type} handler failed: {exception}"); }
                }
            }
            catch (Exception exception) { Console.Error.WriteLine($"UI event dispatch failed: {exception}"); }
        }
    }
}
