#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <render/render.hpp>
#include <render/buffer.hpp>
#include <event.hpp>
#include <component/transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#ifdef _MINWINDEF_
#undef near
#undef far
#endif

namespace vke_component
{
    struct CameraData
    {
        float fovRadians;
        float width;
        float height;
        float aspect;
        float nearPlane;
        float farPlane;

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

        void LoadToEngine()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            resizeListenerID = renderer->resizeEventHub.AddEventListener(
                this,
                vke_common::EventHub<glm::vec2>::callback_t(OnWindowResize));
            std::function<void()> callback = std::bind(&Camera::onCameraSelected, this);
            id = vke_render::Renderer::RegisterCamera(callback);
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

        static void OnWindowResize(void *listener, glm::vec2 *info)
        {
            Camera *cam = (Camera *)listener;
            cam->UpdateProjection(info->x, info->y);
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

        void onCameraSelected()
        {
            updateCameraInfo();
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
