#include <ui/ui_manager.hpp>
#include <input.hpp>
#include <scene.hpp>
#include <script.hpp>

namespace vke_ui
{
    UIManager *UIManager::instance = nullptr;

    UIManager *UIManager::Init(entt::registry &registry)
    {
        instance = new UIManager(registry);
        return instance;
    }

    void UIManager::Dispose()
    {
        delete instance;
        instance = nullptr;
    }

    void UIManager::Reset() { instance->hovered = {}; instance->pressed = {}; }

    UIComponent *UIManager::resolve(Target target)
    {
        if (!target.handle || !registry.valid(target.entity) ||
            vke_common::SceneManager::GetInstance()->IsPendingDestroy(target.entity)) return nullptr;
        auto *canvas = registry.try_get<vke_component::Canvas>(target.entity);
        return canvas ? canvas->Get(target.handle) : nullptr;
    }

    UIComponent *UIManager::hitSubtree(UIComponent &node, const glm::vec2 &point)
    {
        // Children can extend outside the parent rectangle; layout does not imply clipping.
        for (auto it = node.children.rbegin(); it != node.children.rend(); ++it)
            if (auto *hit = hitSubtree(**it, point)) return hit;
        return node.GetHitTestEnabled() && node.ContainsPoint(point) ? &node : nullptr;
    }

    UIManager::Target UIManager::hitTest(const glm::vec2 &point)
    {
        auto canvases = orderedCanvases();
        for (auto it = canvases.rbegin(); it != canvases.rend(); ++it)
        {
            const auto [entity, canvas] = *it;
            if (vke_common::SceneManager::GetInstance()->IsPendingDestroy(entity)) continue;
            const auto &children = canvas->GetChildren();
            for (auto child = children.rbegin(); child != children.rend(); ++child)
                if (auto *node = hitSubtree(**child, point)) return {entity, node->handle};
        }
        return {};
    }

    void UIManager::dispatch(Target target, PointerEvent event,
                            const glm::vec2 &point, const glm::vec2 &viewportSize)
    {
        auto *node = resolve(target);
        if (!node) return;
        switch (event)
        {
        case PointerEvent::Enter: node->setPointerState(true, node->IsPressed()); break;
        case PointerEvent::Exit: node->setPointerState(false, node->IsPressed()); break;
        case PointerEvent::Down: node->setPointerState(true, true); break;
        case PointerEvent::Up:
        case PointerEvent::Cancel: node->setPointerState(node->IsHovered(), false); break;
        default: break;
        }
        vke_common::ScriptManager::GetUIFunctions().dispatchPointerEvent(
            entt::to_integral(target.entity), target.handle, static_cast<int32_t>(event), point.x, point.y);
        // Handlers may destroy/reparent nodes or change layout. Retain only handles across callbacks.
        Update(viewportSize);
    }

    void UIManager::ProcessPointer(const glm::vec2 &viewportSize, const UIPointerInput &input)
    {
        auto send = [&](Target target, PointerEvent event) { dispatch(target, event, input.position, viewportSize); };
        auto hit = [&]() { return input.active && input.inside ? hitTest(input.position) : Target{}; };
        auto interactiveHit = [&]()
        {
            auto target = hit();
            auto *node = resolve(target);
            return node && node->GetInteractable() ? target : Target{};
        };
        if (pressed.handle)
        {
            auto *node = resolve(pressed);
            if (!input.active || !node || !node->GetHitTestEnabled() || !node->GetInteractable() ||
                !node->IsPressed() || (!input.down && !input.released))
            {
                auto target = std::exchange(pressed, {});
                send(target, PointerEvent::Cancel);
            }
        }
        auto next = interactiveHit();
        const bool enter = next != hovered || (next.handle && !resolve(next)->IsHovered());
        if (enter)
        {
            auto previous = std::exchange(hovered, {});
            send(previous, PointerEvent::Exit);
            hovered = interactiveHit();
            send(hovered, PointerEvent::Enter);
        }
        if (input.active && input.pressed)
        {
            // A handler may remove the hovered node. Do not transfer this press to a newly exposed control.
            const auto current = interactiveHit();
            pressed = current == hovered ? current : Target{};
            send(pressed, PointerEvent::Down);
        }
        if (input.active && input.released && pressed.handle)
        {
            const auto target = std::exchange(pressed, {});
            auto *node = resolve(target);
            if (!node || !node->IsPressed() || !node->GetInteractable() || !node->GetHitTestEnabled())
            {
                send(target, PointerEvent::Cancel);
                return;
            }
            const bool click = target == hit();
            send(target, PointerEvent::Up);
            if (click && target == interactiveHit()) send(target, PointerEvent::Click);
        }
    }

    UIPointerInput UIManager::BuildUIPointerInput(GLFWwindow *window, const glm::vec2 &viewportSize)
    {
        using vke_common::InputManager;
        UIPointerInput input;
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        input.active = width > 0 && height > 0 && viewportSize.x > 0 && viewportSize.y > 0 &&
            glfwGetWindowAttrib(window, GLFW_FOCUSED) && !glfwGetWindowAttrib(window, GLFW_ICONIFIED) &&
            !InputManager::IsFocusLostThisFrame() && InputManager::GetCursorMode() != GLFW_CURSOR_DISABLED;
        if (!input.active) return input;
        input.position = InputManager::GetMousePosition() * viewportSize / glm::vec2(width, height);
        input.inside = input.position.x >= 0 && input.position.y >= 0 &&
            input.position.x < viewportSize.x && input.position.y < viewportSize.y;
        input.down = InputManager::IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        input.pressed = InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
        input.released = InputManager::IsMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT);
        return input;
    }
}
