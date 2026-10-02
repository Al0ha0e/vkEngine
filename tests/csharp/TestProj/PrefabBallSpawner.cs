using vkEngine.EngineCore;

namespace TestProj;

public sealed class PrefabBallSpawner : EntityScript
{
    [Export] public long BallPrefab = 1024;
    [Export] public float SpawnHeight = 5.0f;
    private int spawnCount;

    public PrefabBallSpawner(UInt32 entity) : base(entity) { }

    public override void Start()
    {
        Input.CursorMode = CursorMode.Normal;
    }

    public override void Update()
    {
        if (Input.IsKeyPressed(KeyCode.Escape))
        {
            EngineStateManager.SetState(EngineState.Terminated);
            return;
        }

        if (!Input.IsMouseButtonPressed(MouseButton.Left) || BallPrefab <= 0)
            return;

        // Spread consecutive drops across five positions, starting in the center.
        float x = ((spawnCount + 2) % 5 - 2) * 2.5f;
        if (SceneManager.InstantiatePrefab((UInt64)BallPrefab,
            new NVec3(x, SpawnHeight, 0), NQuat.Identity, new NVec3(1, 1, 1)))
            ++spawnCount;
    }
}
