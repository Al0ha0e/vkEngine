#ifndef CAMERA_H
#define CAMERA_H

#include <render/render.hpp>
#include <render/buffer.hpp>
#include <event.hpp>
#include <gameobject.hpp>

namespace vke_component
{
    class Camera : public vke_common::Component
    {
    public:
        float fov;
        float width;
        float height;
        float near;
        float far;
        float aspect;
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 viewPos;
        vke_render::HostCoherentBuffer buffer;

        Camera(float fov, float width, float height,
               float near, float far, vke_common::GameObject *obj)
            : fov(glm::radians(fov)), width(width), height(height), aspect(width / height),
              near(near), far(far), Component(obj),
              buffer(sizeof(vke_render::CameraInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            init();
        }

        Camera(vke_common::GameObject *obj, nlohmann::json &json)
            : fov(glm::radians((float)json["fov"])), width(json["width"]), height(json["height"]), aspect(width / height),
              near(json["near"]), far(json["far"]), Component(obj),
              buffer(sizeof(vke_render::CameraInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            init();
        }

        ~Camera()
        {
            vke_common::EventSystem::RemoveEventListener(vke_common::EVENT_WINDOW_RESIZE, resizeListenerID);
        }

        std::string ToJSON() override
        {
            std::string ret = "{\n\"type\":\"camera\",\n";
            ret += "\"fov\": " + std::to_string(glm::degrees(fov)) + ",\n";
            ret += "\"width\": " + std::to_string(width) + ",\n";
            ret += "\"height\": " + std::to_string(height) + ",\n";
            ret += "\"near\": " + std::to_string(near) + ",\n";
            ret += "\"far\": " + std::to_string(far);
            ret += "\n}";
            return ret;
        }

        void OnTransformed(vke_common::TransformParameter &param) override
        {
            glm::vec3 gfront = param.rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            glm::vec3 gup = param.rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            viewPos = param.position;
            view = glm::lookAt(viewPos, viewPos + gfront, gup);
            vke_render::CameraInfo cameraInfo(view, projection, viewPos);
            buffer.ToBuffer(0, &cameraInfo, sizeof(vke_render::CameraInfo));
        }

        void UpdateProjection(uint32_t w, uint32_t h)
        {
            width = w;
            height = h;
            aspect = width / height;
            projection = glm::perspective(fov, aspect, near, far);
            projection[1][1] *= -1;
            vke_render::CameraInfo cameraInfo(view, projection, viewPos);
            buffer.ToBuffer(0, &cameraInfo, sizeof(vke_render::CameraInfo));
        }

        static void OnWindowResize(void *listener, glm::vec2 *info)
        {
            Camera *cam = (Camera *)listener;
            cam->UpdateProjection(info->x, info->y);
        }

    private:
        int resizeListenerID;

        void init()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            resizeListenerID = renderer->resizeEventHub.AddEventListener(
                this,
                vke_common::EventHub<glm::vec2>::callback_t(OnWindowResize));

            vke_common::TransformParameter &transform = gameObject->transform;
            viewPos = transform.position;
            glm::vec3 gfront = transform.rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            glm::vec3 gup = transform.rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            view = glm::lookAt(viewPos, viewPos + gfront, gup);
            projection = glm::perspective(fov, aspect, near, far);
            projection[1][1] *= -1;

            vke_render::CameraInfo cameraInfo(view, projection, viewPos);
            buffer.ToBuffer(0, &cameraInfo, sizeof(vke_render::CameraInfo));
            vke_render::Renderer::RegisterCamera(buffer.buffer);
        }
    };
}

#endif