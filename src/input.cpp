#include <input.hpp>
#include <event.hpp>
#include <iostream>

namespace vke_common
{
    InputManager *InputManager::instance;

    void InputManager::CursorPosCallback(double xpos, double ypos)
    {
        instance->mousePos = glm::vec2(xpos, ypos);
    }

    void InputManager::MouseButtonCallback(int button, int action, int mods)
    {
        MouseEventBody event = {button, action};
        EventSystem::DispatchEvent(EVENT_MOUSE_CLICK, &event);
        if (button == 0)
        {
            instance->leftMouseDown = action;
        }
        if (button == 1)
        {
            instance->rightMouseDown = action;
        }
    }

    int InputManager::GetKeyStatus(KeyCode key)
    {
        return glfwGetKey(instance->window, key);
    }
}