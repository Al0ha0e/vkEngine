#ifndef COMPONENT_H
#define COMPONENT_H

#include <cstdint>

namespace vke_common
{
    enum class ComponentType : int32_t
    {
        Transform = 0,
        Camera = 1,
        RenderableObject = 2,
        SkeletonAnimator = 3,
        RigidBody = 4,
        CharacterController = 5,
        DirectionalLight = 6,
        PointLight = 7,
        SpotLight = 8,
        Script = 9
    };
}

#endif
