#ifndef ANIMATION_H
#define ANIMATION_H

#include <common.hpp>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/track.h>
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
        bool hasRootMotion;
        ozz::animation::Float3Track rootMotionPosition;
        ozz::animation::QuaternionTrack rootMotionRotation;

        Animation() : handle(0), hasRootMotion(false) {}
        Animation(vke_common::AssetHandle hdl) : handle(hdl), hasRootMotion(false) {}

        float Duration() const
        {
            return animation.duration();
        }
    };
}

#endif
