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
    public float YawSpeed = 1.0f;

    private SpotLight? spotLight;
    private Transform? transform;

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

        UpdateYawRotation();
    }

    private void UpdateYawRotation()
    {
        if (transform == null)
            return;

        transform.RotateGlobal(YawSpeed * Time.DeltaTime, new NVec3(0f, 1f, 0f));
    }
}
