namespace vkEngine.EngineCore
{
    public enum BodyActivation
    {
        Activate = 0,
        DontActivate = 1
    }

    public enum MotionType
    {
        Static = 0,
        Kinematic = 1,
        Dynamic = 2
    }

    public enum MotionQuality
    {
        Discrete = 0,
        LinearCast = 1
    }

    public unsafe sealed class RigidBody
    {
        private const UInt32 InvalidBodyID = UInt32.MaxValue;

        private readonly UInt32 entity;
        private readonly UInt32 bodyID;

        private static delegate* unmanaged[Cdecl]<UInt32, UInt32> getBodyID;
        private static delegate* unmanaged[Cdecl]<UInt32, void> activate;
        private static delegate* unmanaged[Cdecl]<UInt32, void> deactivate;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> isActive;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32, void> setObjectLayer;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32> getObjectLayer;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setRestitution;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getRestitution;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setFriction;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getFriction;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setGravityFactor;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getGravityFactor;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getCenterOfMassPosition;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NQuat*, float, void> moveKinematic;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> setLinearAndAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> getLinearAndAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setLinearVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getLinearVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> addLinearVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> addLinearAndAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> getPointVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, Int32, void> addForce;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, Int32, void> addForceAtPoint;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, Int32, void> addTorque;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, Int32, void> addForceAndTorque;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> addImpulse;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> addImpulseAtPoint;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> addAngularImpulse;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, float, float, float, NVec3*, NVec3*, float, Int32> applyBuoyancyImpulse;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32, Int32, void> setMotionType;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> getMotionType;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32, void> setMotionQuality;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> getMotionQuality;

        public RigidBody(UInt32 entity)
        {
            this.entity = entity;
            bodyID = getBodyID(entity);
            if (bodyID == InvalidBodyID)
                throw new InvalidOperationException($"Entity {entity} does not have a loaded RigidBody component.");
        }

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            getBodyID = functions->GetRigidBodyBodyID;
            activate = functions->ActivateBody;
            deactivate = functions->DeactivateBody;
            isActive = functions->IsBodyActive;
            setObjectLayer = functions->SetBodyObjectLayer;
            getObjectLayer = functions->GetBodyObjectLayer;
            setRestitution = functions->SetBodyRestitution;
            getRestitution = functions->GetBodyRestitution;
            setFriction = functions->SetBodyFriction;
            getFriction = functions->GetBodyFriction;
            setGravityFactor = functions->SetBodyGravityFactor;
            getGravityFactor = functions->GetBodyGravityFactor;
            getCenterOfMassPosition = functions->GetBodyCenterOfMassPosition;
            moveKinematic = functions->MoveKinematicBody;
            setLinearAndAngularVelocity = functions->SetBodyLinearAndAngularVelocity;
            getLinearAndAngularVelocity = functions->GetBodyLinearAndAngularVelocity;
            setLinearVelocity = functions->SetBodyLinearVelocity;
            getLinearVelocity = functions->GetBodyLinearVelocity;
            addLinearVelocity = functions->AddBodyLinearVelocity;
            addLinearAndAngularVelocity = functions->AddBodyLinearAndAngularVelocity;
            setAngularVelocity = functions->SetBodyAngularVelocity;
            getAngularVelocity = functions->GetBodyAngularVelocity;
            getPointVelocity = functions->GetBodyPointVelocity;
            addForce = functions->AddBodyForce;
            addForceAtPoint = functions->AddBodyForceAtPoint;
            addTorque = functions->AddBodyTorque;
            addForceAndTorque = functions->AddBodyForceAndTorque;
            addImpulse = functions->AddBodyImpulse;
            addImpulseAtPoint = functions->AddBodyImpulseAtPoint;
            addAngularImpulse = functions->AddBodyAngularImpulse;
            applyBuoyancyImpulse = functions->ApplyBodyBuoyancyImpulse;
            setMotionType = functions->SetBodyMotionType;
            getMotionType = functions->GetBodyMotionType;
            setMotionQuality = functions->SetBodyMotionQuality;
            getMotionQuality = functions->GetBodyMotionQuality;
        }

        public UInt32 Entity => entity;
        public UInt32 BodyID => bodyID;
        public bool IsActive => isActive(bodyID) != 0;

        public UInt32 ObjectLayer
        {
            get => getObjectLayer(bodyID);
            set => setObjectLayer(bodyID, value);
        }

        public float Restitution
        {
            get => getRestitution(bodyID);
            set => setRestitution(bodyID, value);
        }

        public float Friction
        {
            get => getFriction(bodyID);
            set => setFriction(bodyID, value);
        }

        public float GravityFactor
        {
            get => getGravityFactor(bodyID);
            set => setGravityFactor(bodyID, value);
        }

        public NVec3 CenterOfMassPosition
        {
            get
            {
                NVec3 value = default;
                getCenterOfMassPosition(bodyID, &value);
                return value;
            }
        }

        public NVec3 LinearVelocity
        {
            get
            {
                NVec3 value = default;
                getLinearVelocity(bodyID, &value);
                return value;
            }
            set
            {
                NVec3 nativeValue = value;
                setLinearVelocity(bodyID, &nativeValue);
            }
        }

        public NVec3 AngularVelocity
        {
            get
            {
                NVec3 value = default;
                getAngularVelocity(bodyID, &value);
                return value;
            }
            set
            {
                NVec3 nativeValue = value;
                setAngularVelocity(bodyID, &nativeValue);
            }
        }

        public MotionType MotionType
        {
            get => (MotionType)getMotionType(bodyID);
            set => setMotionType(bodyID, (Int32)value, (Int32)BodyActivation.Activate);
        }

        public MotionQuality MotionQuality
        {
            get => (MotionQuality)getMotionQuality(bodyID);
            set => setMotionQuality(bodyID, (Int32)value);
        }

        public void Activate() => activate(bodyID);
        public void Deactivate() => deactivate(bodyID);

        public void MoveKinematic(NVec3 targetPosition, NQuat targetRotation, float deltaTime)
        {
            NVec3 nativePosition = targetPosition;
            NQuat nativeRotation = targetRotation;
            moveKinematic(bodyID, &nativePosition, &nativeRotation, deltaTime);
        }

        public void SetLinearAndAngularVelocity(NVec3 linearVelocity, NVec3 angularVelocity)
        {
            NVec3 nativeLinear = linearVelocity;
            NVec3 nativeAngular = angularVelocity;
            setLinearAndAngularVelocity(bodyID, &nativeLinear, &nativeAngular);
        }

        public void GetLinearAndAngularVelocity(out NVec3 linearVelocity, out NVec3 angularVelocity)
        {
            NVec3 nativeLinear = default;
            NVec3 nativeAngular = default;
            getLinearAndAngularVelocity(bodyID, &nativeLinear, &nativeAngular);
            linearVelocity = nativeLinear;
            angularVelocity = nativeAngular;
        }

        public void AddLinearVelocity(NVec3 linearVelocity)
        {
            NVec3 nativeValue = linearVelocity;
            addLinearVelocity(bodyID, &nativeValue);
        }

        public void AddLinearAndAngularVelocity(NVec3 linearVelocity, NVec3 angularVelocity)
        {
            NVec3 nativeLinear = linearVelocity;
            NVec3 nativeAngular = angularVelocity;
            addLinearAndAngularVelocity(bodyID, &nativeLinear, &nativeAngular);
        }

        public NVec3 GetPointVelocity(NVec3 point)
        {
            NVec3 nativePoint = point;
            NVec3 velocity = default;
            getPointVelocity(bodyID, &nativePoint, &velocity);
            return velocity;
        }

        public void AddForce(NVec3 force, BodyActivation activation = BodyActivation.Activate)
        {
            NVec3 nativeForce = force;
            addForce(bodyID, &nativeForce, (Int32)activation);
        }

        public void AddForceAtPoint(NVec3 force, NVec3 point, BodyActivation activation = BodyActivation.Activate)
        {
            NVec3 nativeForce = force;
            NVec3 nativePoint = point;
            addForceAtPoint(bodyID, &nativeForce, &nativePoint, (Int32)activation);
        }

        public void AddTorque(NVec3 torque, BodyActivation activation = BodyActivation.Activate)
        {
            NVec3 nativeTorque = torque;
            addTorque(bodyID, &nativeTorque, (Int32)activation);
        }

        public void AddForceAndTorque(NVec3 force, NVec3 torque, BodyActivation activation = BodyActivation.Activate)
        {
            NVec3 nativeForce = force;
            NVec3 nativeTorque = torque;
            addForceAndTorque(bodyID, &nativeForce, &nativeTorque, (Int32)activation);
        }

        public void AddImpulse(NVec3 impulse)
        {
            NVec3 nativeImpulse = impulse;
            addImpulse(bodyID, &nativeImpulse);
        }

        public void AddImpulseAtPoint(NVec3 impulse, NVec3 point)
        {
            NVec3 nativeImpulse = impulse;
            NVec3 nativePoint = point;
            addImpulseAtPoint(bodyID, &nativeImpulse, &nativePoint);
        }

        public void AddAngularImpulse(NVec3 angularImpulse)
        {
            NVec3 nativeImpulse = angularImpulse;
            addAngularImpulse(bodyID, &nativeImpulse);
        }

        public bool ApplyBuoyancyImpulse(NVec3 surfacePosition, NVec3 surfaceNormal, float buoyancy, float linearDrag, float angularDrag, NVec3 fluidVelocity, NVec3 gravity, float deltaTime)
        {
            NVec3 nativeSurfacePosition = surfacePosition;
            NVec3 nativeSurfaceNormal = surfaceNormal;
            NVec3 nativeFluidVelocity = fluidVelocity;
            NVec3 nativeGravity = gravity;
            return applyBuoyancyImpulse(bodyID, &nativeSurfacePosition, &nativeSurfaceNormal, buoyancy, linearDrag, angularDrag, &nativeFluidVelocity, &nativeGravity, deltaTime) != 0;
        }

        public void SetMotionType(MotionType motionType, BodyActivation activation)
        {
            setMotionType(bodyID, (Int32)motionType, (Int32)activation);
        }
    }
}
