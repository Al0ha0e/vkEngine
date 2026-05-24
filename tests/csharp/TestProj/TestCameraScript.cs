using System;
using vkEngine.EngineCore;

namespace TestProj;

public sealed class TestCameraScript : EntityScript
{
    public float MoveSpeed = 2.5f;
    public float RotateSpeed = 1.0f;
    public float JumpSpeed = 5.0f;
    private Transform? transform;
    private CharacterController? characterController;

    public TestCameraScript(UInt32 entity) : base(entity)
    {
        Console.WriteLine("TestCameraScript Construct!!!!! " + entity);
    }

    public override void Start()
    {
        Console.WriteLine(
            $"TestCameraScript.Start entity={Entity} MoveSpeed={MoveSpeed} RotateSpeed={RotateSpeed}");
        transform = new Transform(Entity);
        if (HasComponent<CharacterController>())
            characterController = new CharacterController(Entity);
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
        NVec3 velocity = NVec3.Zero;

        if (Input.IsKeyDown(KeyCode.W))
        {
            movement += new NVec3(0f, 0f, -moveStep);
            velocity += new NVec3(0f, 0f, -MoveSpeed);
        }
        if (Input.IsKeyDown(KeyCode.S))
        {
            movement += new NVec3(0f, 0f, moveStep);
            velocity += new NVec3(0f, 0f, MoveSpeed);
        }
        if (Input.IsKeyDown(KeyCode.A))
        {
            movement += new NVec3(-moveStep, 0f, 0f);
            velocity += new NVec3(-MoveSpeed, 0f, 0f);
        }
        if (Input.IsKeyDown(KeyCode.D))
        {
            movement += new NVec3(moveStep, 0f, 0f);
            velocity += new NVec3(MoveSpeed, 0f, 0f);
        }

        bool wantsJump = characterController != null
            && Input.IsKeyPressed(KeyCode.Space)
            && characterController.IsGrounded;
        bool wantsMove = movement.x != 0f || movement.y != 0f || movement.z != 0f;

        if (wantsMove || wantsJump)
        {
            if (characterController != null)
            {
                NVec3 worldVelocity = LocalToHorizontalWorld(velocity);
                if (wantsJump)
                    worldVelocity.y = JumpSpeed;
                characterController.Move(worldVelocity);
            }
            else
            {
                transform.TranslateLocal(movement);
            }
        }
        else if (characterController != null)
        {
            characterController.Move(NVec3.Zero);
        }

        NVec2 mouseDelta = Input.MouseDelta;
        if (mouseDelta.x != 0f)
            transform.RotateGlobal(-mouseDelta.x * rotateStep, new NVec3(0f, 1f, 0f));
        if (mouseDelta.y != 0f)
            transform.RotateLocal(-mouseDelta.y * rotateStep, new NVec3(1f, 0f, 0f));
    }

    public override void Unload()
    {
        characterController?.Move(NVec3.Zero);
        Input.CursorMode = CursorMode.Normal;
    }

    private NVec3 LocalToHorizontalWorld(NVec3 local)
    {
        NQuat rotation = transform?.LocalRotation ?? NQuat.Identity;
        NVec3 forward = NormalizeHorizontal(Rotate(rotation, new NVec3(0f, 0f, -1f)));
        NVec3 right = NormalizeHorizontal(Rotate(rotation, new NVec3(1f, 0f, 0f)));
        return right * local.x + forward * -local.z;
    }

    private static NVec3 Rotate(NQuat q, NVec3 v)
    {
        NVec3 u = new(q.x, q.y, q.z);
        float s = q.w;
        return u * (2f * Dot(u, v)) + v * (s * s - Dot(u, u)) + Cross(u, v) * (2f * s);
    }

    private static NVec3 NormalizeHorizontal(NVec3 v)
    {
        float length = MathF.Sqrt(v.x * v.x + v.z * v.z);
        return length > 0.0001f ? new NVec3(v.x / length, 0f, v.z / length) : NVec3.Zero;
    }

    private static float Dot(NVec3 left, NVec3 right)
    {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    private static NVec3 Cross(NVec3 left, NVec3 right)
    {
        return new NVec3(
            left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x);
    }
}
