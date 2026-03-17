#ifndef CAMERA_H
#define CAMERA_H

#ifdef _MINWINDEF_
#undef near
#undef far
#endif

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace vke_render
{
    struct CameraInfo
    {
        float near;
        float far;
        float fov;
        float aspect;
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 viewPos;

        CameraInfo() = default;

        CameraInfo(float near, float far, float fov, float aspect,
                   glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos)
            : near(near), far(far), fov(fov), aspect(aspect),
              view(view), projection(projection), viewPos(viewPos, 1.0) {}

        CameraInfo(float near, float far, float fov, float aspect)
            : near(near), far(far), fov(fov), aspect(aspect),
              view(0), projection(0), viewPos(0) {}
    };
}

#ifdef _MINWINDEF_
#define near
#define far
#endif

#endif