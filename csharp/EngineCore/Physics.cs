using System.Text;
using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    [StructLayout(LayoutKind.Sequential)]
    public struct RaycastHit
    {
        public UInt32 Entity;
        public UInt32 BodyID;
        public UInt32 SubShapeID;
        public NVec3 Point;
        public NVec3 Normal;
        public float Distance;
        public float Fraction;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct CollidePointHit
    {
        public UInt32 Entity;
        public UInt32 BodyID;
        public UInt32 SubShapeID;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct CollideShapeHit
    {
        public UInt32 Entity;
        public UInt32 BodyID;
        public UInt32 SubShapeID1;
        public UInt32 SubShapeID2;
        public NVec3 ContactPointOnShape;
        public NVec3 ContactPointOnBody;
        public NVec3 PenetrationAxis;
        public float PenetrationDepth;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ShapeCastHit
    {
        public UInt32 Entity;
        public UInt32 BodyID;
        public UInt32 SubShapeID1;
        public UInt32 SubShapeID2;
        public NVec3 ContactPointOnShape;
        public NVec3 ContactPointOnBody;
        public NVec3 PenetrationAxis;
        public float PenetrationDepth;
        public float Distance;
        public float Fraction;
        public Int32 IsBackFaceHit;
    }

    public static unsafe class Physics
    {
        public const UInt32 AllLayers = UInt32.MaxValue;
        public const UInt32 MaxObjectLayers = 32;
        public const UInt32 MaxBroadPhaseLayers = 32;
        public const UInt32 NonMovingObjectLayer = 0;
        public const UInt32 MovingObjectLayer = 1;
        public const UInt32 NonMovingBroadPhaseLayer = 0;
        public const UInt32 MovingBroadPhaseLayer = 1;

        private static delegate* unmanaged[Cdecl]<UInt32> getObjectLayerCount;
        private static delegate* unmanaged[Cdecl]<UInt32> getBroadPhaseLayerCount;
        private static delegate* unmanaged[Cdecl]<byte*, UInt32, UInt32> addObjectLayer;
        private static delegate* unmanaged[Cdecl]<byte*, UInt32> addBroadPhaseLayer;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32, void> setObjectLayerBroadPhaseLayer;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32> getObjectLayerBroadPhaseLayer;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32, Int32, void> setObjectLayerCollision;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32, Int32> getObjectLayerCollision;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32, Int32, void> setObjectVsBroadPhaseLayerCollision;
        private static delegate* unmanaged[Cdecl]<UInt32, UInt32, Int32> getObjectVsBroadPhaseLayerCollision;
        private static delegate* unmanaged[Cdecl]<NVec3*, NVec3*, float, UInt32, UInt32, RaycastHit*, Int32> raycast;
        private static delegate* unmanaged[Cdecl]<NVec3*, NVec3*, float, UInt32, UInt32, RaycastHit*, UInt32, UInt32> raycastAll;
        private static delegate* unmanaged[Cdecl]<NVec3*, UInt32, UInt32, CollidePointHit*, UInt32, UInt32> collidePoint;
        private static delegate* unmanaged[Cdecl]<Int32, void*, NVec3*, NQuat*, NVec3*, UInt32, UInt32, CollideShapeHit*, UInt32, UInt32> collideShape;
        private static delegate* unmanaged[Cdecl]<Int32, void*, NVec3*, NQuat*, NVec3*, NVec3*, float, UInt32, UInt32, ShapeCastHit*, UInt32, UInt32> castShape;

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            getObjectLayerCount = functions->GetPhysicsObjectLayerCount;
            getBroadPhaseLayerCount = functions->GetPhysicsBroadPhaseLayerCount;
            addObjectLayer = functions->AddPhysicsObjectLayer;
            addBroadPhaseLayer = functions->AddPhysicsBroadPhaseLayer;
            setObjectLayerBroadPhaseLayer = functions->SetPhysicsObjectLayerBroadPhaseLayer;
            getObjectLayerBroadPhaseLayer = functions->GetPhysicsObjectLayerBroadPhaseLayer;
            setObjectLayerCollision = functions->SetPhysicsObjectLayerCollision;
            getObjectLayerCollision = functions->GetPhysicsObjectLayerCollision;
            setObjectVsBroadPhaseLayerCollision = functions->SetPhysicsObjectVsBroadPhaseLayerCollision;
            getObjectVsBroadPhaseLayerCollision = functions->GetPhysicsObjectVsBroadPhaseLayerCollision;
            raycast = functions->PhysicsRaycast;
            raycastAll = functions->PhysicsRaycastAll;
            collidePoint = functions->PhysicsCollidePoint;
            collideShape = functions->PhysicsCollideShape;
            castShape = functions->PhysicsCastShape;
        }

        private static void* PinShapeParameters(PhysicsShape shape, out GCHandle handle)
        {
            handle = GCHandle.Alloc(shape.ToNativeParameters(), GCHandleType.Pinned);
            return (void*)handle.AddrOfPinnedObject();
        }

        private static byte[] EncodeNullTerminated(string value)
        {
            ArgumentNullException.ThrowIfNull(value);
            byte[] bytes = new byte[Encoding.UTF8.GetByteCount(value) + 1];
            Encoding.UTF8.GetBytes(value, bytes);
            return bytes;
        }

        public static UInt32 ObjectLayerCount => getObjectLayerCount();
        public static UInt32 BroadPhaseLayerCount => getBroadPhaseLayerCount();

        public static UInt32 LayerMask(UInt32 layer)
        {
            if (layer >= 32)
                throw new ArgumentOutOfRangeException(nameof(layer), "Layer masks support layer indices 0 through 31.");

            return 1u << (int)layer;
        }

        public static UInt32 AddObjectLayer(string name, UInt32 broadPhaseLayer = NonMovingBroadPhaseLayer)
        {
            byte[] bytes = EncodeNullTerminated(name);
            fixed (byte* namePtr = bytes)
            {
                return addObjectLayer(namePtr, broadPhaseLayer);
            }
        }

        public static UInt32 AddBroadPhaseLayer(string name)
        {
            byte[] bytes = EncodeNullTerminated(name);
            fixed (byte* namePtr = bytes)
            {
                return addBroadPhaseLayer(namePtr);
            }
        }

        public static void SetObjectLayerBroadPhaseLayer(UInt32 objectLayer, UInt32 broadPhaseLayer)
        {
            setObjectLayerBroadPhaseLayer(objectLayer, broadPhaseLayer);
        }

        public static UInt32 GetObjectLayerBroadPhaseLayer(UInt32 objectLayer)
        {
            return getObjectLayerBroadPhaseLayer(objectLayer);
        }

        public static void SetObjectLayerCollision(UInt32 layer1, UInt32 layer2, bool shouldCollide)
        {
            setObjectLayerCollision(layer1, layer2, shouldCollide ? 1 : 0);
        }

        public static bool GetObjectLayerCollision(UInt32 layer1, UInt32 layer2)
        {
            return getObjectLayerCollision(layer1, layer2) != 0;
        }

        public static void SetObjectVsBroadPhaseLayerCollision(UInt32 objectLayer, UInt32 broadPhaseLayer, bool shouldCollide)
        {
            setObjectVsBroadPhaseLayerCollision(objectLayer, broadPhaseLayer, shouldCollide ? 1 : 0);
        }

        public static bool GetObjectVsBroadPhaseLayerCollision(UInt32 objectLayer, UInt32 broadPhaseLayer)
        {
            return getObjectVsBroadPhaseLayerCollision(objectLayer, broadPhaseLayer) != 0;
        }

        public static bool Raycast(NVec3 origin, NVec3 direction, float maxDistance, out RaycastHit hit, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            NVec3 nativeOrigin = origin;
            NVec3 nativeDirection = direction;
            RaycastHit nativeHit = default;
            bool hadHit = raycast(&nativeOrigin, &nativeDirection, maxDistance, broadPhaseLayerMask, objectLayerMask, &nativeHit) != 0;
            hit = nativeHit;
            return hadHit;
        }

        public static RaycastHit[] RaycastAll(NVec3 origin, NVec3 direction, float maxDistance, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            if (maxHits <= 0)
                return Array.Empty<RaycastHit>();

            RaycastHit[] results = new RaycastHit[maxHits];
            NVec3 nativeOrigin = origin;
            NVec3 nativeDirection = direction;
            int count;
            fixed (RaycastHit* nativeResults = results)
            {
                count = (int)raycastAll(&nativeOrigin, &nativeDirection, maxDistance, broadPhaseLayerMask, objectLayerMask, nativeResults, (UInt32)results.Length);
            }

            if (count == results.Length)
                return results;

            Array.Resize(ref results, count);
            return results;
        }

        public static CollidePointHit[] CollidePoint(NVec3 point, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            if (maxHits <= 0)
                return Array.Empty<CollidePointHit>();

            CollidePointHit[] results = new CollidePointHit[maxHits];
            NVec3 nativePoint = point;
            int count;
            fixed (CollidePointHit* nativeResults = results)
            {
                count = (int)collidePoint(&nativePoint, broadPhaseLayerMask, objectLayerMask, nativeResults, (UInt32)results.Length);
            }

            if (count == results.Length)
                return results;

            Array.Resize(ref results, count);
            return results;
        }

        public static CollideShapeHit[] CollideShape(PhysicsShape shape, NVec3 position, NQuat rotation, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideShape(shape, position, rotation, NVec3.One, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static CollideShapeHit[] CollideShape(PhysicsShape shape, NVec3 position, NQuat rotation, NVec3 scale, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            if (maxHits <= 0)
                return Array.Empty<CollideShapeHit>();

            if (shape == null)
                throw new ArgumentNullException(nameof(shape));

            CollideShapeHit[] results = new CollideShapeHit[maxHits];
            NVec3 nativePosition = position;
            NQuat nativeRotation = rotation;
            NVec3 nativeScale = scale;
            int count;
            void* nativeShape = PinShapeParameters(shape, out GCHandle handle);
            try
            {
                fixed (CollideShapeHit* nativeResults = results)
                {
                    count = (int)collideShape((Int32)shape.Type, nativeShape, &nativePosition, &nativeRotation, &nativeScale, broadPhaseLayerMask, objectLayerMask, nativeResults, (UInt32)results.Length);
                }
            }
            finally
            {
                handle.Free();
            }

            if (count == results.Length)
                return results;

            Array.Resize(ref results, count);
            return results;
        }

        public static CollideShapeHit[] CollideSphere(NVec3 position, float radius, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideShape(new SphereShape(radius), position, NQuat.Identity, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static bool CollideSphereAny(NVec3 position, float radius, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideSphere(position, radius, 1, broadPhaseLayerMask, objectLayerMask).Length > 0;
        }

        public static bool CollideSphereAny(NVec3 position, float radius, out CollideShapeHit hit, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            CollideShapeHit[] results = CollideSphere(position, radius, 1, broadPhaseLayerMask, objectLayerMask);
            hit = results.Length > 0 ? results[0] : default;
            return results.Length > 0;
        }

        public static CollideShapeHit[] CollideBox(NVec3 position, NVec3 halfExtent, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideBox(position, halfExtent, NQuat.Identity, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static CollideShapeHit[] CollideBox(NVec3 position, NVec3 halfExtent, NQuat rotation, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideShape(new BoxShape(halfExtent), position, rotation, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static bool CollideBoxAny(NVec3 position, NVec3 halfExtent, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideBoxAny(position, halfExtent, NQuat.Identity, broadPhaseLayerMask, objectLayerMask);
        }

        public static bool CollideBoxAny(NVec3 position, NVec3 halfExtent, NQuat rotation, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideBox(position, halfExtent, rotation, 1, broadPhaseLayerMask, objectLayerMask).Length > 0;
        }

        public static bool CollideBoxAny(NVec3 position, NVec3 halfExtent, out CollideShapeHit hit, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideBoxAny(position, halfExtent, NQuat.Identity, out hit, broadPhaseLayerMask, objectLayerMask);
        }

        public static bool CollideBoxAny(NVec3 position, NVec3 halfExtent, NQuat rotation, out CollideShapeHit hit, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            CollideShapeHit[] results = CollideBox(position, halfExtent, rotation, 1, broadPhaseLayerMask, objectLayerMask);
            hit = results.Length > 0 ? results[0] : default;
            return results.Length > 0;
        }

        public static CollideShapeHit[] CollideCapsule(NVec3 position, float halfHeight, float radius, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideCapsule(position, halfHeight, radius, NQuat.Identity, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static CollideShapeHit[] CollideCapsule(NVec3 position, float halfHeight, float radius, NQuat rotation, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideShape(new CapsuleShape(halfHeight, radius), position, rotation, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static bool CollideCapsuleAny(NVec3 position, float halfHeight, float radius, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideCapsuleAny(position, halfHeight, radius, NQuat.Identity, broadPhaseLayerMask, objectLayerMask);
        }

        public static bool CollideCapsuleAny(NVec3 position, float halfHeight, float radius, NQuat rotation, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideCapsule(position, halfHeight, radius, rotation, 1, broadPhaseLayerMask, objectLayerMask).Length > 0;
        }

        public static bool CollideCapsuleAny(NVec3 position, float halfHeight, float radius, out CollideShapeHit hit, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CollideCapsuleAny(position, halfHeight, radius, NQuat.Identity, out hit, broadPhaseLayerMask, objectLayerMask);
        }

        public static bool CollideCapsuleAny(NVec3 position, float halfHeight, float radius, NQuat rotation, out CollideShapeHit hit, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            CollideShapeHit[] results = CollideCapsule(position, halfHeight, radius, rotation, 1, broadPhaseLayerMask, objectLayerMask);
            hit = results.Length > 0 ? results[0] : default;
            return results.Length > 0;
        }

        public static ShapeCastHit[] CastShape(PhysicsShape shape, NVec3 position, NQuat rotation, NVec3 direction, float maxDistance, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CastShape(shape, position, rotation, NVec3.One, direction, maxDistance, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static ShapeCastHit[] CastShape(PhysicsShape shape, NVec3 position, NQuat rotation, NVec3 scale, NVec3 direction, float maxDistance, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            if (maxHits <= 0)
                return Array.Empty<ShapeCastHit>();

            if (shape == null)
                throw new ArgumentNullException(nameof(shape));

            ShapeCastHit[] results = new ShapeCastHit[maxHits];
            NVec3 nativePosition = position;
            NQuat nativeRotation = rotation;
            NVec3 nativeScale = scale;
            NVec3 nativeDirection = direction;
            int count;
            void* nativeShape = PinShapeParameters(shape, out GCHandle handle);
            try
            {
                fixed (ShapeCastHit* nativeResults = results)
                {
                    count = (int)castShape((Int32)shape.Type, nativeShape, &nativePosition, &nativeRotation, &nativeScale, &nativeDirection, maxDistance, broadPhaseLayerMask, objectLayerMask, nativeResults, (UInt32)results.Length);
                }
            }
            finally
            {
                handle.Free();
            }

            if (count == results.Length)
                return results;

            Array.Resize(ref results, count);
            return results;
        }

        public static ShapeCastHit[] CastSphere(NVec3 position, float radius, NVec3 direction, float maxDistance, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CastShape(new SphereShape(radius), position, NQuat.Identity, direction, maxDistance, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static ShapeCastHit[] CastBox(NVec3 position, NVec3 halfExtent, NVec3 direction, float maxDistance, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CastBox(position, halfExtent, NQuat.Identity, direction, maxDistance, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static ShapeCastHit[] CastBox(NVec3 position, NVec3 halfExtent, NQuat rotation, NVec3 direction, float maxDistance, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CastShape(new BoxShape(halfExtent), position, rotation, direction, maxDistance, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static ShapeCastHit[] CastCapsule(NVec3 position, float halfHeight, float radius, NVec3 direction, float maxDistance, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CastCapsule(position, halfHeight, radius, NQuat.Identity, direction, maxDistance, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        public static ShapeCastHit[] CastCapsule(NVec3 position, float halfHeight, float radius, NQuat rotation, NVec3 direction, float maxDistance, int maxHits = 32, UInt32 broadPhaseLayerMask = AllLayers, UInt32 objectLayerMask = AllLayers)
        {
            return CastShape(new CapsuleShape(halfHeight, radius), position, rotation, direction, maxDistance, maxHits, broadPhaseLayerMask, objectLayerMask);
        }
    }
}
