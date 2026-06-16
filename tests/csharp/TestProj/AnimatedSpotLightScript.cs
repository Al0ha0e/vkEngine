using System;
using vkEngine.EngineCore;

namespace TestProj;

public sealed class AnimatedSpotLightScript : EntityScript
{
    public float ColorSpeed = 1.2f;
    public float IntensityBase = 8.0f;
    public float IntensityAmplitude = 4.0f;
    public float InnerConeBase = 0.35f;
    public float InnerConeAmplitude = 0.12f;
    public float OuterConeBase = 0.75f;
    public float OuterConeAmplitude = 0.18f;
    public float MaxYawJitter = 0.45f;
    public float MaxPitchJitter = 0.25f;
    public float RotationRetargetInterval = 0.8f;
    public float RotationFollowSpeed = 5.0f;

    private SpotLight? spotLight;
    private Transform? transform;
    private NQuat baseRotation = NQuat.Identity;
    private NQuat targetRotation = NQuat.Identity;
    private float nextRotationRetargetTime;
    private readonly Random random = new(17);

    public AnimatedSpotLightScript(UInt32 entity) : base(entity)
    {
    }

    public override void Start()
    {
        spotLight = GetComponent<SpotLight>();
        if (spotLight == null)
            Console.WriteLine($"AnimatedSpotLightScript entity={Entity} has no SpotLight component.");

        transform = GetComponent<Transform>();
        if (transform == null)
        {
            Console.WriteLine($"AnimatedSpotLightScript entity={Entity} has no Transform component.");
            return;
        }

        baseRotation = transform.LocalRotation;
        targetRotation = baseRotation;
        nextRotationRetargetTime = Time.TimeSinceStartup;
    }

    public override void Update()
    {
        if (spotLight == null)
            return;

        float t = Time.TimeSinceStartup;
        float r = 0.5f + 0.5f * MathF.Sin(t * ColorSpeed);
        float g = 0.5f + 0.5f * MathF.Sin(t * ColorSpeed + 2.0943951f);
        float b = 0.5f + 0.5f * MathF.Sin(t * ColorSpeed + 4.1887902f);
        spotLight.Color = new NVec3(r, g, b);

        spotLight.Intensity = IntensityBase + IntensityAmplitude * (0.5f + 0.5f * MathF.Sin(t * 1.7f));

        float coneWave = MathF.Sin(t * 1.1f);
        float innerCone = InnerConeBase + InnerConeAmplitude * coneWave;
        float outerCone = OuterConeBase + OuterConeAmplitude * MathF.Sin(t * 1.1f + 0.7f);
        spotLight.InnerCone = MathF.Max(0.05f, innerCone);
        spotLight.OuterCone = MathF.Max(spotLight.InnerCone + 0.05f, outerCone);

        UpdateRandomRotation(t);
    }

    private void UpdateRandomRotation(float time)
    {
        if (transform == null)
            return;

        if (time >= nextRotationRetargetTime)
        {
            float yaw = RandomRange(-MaxYawJitter, MaxYawJitter);
            float pitch = RandomRange(-MaxPitchJitter, MaxPitchJitter);
            targetRotation = Normalize(Multiply(baseRotation, Multiply(FromAxisAngle(new NVec3(0f, 1f, 0f), yaw), FromAxisAngle(new NVec3(1f, 0f, 0f), pitch))));
            nextRotationRetargetTime = time + MathF.Max(0.05f, RotationRetargetInterval);
        }

        float alpha = 1.0f - MathF.Exp(-MathF.Max(0.0f, RotationFollowSpeed) * Time.DeltaTime);
        transform.LocalRotation = Normalize(Lerp(transform.LocalRotation, targetRotation, alpha));
    }

    private float RandomRange(float min, float max)
    {
        return min + (max - min) * random.NextSingle();
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

    private static NQuat Lerp(NQuat from, NQuat to, float alpha)
    {
        float dot = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
        if (dot < 0.0f)
            to = new NQuat(-to.x, -to.y, -to.z, -to.w);

        return new NQuat(
            from.x + (to.x - from.x) * alpha,
            from.y + (to.y - from.y) * alpha,
            from.z + (to.z - from.z) * alpha,
            from.w + (to.w - from.w) * alpha);
    }

    private static NQuat Normalize(NQuat q)
    {
        float length = MathF.Sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        return length > 0.0001f ? new NQuat(q.x / length, q.y / length, q.z / length, q.w / length) : NQuat.Identity;
    }
}
