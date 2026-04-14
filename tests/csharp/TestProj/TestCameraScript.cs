using System;
using vkEngine.EngineCore;

namespace TestProj;

public sealed class TestCameraScript : EntityScript
{
    public float MoveSpeed = 2.5f;
    public float RotateSpeed = 1.0f;
    private Transform? transform;

    public TestCameraScript(UInt32 entity) : base(entity)
    {
        Console.WriteLine("TestCameraScript Construct!!!!! " + entity);
    }

    public override void Start()
    {
        Console.WriteLine(
            $"TestCameraScript.Start entity={Entity} MoveSpeed={MoveSpeed} RotateSpeed={RotateSpeed}");
        transform = new Transform(Entity);
        Input.CursorMode = CursorMode.Disabled;
    }

    public override void Update()
    {
        if (transform == null)
            return;

        if (Input.IsKeyDown(KeyCode.Escape))
        {
            EngineStateManager.SetState(EngineState.Terminated);
            return;
        }

        float deltaTime = Time.DeltaTime;
        float moveStep = MoveSpeed * deltaTime;
        float rotateStep = RotateSpeed * deltaTime;

        NVec3 movement = NVec3.Zero;

        if (Input.IsKeyDown(KeyCode.W))
            movement += new NVec3(0f, 0f, -moveStep);
        if (Input.IsKeyDown(KeyCode.S))
            movement += new NVec3(0f, 0f, moveStep);
        if (Input.IsKeyDown(KeyCode.A))
            movement += new NVec3(-moveStep, 0f, 0f);
        if (Input.IsKeyDown(KeyCode.D))
            movement += new NVec3(moveStep, 0f, 0f);

        if (movement.x != 0f || movement.y != 0f || movement.z != 0f)
            transform.TranslateLocal(movement);

        NVec2 mouseDelta = Input.MouseDelta;
        if (mouseDelta.x != 0f)
            transform.RotateGlobal(-mouseDelta.x * rotateStep, new NVec3(0f, 1f, 0f));
        if (mouseDelta.y != 0f)
            transform.RotateLocal(-mouseDelta.y * rotateStep, new NVec3(1f, 0f, 0f));
    }

    public override void Unload()
    {
        Input.CursorMode = CursorMode.Normal;
    }
}