#ifndef ANIMATION_H
#define ANIMATION_H

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

namespace vke_common
{

    class Skeleton
    {
    public:
        vke_common::AssetHandle handle;
        ozz::animation::Skeleton skeleton;

        Skeleton() = default;
        Skeleton(vke_common::AssetHandle hdl) : handle(hdl) {}
    };

    class Animation
    {
    public:
        vke_common::AssetHandle handle;
        ozz::animation::Animation animation;

        Animation() = default;
        Animation(vke_common::AssetHandle hdl) : handle(hdl) {}

        float Duration() const
        {
            return animation.duration();
        }
    };
}

#endif