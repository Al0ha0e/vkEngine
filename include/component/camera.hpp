#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <json_validation.hpp>
#include <render/render.hpp>
#include <render/buffer.hpp>
#include <event.hpp>
#include <component/transform.hpp>
#include <entt/entity/registry.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#ifdef _MINWINDEF_
#undef near
#undef far
#endif

namespace vke_component
{
    struct CameraData
    {
        float fovRadians = glm::radians(60.0f);
        float width = 1280.0f;
        float height = 720.0f;
        float aspect = width / height;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;

        // Call before constructing from JSON; asset checks happen in LoadAssets where applicable.
        static vke_common::SceneResult<void> ValidateJSON(const nlohmann::json &json)
        {
            using namespace vke_common::json_validation;
            auto result = Object(json).Require({"fov", "width", "height", "near", "far"}).Numbers({"fov", "width", "height", "near", "far"}).Result();
            if (!result)
                return result;
            if (json["height"].get<float>() <= 0 || json["width"].get<float>() <= 0 ||
                json["near"].get<float>() <= 0 || json["far"].get<float>() <= json["near"].get<float>())
                return std::unexpected("invalid camera dimensions or clipping planes");
            return {};
        }

        CameraData() = default;
        CameraData(const nlohmann::json &json)
            : fovRadians(glm::radians(json["fov"].get<float>())),
              width(json["width"]), height(json["height"]), aspect(width / height),
              nearPlane(json["near"]), farPlane(json["far"]) {}
        nlohmann::json ToJSON() const
        {
            return {{"type", "camera"},
                    {"fov", glm::degrees(fovRadians)},
                    {"width", width},
                    {"height", height},
                    {"near", nearPlane},
                    {"far", farPlane}};
        }
    };

    class Camera // TODO only CameraInfo in renderer
    {
    public:
        vke_ds::id32_t id;
        float width;
        float height;
        vke_render::CameraInfo cameraInfo;

        Camera(const vke_common::Transform &transform,
               float fov, float width, float height,
               float near, float far)
            : id(0), cameraInfo(near, far, glm::radians(fov), width / height),
              width(width), height(height), resizeListenerID(0)
        {
            init(transform);
        }

        Camera(const vke_common::Transform &transform, const CameraData &componentData)
            : id(0),
              cameraInfo(componentData.nearPlane, componentData.farPlane,
                         componentData.fovRadians, componentData.aspect),
              width(componentData.width), height(componentData.height), resizeListenerID(0)
        {
            init(transform);
        }

        ~Camera() {}

        void FillData(CameraData &data) const
        {
            data.fovRadians = cameraInfo.fov;
            data.width = width;
            data.height = height;
            data.aspect = cameraInfo.aspect;
            data.nearPlane = cameraInfo.near;
            data.farPlane = cameraInfo.far;
        }

        void LoadToEngine(entt::registry &registry, entt::entity entity)
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            // Resolve the component at dispatch time: packed storage can relocate it.
            resizeListenerID = renderer->resizeEventHub.AddEventListener(
                nullptr, [&registry, entity](void *, glm::vec2 *size)
                {
                    if (auto *camera = registry.try_get<Camera>(entity))
                        camera->UpdateProjection(size->x, size->y); });
            id = vke_render::Renderer::RegisterCamera([&registry, entity]()
                                                      {
                    if (auto *camera = registry.try_get<Camera>(entity))
                        camera->updateCameraInfo(); });
        }

        void UnloadFromEngine()
        {
            vke_render::Renderer::RemoveCamera(id);
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderer->resizeEventHub.RemoveEventListener(resizeListenerID);
        }

        void OnTransformed(vke_common::Transform &transform)
        {
            const glm::vec3 position = transform.GetGlobalPosition();
            const glm::quat rotation = transform.GetGlobalRotation();
            const glm::vec3 gfront = rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            const glm::vec3 gup = rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            cameraInfo.viewPos = glm::vec4(position, 0.0f);
            cameraInfo.view = glm::lookAt(position, position + gfront, gup);
            cameraInfo.invView = glm::inverse(cameraInfo.view);
            if (vke_render::Renderer::GetInstance()->currentCamera == id)
                updateCameraInfo();
        }

        void UpdateProjection(uint32_t w, uint32_t h)
        {
            width = static_cast<float>(w);
            height = static_cast<float>(h);
            cameraInfo.aspect = width / height;
            cameraInfo.projection = glm::perspective(cameraInfo.fov, cameraInfo.aspect, cameraInfo.near, cameraInfo.far);
            cameraInfo.projection[1][1] *= -1;
            cameraInfo.invProjection = glm::inverse(cameraInfo.projection);
            if (vke_render::Renderer::GetInstance()->currentCamera == id)
                updateCameraInfo();
        }

    private:
        vke_ds::id32_t resizeListenerID;

        void init(const vke_common::Transform &transform)
        {
            const glm::vec3 position = transform.GetGlobalPosition();
            const glm::quat rotation = transform.GetGlobalRotation();
            const glm::vec3 gfront = rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            const glm::vec3 gup = rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

            cameraInfo.viewPos = glm::vec4(position, 0.0f);
            cameraInfo.view = glm::lookAt(position, position + gfront, gup);
            cameraInfo.projection = glm::perspective(cameraInfo.fov, cameraInfo.aspect, cameraInfo.near, cameraInfo.far);
            cameraInfo.projection[1][1] *= -1;
            cameraInfo.invView = glm::inverse(cameraInfo.view);
            cameraInfo.invProjection = glm::inverse(cameraInfo.projection);
        }

        void updateCameraInfo()
        {
            vke_render::Renderer::UpdateCameraInfo(cameraInfo);
        }
    };
}

#ifdef _MINWINDEF_
#define near
#define far
#endif

#endif
