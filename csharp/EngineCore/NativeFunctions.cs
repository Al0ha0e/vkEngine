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
        public delegate* unmanaged[Cdecl]<UInt32, Int32, Int32> HasComponent;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> GetDirectionalLightColor;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> SetDirectionalLightColor;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetDirectionalLightIntensity;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetDirectionalLightIntensity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> GetPointLightColor;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> SetPointLightColor;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetPointLightIntensity;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetPointLightIntensity;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetPointLightRadius;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetPointLightRadius;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> GetSpotLightColor;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> SetSpotLightColor;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetSpotLightIntensity;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetSpotLightIntensity;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetSpotLightRadius;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetSpotLightRadius;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetSpotLightInnerCone;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetSpotLightInnerCone;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetSpotLightOuterCone;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetSpotLightOuterCone;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> SetCharacterControllerVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> GetCharacterControllerVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, Int32> IsCharacterControllerGrounded;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32> GetRigidBodyBodyID;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32> GetSensorBodyID;
        public delegate* unmanaged[Cdecl]<UInt32, void> ActivateBody;
        public delegate* unmanaged[Cdecl]<UInt32, void> DeactivateBody;
        public delegate* unmanaged[Cdecl]<UInt32, Int32> IsBodyActive;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32, void> SetBodyObjectLayer;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32> GetBodyObjectLayer;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetBodyRestitution;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetBodyRestitution;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetBodyFriction;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetBodyFriction;
        public delegate* unmanaged[Cdecl]<UInt32, float, void> SetBodyGravityFactor;
        public delegate* unmanaged[Cdecl]<UInt32, float> GetBodyGravityFactor;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> GetBodyCenterOfMassPosition;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, NQuat*, float, void> MoveKinematicBody;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> SetBodyLinearAndAngularVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> GetBodyLinearAndAngularVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> SetBodyLinearVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> GetBodyLinearVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> AddBodyLinearVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> AddBodyLinearAndAngularVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> SetBodyAngularVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> GetBodyAngularVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> GetBodyPointVelocity;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, Int32, void> AddBodyForce;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, Int32, void> AddBodyForceAtPoint;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, Int32, void> AddBodyTorque;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, Int32, void> AddBodyForceAndTorque;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> AddBodyImpulse;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> AddBodyImpulseAtPoint;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> AddBodyAngularImpulse;
        public delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, float, float, float, NVec3*, NVec3*, float, Int32> ApplyBodyBuoyancyImpulse;
        public delegate* unmanaged[Cdecl]<UInt32, Int32, Int32, void> SetBodyMotionType;
        public delegate* unmanaged[Cdecl]<UInt32, Int32> GetBodyMotionType;
        public delegate* unmanaged[Cdecl]<UInt32, Int32, void> SetBodyMotionQuality;
        public delegate* unmanaged[Cdecl]<UInt32, Int32> GetBodyMotionQuality;
        public delegate* unmanaged[Cdecl]<UInt32> GetPhysicsObjectLayerCount;
        public delegate* unmanaged[Cdecl]<UInt32> GetPhysicsBroadPhaseLayerCount;
        public delegate* unmanaged[Cdecl]<byte*, UInt32, UInt32> AddPhysicsObjectLayer;
        public delegate* unmanaged[Cdecl]<byte*, UInt32> AddPhysicsBroadPhaseLayer;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32, void> SetPhysicsObjectLayerBroadPhaseLayer;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32> GetPhysicsObjectLayerBroadPhaseLayer;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32, Int32, void> SetPhysicsObjectLayerCollision;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32, Int32> GetPhysicsObjectLayerCollision;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32, Int32, void> SetPhysicsObjectVsBroadPhaseLayerCollision;
        public delegate* unmanaged[Cdecl]<UInt32, UInt32, Int32> GetPhysicsObjectVsBroadPhaseLayerCollision;
        public delegate* unmanaged[Cdecl]<UInt32> GetPhysicsContactEventCount;
        public delegate* unmanaged[Cdecl]<NativeContactEvent*, UInt32, UInt32> GetPhysicsContactEvents;
        public delegate* unmanaged[Cdecl]<NVec3*, NVec3*, float, UInt32, UInt32, RaycastHit*, Int32> PhysicsRaycast;
        public delegate* unmanaged[Cdecl]<NVec3*, NVec3*, float, UInt32, UInt32, RaycastHit*, UInt32, UInt32> PhysicsRaycastAll;
        public delegate* unmanaged[Cdecl]<NVec3*, UInt32, UInt32, CollidePointHit*, UInt32, UInt32> PhysicsCollidePoint;
        public delegate* unmanaged[Cdecl]<Int32, void*, NVec3*, NQuat*, NVec3*, UInt32, UInt32, CollideShapeHit*, UInt32, UInt32> PhysicsCollideShape;
        public delegate* unmanaged[Cdecl]<Int32, void*, NVec3*, NQuat*, NVec3*, NVec3*, float, UInt32, UInt32, ShapeCastHit*, UInt32, UInt32> PhysicsCastShape;
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
            EntityScript.RegisterNativeFunctions(functions);
            Light.RegisterNativeFunctions(functions);
            CharacterController.RegisterNativeFunctions(functions);
            RigidBody.RegisterNativeFunctions(functions);
            Sensor.RegisterNativeFunctions(functions);
            Physics.RegisterNativeFunctions(functions);
        }
    }
}
