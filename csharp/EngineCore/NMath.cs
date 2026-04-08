using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    [StructLayout(LayoutKind.Sequential)]
    public struct NVec2
    {
        public float x, y;

        public NVec2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }

        public static NVec2 Zero => new(0f, 0f);

        public static NVec2 operator +(NVec2 left, NVec2 right) => new(left.x + right.x, left.y + right.y);
        public static NVec2 operator -(NVec2 left, NVec2 right) => new(left.x - right.x, left.y - right.y);
        public static NVec2 operator *(NVec2 left, float scalar) => new(left.x * scalar, left.y * scalar);
        public static NVec2 operator /(NVec2 left, float scalar) => new(left.x / scalar, left.y / scalar);

        public override readonly string ToString() => $"({x}, {y})";
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NVec3
    {
        public float x, y, z;

        public NVec3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public static NVec3 Zero => new(0f, 0f, 0f);
        public static NVec3 One => new(1f, 1f, 1f);

        public static NVec3 operator +(NVec3 left, NVec3 right) => new(left.x + right.x, left.y + right.y, left.z + right.z);
        public static NVec3 operator -(NVec3 left, NVec3 right) => new(left.x - right.x, left.y - right.y, left.z - right.z);
        public static NVec3 operator *(NVec3 left, float scalar) => new(left.x * scalar, left.y * scalar, left.z * scalar);
        public static NVec3 operator /(NVec3 left, float scalar) => new(left.x / scalar, left.y / scalar, left.z / scalar);

        public override readonly string ToString() => $"({x}, {y}, {z})";
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NQuat
    {
        public float x, y, z, w;

        public NQuat(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        public static NQuat Identity => new(0f, 0f, 0f, 1f);

        public override readonly string ToString() => $"({x}, {y}, {z}, {w})";
    }
}
