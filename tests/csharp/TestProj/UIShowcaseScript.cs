using vkEngine.EngineCore;

namespace TestProj;

public sealed class UIShowcaseScript : EntityScript
{
    private Canvas canvas = null!;
    private UIComponent spinner = null!;
    private UIComponent dockA = null!;
    private UIComponent dockB = null!;
    private UIContainer? runtimeGroup;
    private UIButton? runtimeButton;
    private readonly List<(UIComponent Node, Action<UIPointerEvent> Handler)> clicks = new();
    private UIImage portrait = null!;
    private UIImage crop = null!;
    private UIImage blue = null!;
    private UIImage progress = null!;
    private UIText metrics = null!;
    private UIText treeStatus = null!;
    private UIText animationStatus = null!;
    private UIButton textPreview = null!;
    private UIText textSample = null!;
    private UIText textMetrics = null!;
    private UIText fontSizeLabel = null!;
    private UIText widthLabel = null!;
    private UIText alignmentLabel = null!;
    private UIText wrapLabel = null!;
    private int sampleIndex;
    private bool inDockB;
    private bool blueOnTop;
    private bool paused;
    private float animationTime;
    private float metricsTime;
    private int frames;

    public UIShowcaseScript(UInt32 entity) : base(entity) { }

    public override void Start()
    {
        Input.CursorMode = CursorMode.Normal;
        canvas = GetComponent<Canvas>()!;
        // Find searches direct children; cache the scene's named nodes once.
        var board = canvas.Find("Board")!;
        var typography = canvas.Find("Typography", board)!;
        textPreview = (UIButton)canvas.Find("TextPreview", typography)!;
        textSample = (UIText)canvas.Find("Sample", textPreview)!;
        textMetrics = (UIText)canvas.Find("TextMetrics", typography)!;
        var fontButton = canvas.Find("FontSizeButton", typography)!;
        var widthButton = canvas.Find("WidthButton", typography)!;
        var alignmentButton = canvas.Find("AlignmentButton", typography)!;
        var wrapButton = canvas.Find("WrapButton", typography)!;
        fontSizeLabel = (UIText)canvas.Find("Label", fontButton)!;
        widthLabel = (UIText)canvas.Find("Label", widthButton)!;
        alignmentLabel = (UIText)canvas.Find("Label", alignmentButton)!;
        wrapLabel = (UIText)canvas.Find("Label", wrapButton)!;
        bindClick(textPreview, _ => cycleTextSample());
        bindClick(fontButton, _ => cycleFontSize());
        bindClick(widthButton, _ => toggleTextWidth());
        bindClick(alignmentButton, _ => cycleTextAlignment());
        bindClick(wrapButton, _ => toggleTextWrap());
        var images = canvas.Find("Images", board)!;
        portrait = (UIImage)canvas.Find("Aspect", canvas.Find("AspectFrame", images))!;
        crop = (UIImage)canvas.Find("Crop", canvas.Find("CropFrame", images))!;
        spinner = canvas.Find("Spinner", canvas.Find("Layout", board))!;
        var hierarchy = canvas.Find("Hierarchy", board)!;
        dockA = canvas.Find("DockA", hierarchy)!;
        dockB = canvas.Find("DockB", hierarchy)!;
        treeStatus = (UIText)canvas.Find("TreeStatus", hierarchy)!;
        var ordering = canvas.Find("Ordering", board)!;
        blue = (UIImage)canvas.Find("Blue", canvas.Find("Stack", ordering))!;
        var live = canvas.Find("Live", board)!;
        metrics = (UIText)canvas.Find("Metrics", live)!;
        animationStatus = (UIText)canvas.Find("AnimationStatus", live)!;
        progress = (UIImage)canvas.Find("Progress", canvas.Find("ProgressTrack", live))!;
        bindClick(canvas.Find("TextureButton", images)!, _ => toggleTexture());
        bindClick(canvas.Find("AspectButton", images)!, _ => toggleAspect());
        bindClick(canvas.Find("CropButton", images)!, _ => toggleCrop());
        bindClick(canvas.Find("ToggleTree", hierarchy)!, _ => toggleRuntimeGroup());
        bindClick(canvas.Find("MoveTree", hierarchy)!, _ => moveRuntimeGroup());
        bindClick(canvas.Find("SwapOrder", ordering)!, _ => swapOrder());
        bindClick(canvas.Find("AnimationButton", live)!, _ => toggleAnimation());
        bindClick(canvas.Find("PrefabButton", live)!, _ => spawnPrefab());
        createRuntimeGroup();
    }

    public override void Update()
    {
        if (Input.IsKeyPressed(KeyCode.Escape))
        {
            EngineStateManager.SetState(EngineState.Terminated);
            return;
        }

        if (Input.IsKeyPressed(KeyCode.Space))
            toggleAnimation();
        if (Input.IsKeyPressed(KeyCode.I))
            toggleAspect();
        if (Input.IsKeyPressed(KeyCode.U))
            toggleCrop();
        if (Input.IsKeyPressed(KeyCode.T))
            toggleTexture();
        if (Input.IsKeyPressed(KeyCode.B))
            swapOrder();
        if (Input.IsKeyPressed(KeyCode.R))
            canvas.ReferenceResolution = canvas.ReferenceResolution.x == 1600 ? new NVec2(1920, 1584) : new NVec2(1600, 1320);
        if (Input.IsKeyPressed(KeyCode.N))
            toggleRuntimeGroup();
        if (Input.IsKeyPressed(KeyCode.H))
            moveRuntimeGroup();
        if (Input.IsKeyPressed(KeyCode.P))
            spawnPrefab();

        if (!paused)
        {
            animationTime += Time.DeltaTime;
            var transform = spinner.RectTransform;
            transform.Rotation = 0.3f * MathF.Sin(animationTime);
            float scale = 1 + 0.12f * MathF.Sin(animationTime * 0.7f);
            transform.Scale = new NVec2(scale, scale);
            spinner.RectTransform = transform;

            float amount = 0.5f + 0.5f * MathF.Sin(animationTime * 0.8f);
            var fill = progress.RectTransform;
            fill.AnchorMax = new NVec2(amount, 1);
            progress.RectTransform = fill;
            progress.Color = new NVec4(0.15f + 0.7f * amount, 0.82f, 0.85f - 0.5f * amount, 1);
        }

        // Refresh diagnostic labels four times per second; transforms animate every frame.
        ++frames;
        metricsTime += Time.DeltaTime;
        if (metricsTime >= 0.25f)
        {
            var size = canvas.ResolvedSize;
            var reference = canvas.ReferenceResolution;
            metrics.Text = $"FPS: {frames / metricsTime:0}\nCanvas: {size.x:0} x {size.y:0}\nReference: {reference.x:0} x {reference.y:0}";
            var natural = textSample.Measure();
            var laidOut = textSample.TextSize;
            textMetrics.Text = $"Natural: {natural.x:0} x {natural.y:0} | Layout: {laidOut.x:0} x {laidOut.y:0}\n" +
                $"Box: {textSample.ResolvedSize.x:0} x {textSample.ResolvedSize.y:0} | " +
                (textSample.WordWrap ? "Word wrap; long words split." : "No wrap; overflow stays visible.");
            frames = 0;
            metricsTime = 0;
        }
    }

    public override void Unload()
    {
        foreach (var (node, handler) in clicks) node.Click -= handler;
        clicks.Clear();
        if (runtimeButton != null) runtimeButton.Click -= onRuntimeClick;
    }

    private void bindClick(UIComponent node, Action<UIPointerEvent> handler)
    {
        node.Click += handler;
        clicks.Add((node, handler));
    }

    private void toggleAnimation()
    {
        paused = !paused;
        animationStatus.Text = paused ? "Animation: paused" : "Animation: running";
        animationStatus.Color = paused ? new NVec4(1, 0.53f, 0.22f, 1) : new NVec4(0.15f, 0.82f, 0.85f, 1);
    }

    private void cycleTextSample()
    {
        sampleIndex = (sampleIndex + 1) % 3;
        textSample.Text = sampleIndex switch
        {
            0 => "Words wrap with the box. Alignment applies to every line.",
            1 => "Supercalifragilisticexpialidocious\nLong words split at codepoints.",
            _ => "UTF-8: café  naïve  Ω  Ж\nExplicit breaks + automatic wrapping."
        };
    }

    private void cycleFontSize()
    {
        textSample.FontSize = textSample.FontSize switch { 18 => 22, 22 => 26, _ => 18 };
        fontSizeLabel.Text = $"Font: {textSample.FontSize:0}";
    }

    private void toggleTextWidth()
    {
        var rect = textPreview.RectTransform;
        float width = rect.OffsetMax.x - rect.OffsetMin.x == 456 ? 300 : 456;
        rect.OffsetMax = new NVec2(rect.OffsetMin.x + width, rect.OffsetMax.y);
        textPreview.RectTransform = rect;
        widthLabel.Text = $"Width: {width:0}";
    }

    private void cycleTextAlignment()
    {
        var options = textSample.LayoutOptions;
        int alignmentIndex = ((int)options.VerticalAlignment * 3 + (int)options.HorizontalAlignment + 1) % 9;
        options.HorizontalAlignment = (TextHorizontalAlignment)(alignmentIndex % 3);
        options.VerticalAlignment = (TextVerticalAlignment)(alignmentIndex / 3);
        textSample.LayoutOptions = options;
        alignmentLabel.Text = $"Align: {options.HorizontalAlignment.ToString()[0]}/{options.VerticalAlignment.ToString()[0]}";
    }

    private void toggleTextWrap()
    {
        textSample.WordWrap = !textSample.WordWrap;
        wrapLabel.Text = textSample.WordWrap ? "Wrap: on" : "Wrap: off";
    }

    private void toggleAspect() => portrait.PreserveAspect = !portrait.PreserveAspect;
    private void toggleCrop() => crop.UVRect = crop.UVRect.x == 0 ? new NVec4(0.2f, 0.1f, 0.8f, 0.55f) : new NVec4(0, 0, 1, 1);
    private void toggleTexture() => portrait.Texture = portrait.Texture == 0 ? 1024UL : 0;
    private void spawnPrefab() => SceneManager.InstantiatePrefab(1024, NVec3.Zero, NQuat.Identity, new NVec3(1, 1, 1));

    private void swapOrder()
    {
        blueOnTop = !blueOnTop;
        blue.SetSiblingIndex(blueOnTop ? 1 : 0);
    }

    private void toggleRuntimeGroup()
    {
        if (runtimeGroup == null) createRuntimeGroup();
        else removeRuntimeGroup();
    }

    private void onRuntimeClick(UIPointerEvent e) => removeRuntimeGroup();

    private void removeRuntimeGroup()
    {
        UInt64 handle = runtimeGroup!.Handle;
        // Removing the ancestor during Click also invalidates the event target and its subscriptions.
        canvas.Remove(runtimeGroup);
        treeStatus.Text = $"Removed: valid={runtimeGroup.IsValid}, Get=null: {canvas.Get(handle) == null}";
        runtimeGroup = null;
        runtimeButton = null;
    }

    private void moveRuntimeGroup()
    {
        if (runtimeGroup == null) return;
        inDockB = !inDockB;
        runtimeGroup.SetParent(inDockB ? dockB : dockA);
        updateTreeStatus();
    }

    private void createRuntimeGroup()
    {
        runtimeGroup = canvas.CreateContainer("RuntimeGroup", inDockB ? dockB : dockA);
        runtimeGroup.RectTransform = new RectTransform
        {
            OffsetMin = new NVec2(12, 48), OffsetMax = new NVec2(198, 132), Pivot = NVec2.Zero
        };
        var background = canvas.CreateButton("Card", runtimeGroup);
        background.RectTransform = new RectTransform
        {
            AnchorMax = new NVec2(1, 1), Pivot = NVec2.Zero
        };
        background.Color = new NVec4(0.08f, 0.5f, 0.52f, 0.9f);
        background.HoverColor = new NVec4(0.12f, 0.7f, 0.65f, 1);
        background.PressedColor = new NVec4(0.04f, 0.25f, 0.3f, 1);
        runtimeButton = background;
        runtimeButton.Click += onRuntimeClick;
        var label = canvas.CreateText("Label", background);
        label.RectTransform = new RectTransform
        {
            AnchorMax = new NVec2(1, 1),
            OffsetMin = new NVec2(8, 4), OffsetMax = new NVec2(-8, -4)
        };
        label.LayoutOptions = new TextLayoutOptions
        {
            FontSize = 19.2f, HorizontalAlignment = TextHorizontalAlignment.Center,
            VerticalAlignment = TextVerticalAlignment.Middle, WordWrap = true
        };
        label.Text = "Created in C#\nClick: remove me";
        updateTreeStatus();
    }

    private void updateTreeStatus()
    {
        var parent = inDockB ? dockB : dockA;
        treeStatus.Text = $"Parent {(inDockB ? "B" : "A")} children: {parent.Children.Count} | handle: {runtimeGroup!.Handle}";
    }
}

// Each Canvas is addressed through its own host entity, including after scene restoration.
public sealed class UIOverlayScript : EntityScript
{
    private Canvas canvas = null!;

    public UIOverlayScript(UInt32 entity) : base(entity) { }

    public override void Start() => canvas = GetComponent<Canvas>()!;

    public override void Update()
    {
        if (Input.IsKeyPressed(KeyCode.O))
            canvas.SortOrder = canvas.SortOrder == 10 ? -10 : 10;
        if (Input.IsKeyPressed(KeyCode.R))
            canvas.ReferenceResolution = canvas.ReferenceResolution.x == 1600 ? new NVec2(1920, 1584) : new NVec2(1600, 1320);
    }
}

public sealed class UIToastScript : EntityScript
{
    private float elapsed;
    private UIButton dismiss = null!;

    public UIToastScript(UInt32 entity) : base(entity) { }

    public override void Start()
    {
        var canvas = GetComponent<Canvas>()!;
        dismiss = (UIButton)canvas.Find("Dismiss", canvas.Find("Toast"))!;
        dismiss.Click += onDismiss;
    }

    public override void Unload() => dismiss.Click -= onDismiss;

    private void onDismiss(UIPointerEvent e) => DestroyEntity();

    public override void Update()
    {
        elapsed += Time.DeltaTime;
        if (elapsed >= 3)
            SceneManager.DestroyEntity(Entity);
    }
}
