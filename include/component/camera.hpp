#ifndef CAMERA_H
#define CAMERA_H

#include <render/render.hpp>
#include <render/buffer.hpp>
#include <event.hpp>
#include <component/transform.hpp>

#ifdef _MINWINDEF_
#undef near
#undef far
#endif

namespace vke_component
{
    class Camera
    {
    public:
        vke_ds::id32_t id;
        float fov;
        float width;
        float height;
        float near;
        float far;
        float aspect;
        vke_render::CameraInfo cameraInfo;

        Camera(const vke_common::Transform &transform,
               float fov, float width, float height,
               float near, float far)
            : id(0), fov(glm::radians(fov)), width(width), height(height), aspect(width / height),
              near(near), far(far)
        {
            init(transform);
        }

        Camera(const vke_common::Transform &transform, const nlohmann::json &json)
            : id(0), fov(glm::radians((float)json["fov"])),
              width(json["width"]), height(json["height"]), aspect(width / height),
              near(json["near"]), far(json["far"])
        {
            init(transform);
        }

        ~Camera()
        {
            vke_render::Renderer::RemoveCamera(id);
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderer->resizeEventHub.RemoveEventListener(resizeListenerID);
        }

        nlohmann::json ToJSON()
        {
            nlohmann::json ret = {
                {"type", "camera"},
                {"fov", glm::degrees(fov)},
                {"width", width},
                {"height", height},
                {"near", near},
                {"far", far}};
            return ret;
        }

        void OnTransformed(vke_common::Transform &transform)
        {
            const glm::vec3 position = transform.GetGlobalPosition();
            const glm::quat rotation = transform.GetGlobalRotation();
            const glm::vec3 gfront = rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            const glm::vec3 gup = rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            cameraInfo.viewPos = glm::vec4(position, 0.0f);
            cameraInfo.view = glm::lookAt(position, position + gfront, gup);
            if (vke_render::Renderer::GetInstance()->currentCamera == id)
                updateCameraInfo();
        }

        void UpdateProjection(uint32_t w, uint32_t h)
        {
            width = w;
            height = h;
            aspect = width / height;
            cameraInfo.projection = glm::perspective(fov, aspect, near, far);
            cameraInfo.projection[1][1] *= -1;
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
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            resizeListenerID = renderer->resizeEventHub.AddEventListener(
                this,
                vke_common::EventHub<glm::vec2>::callback_t(OnWindowResize));

            const glm::vec3 position = transform.GetGlobalPosition();
            const glm::quat rotation = transform.GetGlobalRotation();
            const glm::vec3 gfront = rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            const glm::vec3 gup = rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

            cameraInfo.viewPos = glm::vec4(position, 0.0f);
            cameraInfo.view = glm::lookAt(position, position + gfront, gup);
            cameraInfo.projection = glm::perspective(fov, aspect, near, far);
            cameraInfo.projection[1][1] *= -1;

            std::function<void()> callback = std::bind(&Camera::onCameraSelected, this);
            id = vke_render::Renderer::RegisterCamera(callback);
        }

        void onCameraSelected()
        {
            updateCameraInfo();
        }

        void updateCameraInfo()
        {
            vke_render::Renderer::UpdateCameraInfo(&cameraInfo);
        }
    };
}

#ifdef _MINWINDEF_
#define near
#define far
#endif

#endif