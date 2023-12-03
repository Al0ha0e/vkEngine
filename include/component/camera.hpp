#ifndef CAMERA_H
#define CAMERA_H

#include <render/render.hpp>
#include <render/buffer.hpp>
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
            : fov(fov), width(width), height(height), aspect(width / height),
              near(near), far(far), Component(obj),
              buffer(sizeof(vke_render::CameraInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            vke_common::TransformParameter &transform = gameObject->transform;
            viewPos = transform.position;
            glm::vec3 gfront = transform.rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            glm::vec3 gup = transform.rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            view = glm::lookAt(viewPos, viewPos + gfront, gup);
            projection = glm::perspective(fov, aspect, near, far);

            vke_render::CameraInfo cameraInfo(view, projection, viewPos);
            buffer.ToBuffer(0, &cameraInfo, sizeof(vke_render::CameraInfo));
            vke_render::Renderer::RegisterCamera(buffer.buffer);
        }

        ~Camera()
        {
            VkDevice logicalDevice = vke_render::RenderEnvironment::GetInstance()->logicalDevice;
            buffer.~HostCoherentBuffer();
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
    };
}

#endif