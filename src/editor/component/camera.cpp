#include <editor/editor.hpp>
#include <component/camera.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

namespace vke_editor
{
    static void UpdateCameraProjection(vke_component::Camera &camera)
    {
        camera.cameraInfo.projection = glm::perspective(
            camera.cameraInfo.fov,
            camera.cameraInfo.aspect,
            camera.cameraInfo.near,
            camera.cameraInfo.far);
        camera.cameraInfo.projection[1][1] *= -1.0f;
        camera.cameraInfo.invProjection = glm::inverse(camera.cameraInfo.projection);

        if (vke_render::Renderer::GetInstance()->currentCamera == camera.id)
            vke_render::Renderer::UpdateCameraInfo(camera.cameraInfo);
    }

    static void DrawReadOnlyFloat(const char *label, float value)
    {
        ImGui::BeginDisabled();
        ImGui::InputFloat(label, &value, 0.0f, 0.0f, "%.3f");
        ImGui::EndDisabled();
    }

    void Editor::drawCameraComponent(vke_common::Scene *scene)
    {
        if (scene == nullptr || selectedEntity == entt::null ||
            !scene->registry.all_of<vke_component::Camera>(selectedEntity))
            return;

        if (!ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        vke_component::Camera &camera = scene->registry.get<vke_component::Camera>(selectedEntity);
        float fov = glm::degrees(camera.cameraInfo.fov);
        float nearPlane = camera.cameraInfo.near;
        float farPlane = camera.cameraInfo.far;

        bool changed = false;
        changed |= ImGui::InputFloat("FOV", &fov, 1.0f, 5.0f, "%.2f deg");
        changed |= ImGui::InputFloat("Near", &nearPlane, 0.01f, 0.1f, "%.4f");
        changed |= ImGui::InputFloat("Far", &farPlane, 1.0f, 10.0f, "%.3f");

        if (changed)
        {
            fov = glm::clamp(fov, 1.0f, 179.0f);
            nearPlane = glm::max(nearPlane, 0.0001f);
            farPlane = glm::max(farPlane, nearPlane + 0.0001f);

            camera.cameraInfo.fov = glm::radians(fov);
            camera.cameraInfo.near = nearPlane;
            camera.cameraInfo.far = farPlane;
            UpdateCameraProjection(camera);
        }

        DrawReadOnlyFloat("Aspect", camera.cameraInfo.aspect);
        DrawReadOnlyFloat("Width", camera.width);
        DrawReadOnlyFloat("Height", camera.height);

        ImGui::TreePop();
    }
}
