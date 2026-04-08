#include <input.hpp>

namespace vke_common
{
    InputManager *InputManager::instance = nullptr;

    InputManager::InputManager()
        : window(nullptr),
          keyDown{},
          keyPressed{},
          keyReleased{},
          mouseButtonDown{},
          mouseButtonPressed{},
          mouseButtonReleased{},
          mousePosition(0.0f, 0.0f),
          mouseDelta(0.0f, 0.0f),
          scrollDelta(0.0f, 0.0f),
          hasMousePosition(false)
    {
    }

    InputManager *InputManager::Init(GLFWwindow *window)
    {
        if (instance == nullptr)
            instance = new InputManager();

        instance->window = window;
        instance->ClearTransientState();
        instance->keyDown.fill(0);
        instance->mouseButtonDown.fill(0);
        instance->mousePosition = glm::vec2(0.0f, 0.0f);
        instance->hasMousePosition = false;

        glfwSetKeyCallback(window, &InputManager::KeyCallback);
        glfwSetCursorPosCallback(window, &InputManager::CursorPosCallback);
        glfwSetMouseButtonCallback(window, &InputManager::MouseButtonCallback);
        glfwSetScrollCallback(window, &InputManager::ScrollCallback);
        return instance;
    }

    void InputManager::Dispose()
    {
        if (instance == nullptr)
            return;

        delete instance;
        instance = nullptr;
    }

    void InputManager::EndFrame()
    {
        RequireInstance().ClearTransientState();
    }

    void InputManager::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        (void)window;
        (void)scancode;
        (void)mods;

        InputManager &input = RequireInstance();
        if (!IsKeyInRange(key))
            return;

        if (action == GLFW_PRESS)
        {
            if (input.keyDown[key] == 0)
                input.keyPressed[key] = 1;
            input.keyDown[key] = 1;
            return;
        }

        if (action == GLFW_RELEASE)
        {
            if (input.keyDown[key] != 0)
                input.keyReleased[key] = 1;
            input.keyDown[key] = 0;
        }
    }

    void InputManager::CursorPosCallback(GLFWwindow *window, double xpos, double ypos)
    {
        (void)window;

        InputManager &input = RequireInstance();
        glm::vec2 current(static_cast<float>(xpos), static_cast<float>(ypos));

        if (input.hasMousePosition)
            input.mouseDelta += current - input.mousePosition;

        input.mousePosition = current;
        input.hasMousePosition = true;
    }

    void InputManager::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
    {
        (void)window;
        (void)mods;

        InputManager &input = RequireInstance();
        if (!IsMouseButtonInRange(button))
            return;

        if (action == GLFW_PRESS)
        {
            if (input.mouseButtonDown[button] == 0)
                input.mouseButtonPressed[button] = 1;
            input.mouseButtonDown[button] = 1;
            return;
        }

        if (action == GLFW_RELEASE)
        {
            if (input.mouseButtonDown[button] != 0)
                input.mouseButtonReleased[button] = 1;
            input.mouseButtonDown[button] = 0;
        }
    }

    void InputManager::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
    {
        (void)window;

        InputManager &input = RequireInstance();
        input.scrollDelta += glm::vec2(static_cast<float>(xoffset), static_cast<float>(yoffset));
    }

    bool InputManager::IsKeyDown(int key)
    {
        if (!IsKeyInRange(key))
            return false;
        return RequireInstance().keyDown[key] != 0;
    }

    bool InputManager::IsKeyPressed(int key)
    {
        if (!IsKeyInRange(key))
            return false;
        return RequireInstance().keyPressed[key] != 0;
    }

    bool InputManager::IsKeyReleased(int key)
    {
        if (!IsKeyInRange(key))
            return false;
        return RequireInstance().keyReleased[key] != 0;
    }

    bool InputManager::IsMouseButtonDown(int button)
    {
        if (!IsMouseButtonInRange(button))
            return false;
        return RequireInstance().mouseButtonDown[button] != 0;
    }

    bool InputManager::IsMouseButtonPressed(int button)
    {
        if (!IsMouseButtonInRange(button))
            return false;
        return RequireInstance().mouseButtonPressed[button] != 0;
    }

    bool InputManager::IsMouseButtonReleased(int button)
    {
        if (!IsMouseButtonInRange(button))
            return false;
        return RequireInstance().mouseButtonReleased[button] != 0;
    }

    glm::vec2 InputManager::GetMousePosition()
    {
        return RequireInstance().mousePosition;
    }

    glm::vec2 InputManager::GetMouseDelta()
    {
        return RequireInstance().mouseDelta;
    }

    glm::vec2 InputManager::GetScrollDelta()
    {
        return RequireInstance().scrollDelta;
    }

    void InputManager::SetCursorMode(int mode)
    {
        glfwSetInputMode(RequireInstance().window, GLFW_CURSOR, mode);
    }

    int InputManager::GetCursorMode()
    {
        return glfwGetInputMode(RequireInstance().window, GLFW_CURSOR);
    }

    InputManager &InputManager::RequireInstance()
    {
        return *instance;
    }

    bool InputManager::IsKeyInRange(int key)
    {
        return key >= 0 && key < KEY_CAPACITY;
    }

    bool InputManager::IsMouseButtonInRange(int button)
    {
        return button >= 0 && button < MOUSE_BUTTON_CAPACITY;
    }

    void InputManager::ClearTransientState()
    {
        keyPressed.fill(0);
        keyReleased.fill(0);
        mouseButtonPressed.fill(0);
        mouseButtonReleased.fill(0);
        mouseDelta = glm::vec2(0.0f, 0.0f);
        scrollDelta = glm::vec2(0.0f, 0.0f);
    }
}
