using System;
using vkEngine.EngineCore;

namespace TestProj;

public sealed class FpsTextScript : EntityScript
{
    public float UpdateInterval = 0.25f;
    private UIText? text;
    private int frames;
    private float elapsed;

    public FpsTextScript(UInt32 entity) : base(entity) { }

    public override void Start()
    {
        text = GetComponent<UIText>();
        UpdateText(0.0f);
    }

    public override void Update()
    {
        if (text == null)
            return;

        frames++;
        elapsed += Time.DeltaTime;
        if (elapsed < UpdateInterval)
            return;

        UpdateText(frames / elapsed);
        frames = 0;
        elapsed = 0.0f;
    }

    private void UpdateText(float fps)
    {
        if (text != null)
            text.Text = $"FPS: {fps:0.0}";
    }
}
