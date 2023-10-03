#ifndef CAMERA_H
#define CAMERA_H

#include <render/render.hpp>
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
        VkBuffer buffer;

        Camera(float fov, float width, float height,
               float near, float far, vke_common::GameObject *obj)
            : fov(fov), width(width), height(height), aspect(width / height), near(near), far(far), Component(obj)
        {
            vke_common::TransformParameter &transform = gameObject->transform;
            viewPos = transform.position;
            glm::vec3 gfront = transform.rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            glm::vec3 gup = transform.rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            view = glm::lookAt(viewPos, viewPos + gfront, gup);
            projection = glm::perspective(fov, aspect, near, far);

            vke_render::CameraInfo cameraInfo(view, projection, viewPos);

            VkDeviceSize bufferSize = sizeof(vke_render::CameraInfo);
            vke_render::RenderEnvironment::CreateBuffer(
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                buffer, bufferMemory);
            vkMapMemory(
                vke_render::RenderEnvironment::GetInstance()->logicalDevice,
                bufferMemory,
                0,
                bufferSize,
                0,
                &mappedBufferMemory);

            memcpy(mappedBufferMemory, &cameraInfo, sizeof(vke_render::CameraInfo));

            vke_render::OpaqueRenderer::RegisterCamera(buffer);
        }

        ~Camera()
        {
            VkDevice logicalDevice = vke_render::RenderEnvironment::GetInstance()->logicalDevice;
            vkDestroyBuffer(logicalDevice, buffer, nullptr);
            vkFreeMemory(logicalDevice, bufferMemory, nullptr);
        }

        void OnTransformed(vke_common::TransformParameter &param) override
        {
            glm::vec3 gfront = param.rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            glm::vec3 gup = param.rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            viewPos = param.position;
            view = glm::lookAt(viewPos, viewPos + gfront, gup);
            vke_render::CameraInfo cameraInfo(view, projection, viewPos);
            memcpy(mappedBufferMemory, &cameraInfo, sizeof(vke_render::CameraInfo));
        }

    private:
        VkDeviceMemory bufferMemory;
        void *mappedBufferMemory;
    };
}

#endif