#ifndef CAMERA_H
#define CAMERA_H

#include <render/render.hpp>
#include <gameobject.hpp>

namespace vke_component
{
    struct CameraInfo
    {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 viewPos;
    };

    class Camera : public vke_common::Component
    {
    public:
        float fov;
        float width;
        float height;
        float near;
        float far;
        glm::mat4 view;
        VkBuffer buffer;

        Camera(float fov, float width, float height,
               float near, float far, vke_common::GameObject *obj)
            : fov(fov), width(width), height(height), near(near), far(far), Component(obj)
        {
            vke_common::TransformParameter &transform = gameObject->transform;
            glm::vec3 gfront = transform.rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            glm::vec3 gup = transform.rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            view = glm::lookAt(transform.position, transform.position + gfront, gup);
        }

        void OnTransformed(vke_common::TransformParameter &param) override
        {
        }
    };
}

#endif