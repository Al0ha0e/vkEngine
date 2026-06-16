using System;
using vkEngine.EngineCore;

namespace TestProj;

public sealed class SunCycleScript : EntityScript
{
    public float Speed = 0.15f;

    private Transform? transform;
    private NQuat baseRotation = NQuat.Identity;
    private float angle;

    public SunCycleScript(UInt32 entity) : base(entity)
    {
    }

    public override void Start()
    {
        transform = GetComponent<Transform>();
        if (transform == null)
        {
            Console.WriteLine($"SunCycleScript entity={Entity} has no Transform component.");
            return;
        }

        if (GetComponent<DirectionalLight>() == null)
            Console.WriteLine($"SunCycleScript entity={Entity} has no DirectionalLight component.");

        baseRotation = transform.LocalRotation;
    }

    public override void Update()
    {
        if (transform == null)
            return;

        angle += Speed * Time.DeltaTime;
        transform.LocalRotation = Normalize(Multiply(FromAxisAngle(new NVec3(1f, 0f, 0f), angle), baseRotation));
    }

    private static NQuat FromAxisAngle(NVec3 axis, float radians)
    {
        float half = radians * 0.5f;
        float sin = MathF.Sin(half);
        return Normalize(new NQuat(axis.x * sin, axis.y * sin, axis.z * sin, MathF.Cos(half)));
    }

    private static NQuat Multiply(NQuat left, NQuat right)
    {
        return new NQuat(
            left.w * right.x + left.x * right.w + left.y * right.z - left.z * right.y,
            left.w * right.y - left.x * right.z + left.y * right.w + left.z * right.x,
            left.w * right.z + left.x * right.y - left.y * right.x + left.z * right.w,
            left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z);
    }

    private static NQuat Normalize(NQuat q)
    {
        float length = MathF.Sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        return length > 0.0001f ? new NQuat(q.x / length, q.y / length, q.z / length, q.w / length) : NQuat.Identity;
    }
}
