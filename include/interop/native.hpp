#ifndef INTEROP_NATIVE_H
#define INTEROP_NATIVE_H

#include <cstdint>
#include <interop/interop.hpp>
#include <interop/math.hpp>
#include <interop/light.hpp>
#include <interop/physics.hpp>
#include <interop/text.hpp>

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
    using HasComponentFn = int32_t(VKE_INTEROP_CDECL *)(uint32_t, int32_t);
    using SetCharacterControllerVelocityFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector3<float> *);
    using GetCharacterControllerVelocityFn = void(VKE_INTEROP_CDECL *)(uint32_t, Vector3<float> *);
    using IsCharacterControllerGroundedFn = int32_t(VKE_INTEROP_CDECL *)(uint32_t);

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
        HasComponentFn HasComponent;
        GetUITextLengthFn GetUITextLength;
        GetUITextTextFn GetUITextText;
        SetUITextTextFn SetUITextText;
        GetUITextColorFn GetUITextColor;
        SetUITextColorFn SetUITextColor;
        GetDirectionalLightColorFn GetDirectionalLightColor;
        SetDirectionalLightColorFn SetDirectionalLightColor;
        GetDirectionalLightIntensityFn GetDirectionalLightIntensity;
        SetDirectionalLightIntensityFn SetDirectionalLightIntensity;
        GetPointLightColorFn GetPointLightColor;
        SetPointLightColorFn SetPointLightColor;
        GetPointLightIntensityFn GetPointLightIntensity;
        SetPointLightIntensityFn SetPointLightIntensity;
        GetPointLightRadiusFn GetPointLightRadius;
        SetPointLightRadiusFn SetPointLightRadius;
        GetSpotLightColorFn GetSpotLightColor;
        SetSpotLightColorFn SetSpotLightColor;
        GetSpotLightIntensityFn GetSpotLightIntensity;
        SetSpotLightIntensityFn SetSpotLightIntensity;
        GetSpotLightRadiusFn GetSpotLightRadius;
        SetSpotLightRadiusFn SetSpotLightRadius;
        GetSpotLightInnerConeFn GetSpotLightInnerCone;
        SetSpotLightInnerConeFn SetSpotLightInnerCone;
        GetSpotLightOuterConeFn GetSpotLightOuterCone;
        SetSpotLightOuterConeFn SetSpotLightOuterCone;
        SetCharacterControllerVelocityFn SetCharacterControllerVelocity;
        GetCharacterControllerVelocityFn GetCharacterControllerVelocity;
        IsCharacterControllerGroundedFn IsCharacterControllerGrounded;
        GetRigidBodyBodyIDFn GetRigidBodyBodyID;
        GetSensorBodyIDFn GetSensorBodyID;
        ActivateBodyFn ActivateBody;
        DeactivateBodyFn DeactivateBody;
        IsBodyActiveFn IsBodyActive;
        SetBodyObjectLayerFn SetBodyObjectLayer;
        GetBodyObjectLayerFn GetBodyObjectLayer;
        SetBodyRestitutionFn SetBodyRestitution;
        GetBodyRestitutionFn GetBodyRestitution;
        SetBodyFrictionFn SetBodyFriction;
        GetBodyFrictionFn GetBodyFriction;
        SetBodyGravityFactorFn SetBodyGravityFactor;
        GetBodyGravityFactorFn GetBodyGravityFactor;
        GetBodyCenterOfMassPositionFn GetBodyCenterOfMassPosition;
        MoveKinematicBodyFn MoveKinematicBody;
        SetBodyLinearAndAngularVelocityFn SetBodyLinearAndAngularVelocity;
        GetBodyLinearAndAngularVelocityFn GetBodyLinearAndAngularVelocity;
        SetBodyLinearVelocityFn SetBodyLinearVelocity;
        GetBodyLinearVelocityFn GetBodyLinearVelocity;
        AddBodyLinearVelocityFn AddBodyLinearVelocity;
        AddBodyLinearAndAngularVelocityFn AddBodyLinearAndAngularVelocity;
        SetBodyAngularVelocityFn SetBodyAngularVelocity;
        GetBodyAngularVelocityFn GetBodyAngularVelocity;
        GetBodyPointVelocityFn GetBodyPointVelocity;
        AddBodyForceFn AddBodyForce;
        AddBodyForceAtPointFn AddBodyForceAtPoint;
        AddBodyTorqueFn AddBodyTorque;
        AddBodyForceAndTorqueFn AddBodyForceAndTorque;
        AddBodyImpulseFn AddBodyImpulse;
        AddBodyImpulseAtPointFn AddBodyImpulseAtPoint;
        AddBodyAngularImpulseFn AddBodyAngularImpulse;
        ApplyBodyBuoyancyImpulseFn ApplyBodyBuoyancyImpulse;
        SetBodyMotionTypeFn SetBodyMotionType;
        GetBodyMotionTypeFn GetBodyMotionType;
        SetBodyMotionQualityFn SetBodyMotionQuality;
        GetBodyMotionQualityFn GetBodyMotionQuality;
        GetPhysicsObjectLayerCountFn GetPhysicsObjectLayerCount;
        GetPhysicsBroadPhaseLayerCountFn GetPhysicsBroadPhaseLayerCount;
        AddPhysicsObjectLayerFn AddPhysicsObjectLayer;
        AddPhysicsBroadPhaseLayerFn AddPhysicsBroadPhaseLayer;
        SetPhysicsObjectLayerBroadPhaseLayerFn SetPhysicsObjectLayerBroadPhaseLayer;
        GetPhysicsObjectLayerBroadPhaseLayerFn GetPhysicsObjectLayerBroadPhaseLayer;
        SetPhysicsObjectLayerCollisionFn SetPhysicsObjectLayerCollision;
        GetPhysicsObjectLayerCollisionFn GetPhysicsObjectLayerCollision;
        SetPhysicsObjectVsBroadPhaseLayerCollisionFn SetPhysicsObjectVsBroadPhaseLayerCollision;
        GetPhysicsObjectVsBroadPhaseLayerCollisionFn GetPhysicsObjectVsBroadPhaseLayerCollision;
        GetPhysicsContactEventCountFn GetPhysicsContactEventCount;
        GetPhysicsContactEventsFn GetPhysicsContactEvents;
        PhysicsRaycastFn PhysicsRaycast;
        PhysicsRaycastAllFn PhysicsRaycastAll;
        PhysicsCollidePointFn PhysicsCollidePoint;
        PhysicsCollideShapeFn PhysicsCollideShape;
        PhysicsCastShapeFn PhysicsCastShape;
    };
}

#endif
