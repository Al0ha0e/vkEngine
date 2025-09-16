#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H
#include <common.hpp>
#include <glm/vec2.hpp>

namespace vke_common
{
    enum KeyCode
    {
        KEY_0 = GLFW_KEY_0,
        KEY_A = GLFW_KEY_A,
        KEY_D = GLFW_KEY_D,
        KEY_S = GLFW_KEY_S,
        KEY_W = GLFW_KEY_W,
        // TODO
    };

    struct MouseEventBody
    {
        int button;
        int action;
    };

    class InputManager
    {
    private:
        static InputManager *instance;
        InputManager() : leftMouseDown(false), rightMouseDown(false) {}
        ~InputManager() {}
        InputManager(const InputManager &);
        InputManager &operator=(const InputManager);

    public:
        bool leftMouseDown;
        bool rightMouseDown;
        glm::vec2 mousePos;

        static InputManager *GetInstance()
        {
            return instance;
        }

        static InputManager *Init(GLFWwindow *window)
        {
            instance = new InputManager();
            instance->window = window;
            return instance;
        }

        static void Dispose()
        {
            delete instance;
        }

        static void CursorPosCallback(double xpos, double ypos);

        static void MouseButtonCallback(int button, int action, int mods);

        static int GetKeyStatus(KeyCode key);

        static bool KeyPressed(KeyCode key) { return GetKeyStatus(key) == GLFW_PRESS; }

    private:
        GLFWwindow *window;
    };

}

#endif
