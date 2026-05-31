using System;
using System.Collections.Generic;

namespace vkEngine.EngineCore
{
    public unsafe sealed class Sensor
    {
        private const UInt32 InvalidBodyID = UInt32.MaxValue;
        private static readonly Dictionary<UInt32, ContactCallbacks> registeredCallbacks = new();

        private readonly UInt32 entity;
        private readonly UInt32 bodyID;
        private readonly ContactCallbacks contactCallbacks;

        private static delegate* unmanaged[Cdecl]<UInt32, UInt32> getBodyID;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32, void> setObjectLayer;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32> getObjectLayer;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> setLinearAndAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> getLinearAndAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setLinearVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getLinearVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> addLinearVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> addLinearAndAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getAngularVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, NVec3*, void> getPointVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32, Int32, void> setMotionType;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> getMotionType;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32, void> setMotionQuality;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> getMotionQuality;

        public Sensor(UInt32 entity)
        {
            this.entity = entity;
            bodyID = getBodyID(entity);
            if (bodyID == InvalidBodyID)
                throw new InvalidOperationException($"Entity {entity} does not have a loaded Sensor component.");

            contactCallbacks = GetOrCreateCallbacks(bodyID);
        }

        public Sensor(UInt32 entity, UInt32 bodyID)
        {
            if (bodyID == InvalidBodyID)
                throw new ArgumentOutOfRangeException(nameof(bodyID), $"Entity {entity} does not have a valid Sensor body ID.");

            this.entity = entity;
            this.bodyID = bodyID;
            contactCallbacks = GetOrCreateCallbacks(bodyID);
        }

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            getBodyID = functions->GetSensorBodyID;
            setObjectLayer = functions->SetBodyObjectLayer;
            getObjectLayer = functions->GetBodyObjectLayer;
            setLinearAndAngularVelocity = functions->SetBodyLinearAndAngularVelocity;
            getLinearAndAngularVelocity = functions->GetBodyLinearAndAngularVelocity;
            setLinearVelocity = functions->SetBodyLinearVelocity;
            getLinearVelocity = functions->GetBodyLinearVelocity;
            addLinearVelocity = functions->AddBodyLinearVelocity;
            addLinearAndAngularVelocity = functions->AddBodyLinearAndAngularVelocity;
            setAngularVelocity = functions->SetBodyAngularVelocity;
            getAngularVelocity = functions->GetBodyAngularVelocity;
            getPointVelocity = functions->GetBodyPointVelocity;
            setMotionType = functions->SetBodyMotionType;
            getMotionType = functions->GetBodyMotionType;
            setMotionQuality = functions->SetBodyMotionQuality;
            getMotionQuality = functions->GetBodyMotionQuality;
        }

        public UInt32 Entity => entity;
        public UInt32 BodyID => bodyID;

        public Action<ContactEvent>? ContactAdded
        {
            get => contactCallbacks.ContactAdded;
            set => contactCallbacks.ContactAdded = value;
        }

        public Action<ContactEvent>? ContactPersisted
        {
            get => contactCallbacks.ContactPersisted;
            set => contactCallbacks.ContactPersisted = value;
        }

        public Action<ContactEvent>? ContactRemoved
        {
            get => contactCallbacks.ContactRemoved;
            set => contactCallbacks.ContactRemoved = value;
        }

        internal static bool TryDispatchContactEvent(UInt32 bodyID, ContactEvent contactEvent)
        {
            if (!registeredCallbacks.TryGetValue(bodyID, out ContactCallbacks? callbacks))
                return false;

            callbacks.Dispatch(contactEvent);
            return true;
        }

        private static ContactCallbacks GetOrCreateCallbacks(UInt32 bodyID)
        {
            if (!registeredCallbacks.TryGetValue(bodyID, out ContactCallbacks? callbacks))
            {
                callbacks = new ContactCallbacks();
                registeredCallbacks.Add(bodyID, callbacks);
            }

            return callbacks;
        }

        internal static void ClearRegistered()
        {
            registeredCallbacks.Clear();
        }

        private sealed class ContactCallbacks
        {
            public Action<ContactEvent>? ContactAdded;
            public Action<ContactEvent>? ContactPersisted;
            public Action<ContactEvent>? ContactRemoved;

            public void Dispatch(ContactEvent contactEvent)
            {
                switch (contactEvent.Type)
                {
                    case ContactEventType.Added:
                        ContactAdded?.Invoke(contactEvent);
                        break;
                    case ContactEventType.Persisted:
                        ContactPersisted?.Invoke(contactEvent);
                        break;
                    case ContactEventType.Removed:
                        ContactRemoved?.Invoke(contactEvent);
                        break;
                }
            }
        }

        public UInt32 ObjectLayer
        {
            get => getObjectLayer(bodyID);
            set => setObjectLayer(bodyID, value);
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
            set => SetMotionType(value, BodyActivation.Activate);
        }

        public MotionQuality MotionQuality
        {
            get => (MotionQuality)getMotionQuality(bodyID);
            set => setMotionQuality(bodyID, (Int32)value);
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

        public void SetMotionType(MotionType motionType, BodyActivation activation)
        {
            if (motionType == MotionType.Dynamic)
                throw new ArgumentOutOfRangeException(nameof(motionType), "Sensor only supports Static or Kinematic motion type.");

            setMotionType(bodyID, (Int32)motionType, (Int32)activation);
        }
    }
}
