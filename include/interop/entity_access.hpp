#ifndef VKE_INTEROP_ENTITY_ACCESS_HPP
#define VKE_INTEROP_ENTITY_ACCESS_HPP

#include <scene.hpp>

namespace vke_interop
{
    inline bool IsEntityValid(uint32_t entity)
    {
        return vke_common::SceneManager::GetInstance()->registry.valid(static_cast<entt::entity>(entity));
    }
}

#endif
