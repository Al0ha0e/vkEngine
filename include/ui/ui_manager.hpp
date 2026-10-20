#pragma once

#include <component/canvas.hpp>
#include <ui/ui.hpp>
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
        static UIManager *GetInstance()
        {
            VKE_FATAL_IF(instance == nullptr, "UIManager not initialized!")
            return instance;
        }
        static UIManager *Init(entt::registry &registry);
        static void Dispose();
        static void Reset();

        void Update(const glm::vec2 &viewportSize)
        {
            auto canvases = orderedCanvases();
            uint32_t drawOrder = 0;
            for (auto [entity, canvas] : canvases) canvas->Update(viewportSize, drawOrder);
        }
        void ProcessPointer(const glm::vec2 &viewportSize, const UIPointerInput &input);
        static UIPointerInput BuildUIPointerInput(GLFWwindow *window, const glm::vec2 &viewportSize);

    private:
        static UIManager *instance;
        entt::registry &registry;

        UIManager(entt::registry &registry) : registry(registry) {}
        ~UIManager() = default;
        UIManager(const UIManager &) = delete;
        UIManager &operator=(const UIManager &) = delete;

        struct Target
        {
            entt::entity entity = entt::null;
            UIHandle handle = 0;
            bool operator==(const Target &) const = default;
        };
        Target hovered;
        Target pressed;
        UIComponent *resolve(Target target);
        Target hitTest(const glm::vec2 &point);
        static UIComponent *hitSubtree(UIComponent &node, const glm::vec2 &point);
        void dispatch(Target target, PointerEvent event,
                      const glm::vec2 &point, const glm::vec2 &viewportSize);
        std::vector<std::pair<entt::entity, vke_component::Canvas *>> orderedCanvases()
        {
            std::vector<std::pair<entt::entity, vke_component::Canvas *>> canvases;
            for (auto [entity, canvas] : registry.view<vke_component::Canvas>().each())
                canvases.emplace_back(entity, &canvas);
            std::sort(canvases.begin(), canvases.end(), [](const auto &a, const auto &b)
            {
                if (a.second->sortOrder != b.second->sortOrder) return a.second->sortOrder < b.second->sortOrder;
                return entt::to_integral(a.first) < entt::to_integral(b.first);
            });
            return canvases;
        }
    };
}
