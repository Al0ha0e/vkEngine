using System;
using vkEngine.EngineCore;

namespace TestProj;

public sealed class FlashlightFlickerScript : EntityScript
{
    public float BaseIntensity = 2.5f;
    public float FlickerAmplitude = 1.5f;
    public float FlickerSpeed = 6.0f;
    public float DropoutChance = 0.08f;
    public float FollowSpeed = 8.0f;

    private SpotLight? spotLight;
    private readonly Random random = new(91);
    private float targetIntensity;
    private float nextFlickerTime;

    public FlashlightFlickerScript(UInt32 entity) : base(entity)
    {
    }

    public override void Start()
    {
        spotLight = GetComponent<SpotLight>();
        if (spotLight == null)
        {
            Console.WriteLine($"FlashlightFlickerScript entity={Entity} has no SpotLight component.");
            return;
        }

        targetIntensity = BaseIntensity;
        spotLight.Intensity = BaseIntensity;
        spotLight.Color = new NVec3(1.0f, 0.86f, 0.62f);
    }

    public override void Update()
    {
        if (spotLight == null)
            return;

        float time = Time.TimeSinceStartup;
        if (time >= nextFlickerTime)
        {
            bool dropout = random.NextSingle() < DropoutChance;
            float noise = random.NextSingle() * 2.0f - 1.0f;
            targetIntensity = dropout ? BaseIntensity * 0.18f : BaseIntensity + FlickerAmplitude * noise;
            targetIntensity = MathF.Max(0.0f, targetIntensity);
            nextFlickerTime = time + 1.0f / MathF.Max(1.0f, FlickerSpeed);
        }

        float alpha = 1.0f - MathF.Exp(-MathF.Max(0.0f, FollowSpeed) * Time.DeltaTime);
        spotLight.Intensity += (targetIntensity - spotLight.Intensity) * alpha;

        float warmth = 0.5f + 0.5f * MathF.Sin(time * 37.0f + targetIntensity);
        spotLight.Color = new NVec3(1.0f, 0.82f + 0.08f * warmth, 0.55f + 0.08f * warmth);
    }
}
