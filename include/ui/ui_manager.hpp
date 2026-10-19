#pragma once

#include <component/canvas.hpp>
#include <entt/entity/registry.hpp>
#include <glm/vec2.hpp>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace vke_ui
{
    class UIManager
    {
    public:
        static void Update(entt::registry &registry, const glm::vec2 &viewportSize)
        {
            std::vector<std::pair<entt::entity, vke_component::Canvas *>> canvases;
            for (auto [entity, canvas] : registry.view<vke_component::Canvas>().each())
                canvases.emplace_back(entity, &canvas);
            std::sort(canvases.begin(), canvases.end(), [](const auto &a, const auto &b)
            {
                if (a.second->sortOrder != b.second->sortOrder) return a.second->sortOrder < b.second->sortOrder;
                return entt::to_integral(a.first) < entt::to_integral(b.first);
            });
            uint32_t drawOrder = 0;
            for (auto [entity, canvas] : canvases) canvas->Update(viewportSize, drawOrder);
        }
    };
}
