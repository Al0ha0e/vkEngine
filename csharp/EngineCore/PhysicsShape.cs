using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    public enum PhysicsShapeType
    {
        Sphere,
        Box,
        Capsule,
        Cylinder,
        Triangle,
        Plane
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NativeSphereShape
    {
        public float Radius;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NativeBoxShape
    {
        public NVec3 HalfExtent;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NativeCapsuleShape
    {
        public float HalfHeight;
        public float Radius;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NativeCylinderShape
    {
        public float HalfHeight;
        public float Radius;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NativeTriangleShape
    {
        public NVec3 V1;
        public NVec3 V2;
        public NVec3 V3;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NativePlaneShape
    {
        public NVec3 Normal;
        public float Constant;
    }

    public abstract class PhysicsShape
    {
        internal abstract PhysicsShapeType Type { get; }
        internal abstract object ToNativeParameters();
    }

    public sealed class SphereShape : PhysicsShape
    {
        public float Radius { get; }

        public SphereShape(float radius)
        {
            Radius = radius;
        }

        internal override PhysicsShapeType Type => PhysicsShapeType.Sphere;

        internal override object ToNativeParameters() => new NativeSphereShape
        {
            Radius = Radius
        };
    }

    public sealed class BoxShape : PhysicsShape
    {
        public NVec3 HalfExtent { get; }

        public BoxShape(NVec3 halfExtent)
        {
            HalfExtent = halfExtent;
        }

        internal override PhysicsShapeType Type => PhysicsShapeType.Box;

        internal override object ToNativeParameters() => new NativeBoxShape
        {
            HalfExtent = HalfExtent
        };
    }

    public sealed class CapsuleShape : PhysicsShape
    {
        public float HalfHeight { get; }
        public float Radius { get; }

        public CapsuleShape(float halfHeight, float radius)
        {
            HalfHeight = halfHeight;
            Radius = radius;
        }

        internal override PhysicsShapeType Type => PhysicsShapeType.Capsule;

        internal override object ToNativeParameters() => new NativeCapsuleShape
        {
            HalfHeight = HalfHeight,
            Radius = Radius
        };
    }

    public sealed class CylinderShape : PhysicsShape
    {
        public float HalfHeight { get; }
        public float Radius { get; }

        public CylinderShape(float halfHeight, float radius)
        {
            HalfHeight = halfHeight;
            Radius = radius;
        }

        internal override PhysicsShapeType Type => PhysicsShapeType.Cylinder;

        internal override object ToNativeParameters() => new NativeCylinderShape
        {
            HalfHeight = HalfHeight,
            Radius = Radius
        };
    }

    public sealed class TriangleShape : PhysicsShape
    {
        public NVec3 V1 { get; }
        public NVec3 V2 { get; }
        public NVec3 V3 { get; }

        public TriangleShape(NVec3 v1, NVec3 v2, NVec3 v3)
        {
            V1 = v1;
            V2 = v2;
            V3 = v3;
        }

        internal override PhysicsShapeType Type => PhysicsShapeType.Triangle;

        internal override object ToNativeParameters() => new NativeTriangleShape
        {
            V1 = V1,
            V2 = V2,
            V3 = V3
        };
    }

    public sealed class PlaneShape : PhysicsShape
    {
        public NVec3 Normal { get; }
        public float Constant { get; }

        public PlaneShape(NVec3 normal, float constant)
        {
            Normal = normal;
            Constant = constant;
        }

        internal override PhysicsShapeType Type => PhysicsShapeType.Plane;

        internal override object ToNativeParameters() => new NativePlaneShape
        {
            Normal = Normal,
            Constant = Constant
        };
    }
}
