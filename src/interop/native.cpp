#include <script.hpp>
#include <component/transform.hpp>
#include <input.hpp>
#include <time.hpp>
#include <engine_state.hpp>
#include <interop/math.hpp>
#include <interop/native.hpp>
#include <logger.hpp>
#include <scene.hpp>

namespace vke_interop
{
    static vke_common::Scene *GetCurrentScene()
    {
        return vke_common::SceneManager::GetInstance()->currentScene.get();
    }

    static inline entt::entity GetEntity(vke_common::Scene *scene, uint32_t entity)
    {
        entt::entity ent = static_cast<entt::entity>(entity);
        // VKE_FATAL_IF(scene == nullptr || !scene->registry.valid(ent), "Invalid scene entity {}", entity)
        return ent;
    }

    static vke_common::Transform &GetTransform(vke_common::Scene *scene, uint32_t entity)
    {
        return scene->registry.get<vke_common::Transform>(GetEntity(scene, entity));
    }

    static glm::vec3 ToGlm(const Vector3<float> &value)
    {
        return glm::vec3(value.x, value.y, value.z);
    }

    static glm::quat ToGlm(const Quaternion<float> &value)
    {
        return glm::normalize(glm::quat(value.w, value.x, value.y, value.z));
    }

    static Vector3<float> ToInterop(const glm::vec3 &value)
    {
        return Vector3<float>{value.x, value.y, value.z};
    }

    static Vector2<float> ToInterop(const glm::vec2 &value)
    {
        return Vector2<float>{value.x, value.y};
    }

    static Quaternion<float> ToInterop(const glm::quat &value)
    {
        return Quaternion<float>{value.x, value.y, value.z, value.w};
    }

    static void VKE_INTEROP_CDECL GetTransformLocalPosition(uint32_t entity, Vector3<float> *position)
    {
        vke_common::Scene *scene = GetCurrentScene();
        *position = ToInterop(GetTransform(scene, entity).localPosition);
    }

    static void VKE_INTEROP_CDECL SetTransformLocalPosition(uint32_t entity, const Vector3<float> *position)
    {
        vke_common::Scene *scene = GetCurrentScene();
        scene->transformSystem.SetLocalPosition(GetEntity(scene, entity), ToGlm(*position));
    }

    static void VKE_INTEROP_CDECL GetTransformLocalRotation(uint32_t entity, Quaternion<float> *rotation)
    {
        vke_common::Scene *scene = GetCurrentScene();
        *rotation = ToInterop(GetTransform(scene, entity).localRotation);
    }

    static void VKE_INTEROP_CDECL SetTransformLocalRotation(uint32_t entity, const Quaternion<float> *rotation)
    {
        vke_common::Scene *scene = GetCurrentScene();
        scene->transformSystem.SetLocalRotation(GetEntity(scene, entity), ToGlm(*rotation));
    }

    static void VKE_INTEROP_CDECL GetTransformLocalScale(uint32_t entity, Vector3<float> *scale)
    {
        vke_common::Scene *scene = GetCurrentScene();
        *scale = ToInterop(GetTransform(scene, entity).localScale);
    }

    static void VKE_INTEROP_CDECL SetTransformLocalScale(uint32_t entity, const Vector3<float> *scale)
    {
        vke_common::Scene *scene = GetCurrentScene();
        scene->transformSystem.SetLocalScale(GetEntity(scene, entity), ToGlm(*scale));
    }

    static void VKE_INTEROP_CDECL TranslateTransformLocal(uint32_t entity, const Vector3<float> *det)
    {
        vke_common::Scene *scene = GetCurrentScene();
        scene->transformSystem.TranslateLocal(GetEntity(scene, entity), ToGlm(*det));
    }

    static void VKE_INTEROP_CDECL TranslateTransformGlobal(uint32_t entity, const Vector3<float> *det)
    {
        vke_common::Scene *scene = GetCurrentScene();
        scene->transformSystem.TranslateGlobal(GetEntity(scene, entity), ToGlm(*det));
    }

    static void VKE_INTEROP_CDECL RotateTransformLocal(uint32_t entity, float det, const Vector3<float> *axis)
    {
        vke_common::Scene *scene = GetCurrentScene();
        scene->transformSystem.RotateLocal(GetEntity(scene, entity), det, ToGlm(*axis));
    }

    static void VKE_INTEROP_CDECL RotateTransformGlobal(uint32_t entity, float det, const Vector3<float> *axis)
    {
        vke_common::Scene *scene = GetCurrentScene();
        scene->transformSystem.RotateGlobal(GetEntity(scene, entity), det, ToGlm(*axis));
    }

    static void VKE_INTEROP_CDECL ScaleTransform(uint32_t entity, const Vector3<float> *scale)
    {
        vke_common::Scene *scene = GetCurrentScene();
        scene->transformSystem.Scale(GetEntity(scene, entity), ToGlm(*scale));
    }

    static int32_t VKE_INTEROP_CDECL IsKeyDown(int32_t key)
    {
        return vke_common::InputManager::IsKeyDown(key) ? 1 : 0;
    }

    static int32_t VKE_INTEROP_CDECL IsKeyPressed(int32_t key)
    {
        return vke_common::InputManager::IsKeyPressed(key) ? 1 : 0;
    }

    static int32_t VKE_INTEROP_CDECL IsKeyReleased(int32_t key)
    {
        return vke_common::InputManager::IsKeyReleased(key) ? 1 : 0;
    }

    static int32_t VKE_INTEROP_CDECL IsMouseButtonDown(int32_t button)
    {
        return vke_common::InputManager::IsMouseButtonDown(button) ? 1 : 0;
    }

    static int32_t VKE_INTEROP_CDECL IsMouseButtonPressed(int32_t button)
    {
        return vke_common::InputManager::IsMouseButtonPressed(button) ? 1 : 0;
    }

    static int32_t VKE_INTEROP_CDECL IsMouseButtonReleased(int32_t button)
    {
        return vke_common::InputManager::IsMouseButtonReleased(button) ? 1 : 0;
    }

    static void VKE_INTEROP_CDECL GetMousePosition(Vector2<float> *position)
    {
        *position = ToInterop(vke_common::InputManager::GetMousePosition());
    }

    static void VKE_INTEROP_CDECL GetMouseDelta(Vector2<float> *delta)
    {
        *delta = ToInterop(vke_common::InputManager::GetMouseDelta());
    }

    static void VKE_INTEROP_CDECL GetMouseScrollDelta(Vector2<float> *delta)
    {
        *delta = ToInterop(vke_common::InputManager::GetScrollDelta());
    }

    static void VKE_INTEROP_CDECL SetCursorMode(int32_t mode)
    {
        vke_common::InputManager::SetCursorMode(mode);
    }

    static int32_t VKE_INTEROP_CDECL GetCursorMode()
    {
        return vke_common::InputManager::GetCursorMode();
    }

    static float VKE_INTEROP_CDECL GetTime()
    {
        return vke_common::TimeManager::GetTime();
    }

    static float VKE_INTEROP_CDECL GetDeltaTime()
    {
        return vke_common::TimeManager::GetDeltaTime();
    }

    static float VKE_INTEROP_CDECL GetPreviousFrameTime()
    {
        return vke_common::TimeManager::GetPreviousFrameTime();
    }

    static void VKE_INTEROP_CDECL SetEngineState(int32_t state)
    {
        vke_common::EngineStateManager::SetState(static_cast<vke_common::EngineState>(state));
    }
}

namespace vke_common
{

    void ScriptManager::registerNativeFunctions()
    {
        vke_interop::NativeFunctions nativeFunctions{
            &vke_interop::GetTransformLocalPosition,
            &vke_interop::SetTransformLocalPosition,
            &vke_interop::GetTransformLocalRotation,
            &vke_interop::SetTransformLocalRotation,
            &vke_interop::GetTransformLocalScale,
            &vke_interop::SetTransformLocalScale,
            &vke_interop::TranslateTransformLocal,
            &vke_interop::TranslateTransformGlobal,
            &vke_interop::RotateTransformLocal,
            &vke_interop::RotateTransformGlobal,
            &vke_interop::ScaleTransform,
            &vke_interop::IsKeyDown,
            &vke_interop::IsKeyPressed,
            &vke_interop::IsKeyReleased,
            &vke_interop::IsMouseButtonDown,
            &vke_interop::IsMouseButtonPressed,
            &vke_interop::IsMouseButtonReleased,
            &vke_interop::GetMousePosition,
            &vke_interop::GetMouseDelta,
            &vke_interop::GetMouseScrollDelta,
            &vke_interop::SetCursorMode,
            &vke_interop::GetCursorMode,
            &vke_interop::GetTime,
            &vke_interop::GetDeltaTime,
            &vke_interop::GetPreviousFrameTime,
            &vke_interop::SetEngineState};
        csharpExports.registerNativeFunctions(&nativeFunctions);
    }
}