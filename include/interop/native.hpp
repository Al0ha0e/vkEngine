#ifndef INTEROP_NATIVE_H
#define INTEROP_NATIVE_H

#include <cstdint>
#include <interop/interop.hpp>
#include <interop/math.hpp>

namespace vke_interop
{
    using GetTransformLocalPositionFn = void(VKE_INTEROP_CDECL *)(uint32_t, Vector3<float> *);
    using SetTransformLocalPositionFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector3<float> *);
    using GetTransformLocalRotationFn = void(VKE_INTEROP_CDECL *)(uint32_t, Quaternion<float> *);
    using SetTransformLocalRotationFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Quaternion<float> *);
    using GetTransformLocalScaleFn = void(VKE_INTEROP_CDECL *)(uint32_t, Vector3<float> *);
    using SetTransformLocalScaleFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector3<float> *);
    using TranslateTransformLocalFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector3<float> *);
    using TranslateTransformGlobalFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector3<float> *);
    using RotateTransformLocalFn = void(VKE_INTEROP_CDECL *)(uint32_t, float, const Vector3<float> *);
    using RotateTransformGlobalFn = void(VKE_INTEROP_CDECL *)(uint32_t, float, const Vector3<float> *);
    using ScaleTransformFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector3<float> *);
    using IsKeyDownFn = int32_t(VKE_INTEROP_CDECL *)(int32_t);
    using IsKeyPressedFn = int32_t(VKE_INTEROP_CDECL *)(int32_t);
    using IsKeyReleasedFn = int32_t(VKE_INTEROP_CDECL *)(int32_t);
    using IsMouseButtonDownFn = int32_t(VKE_INTEROP_CDECL *)(int32_t);
    using IsMouseButtonPressedFn = int32_t(VKE_INTEROP_CDECL *)(int32_t);
    using IsMouseButtonReleasedFn = int32_t(VKE_INTEROP_CDECL *)(int32_t);
    using GetMousePositionFn = void(VKE_INTEROP_CDECL *)(Vector2<float> *);
    using GetMouseDeltaFn = void(VKE_INTEROP_CDECL *)(Vector2<float> *);
    using GetMouseScrollDeltaFn = void(VKE_INTEROP_CDECL *)(Vector2<float> *);
    using SetCursorModeFn = void(VKE_INTEROP_CDECL *)(int32_t);
    using GetCursorModeFn = int32_t(VKE_INTEROP_CDECL *)();
    using GetTimeFn = float(VKE_INTEROP_CDECL *)();
    using GetDeltaTimeFn = float(VKE_INTEROP_CDECL *)();
    using GetPreviousFrameTimeFn = float(VKE_INTEROP_CDECL *)();
    using SetEngineStateFn = void(VKE_INTEROP_CDECL *)(int32_t);

    struct NativeFunctions
    {
        GetTransformLocalPositionFn GetTransformLocalPosition;
        SetTransformLocalPositionFn SetTransformLocalPosition;
        GetTransformLocalRotationFn GetTransformLocalRotation;
        SetTransformLocalRotationFn SetTransformLocalRotation;
        GetTransformLocalScaleFn GetTransformLocalScale;
        SetTransformLocalScaleFn SetTransformLocalScale;
        TranslateTransformLocalFn TranslateTransformLocal;
        TranslateTransformGlobalFn TranslateTransformGlobal;
        RotateTransformLocalFn RotateTransformLocal;
        RotateTransformGlobalFn RotateTransformGlobal;
        ScaleTransformFn ScaleTransform;
        IsKeyDownFn IsKeyDown;
        IsKeyPressedFn IsKeyPressed;
        IsKeyReleasedFn IsKeyReleased;
        IsMouseButtonDownFn IsMouseButtonDown;
        IsMouseButtonPressedFn IsMouseButtonPressed;
        IsMouseButtonReleasedFn IsMouseButtonReleased;
        GetMousePositionFn GetMousePosition;
        GetMouseDeltaFn GetMouseDelta;
        GetMouseScrollDeltaFn GetMouseScrollDelta;
        SetCursorModeFn SetCursorMode;
        GetCursorModeFn GetCursorMode;
        GetTimeFn GetTime;
        GetDeltaTimeFn GetDeltaTime;
        GetPreviousFrameTimeFn GetPreviousFrameTime;
        SetEngineStateFn SetEngineState;
    };
}

#endif
