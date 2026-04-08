using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct NativeFunctions
    {
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> GetTransformLocalPosition;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> SetTransformLocalPosition;
        public delegate* unmanaged[Cdecl]<UInt32, NQuat*, void> GetTransformLocalRotation;
        public delegate* unmanaged[Cdecl]<UInt32, NQuat*, void> SetTransformLocalRotation;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> GetTransformLocalScale;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> SetTransformLocalScale;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> TranslateTransformLocal;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> TranslateTransformGlobal;
        public delegate* unmanaged[Cdecl]<UInt32, float, NVec3*, void> RotateTransformLocal;
        public delegate* unmanaged[Cdecl]<UInt32, float, NVec3*, void> RotateTransformGlobal;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> ScaleTransform;
        public delegate* unmanaged[Cdecl]<Int32, Int32> IsKeyDown;
        public delegate* unmanaged[Cdecl]<Int32, Int32> IsKeyPressed;
        public delegate* unmanaged[Cdecl]<Int32, Int32> IsKeyReleased;
        public delegate* unmanaged[Cdecl]<Int32, Int32> IsMouseButtonDown;
        public delegate* unmanaged[Cdecl]<Int32, Int32> IsMouseButtonPressed;
        public delegate* unmanaged[Cdecl]<Int32, Int32> IsMouseButtonReleased;
        public delegate* unmanaged[Cdecl]<NVec2*, void> GetMousePosition;
        public delegate* unmanaged[Cdecl]<NVec2*, void> GetMouseDelta;
        public delegate* unmanaged[Cdecl]<NVec2*, void> GetMouseScrollDelta;
        public delegate* unmanaged[Cdecl]<Int32, void> SetCursorMode;
        public delegate* unmanaged[Cdecl]<Int32> GetCursorMode;
        public delegate* unmanaged[Cdecl]<float> GetTime;
        public delegate* unmanaged[Cdecl]<float> GetDeltaTime;
        public delegate* unmanaged[Cdecl]<float> GetPreviousFrameTime;
        public delegate* unmanaged[Cdecl]<Int32, void> SetEngineState;
    }

    public static class NativeFunctionRegistry
    {
        public static unsafe NativeFunctions Functions { get; private set; }
        public static bool IsRegistered { get; private set; }

        public static unsafe void Register(NativeFunctions* functions)
        {
            Functions = *functions;
            IsRegistered = true;
            Transform.RegisterNativeFunctions(functions);
            Input.RegisterNativeFunctions(functions);
            Time.RegisterNativeFunctions(functions);
            EngineStateManager.RegisterNativeFunctions(functions);
        }
    }
}
