#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <engine.hpp>

const uint32_t WIDTH = 1024;
const uint32_t HEIGHT = 768;

vke_common::Engine *engine;
vke_common::TransformParameter camParam(glm::vec3(-5.0f, 4.0f, 10.0f), glm::vec3(1), glm::quat(1.0, 0.0, 0.0, 0.0));
vke_common::GameObject *camp = nullptr;

void processInput(GLFWwindow *window, vke_common::GameObject *target);
void mouseCallback(GLFWwindow *window, double xpos, double ypos);

GLFWwindow *initWindow(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
}

int main()
{
    std::vector<vke_render::PassType> passes = {
        vke_render::GBUFFER_PASS,
        vke_render::DEFERRED_LIGHTING_PASS,
        vke_render::SKYBOX_RENDERER};
    std::vector<std::unique_ptr<vke_render::RenderPassBase>> customPasses;
    GLFWwindow *window = initWindow(WIDTH, HEIGHT);
    engine = vke_common::Engine::Init(window, nullptr, passes, customPasses);
    vke_common::AssetManager::LoadAssetLUT("./tests/scene/test_anim_desc.json");
    vke_common::SceneManager::LoadScene("./tests/scene/test_anim_scene.json");
    camp = vke_common::SceneManager::GetInstance()->currentScene->objects[1].get();

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetFramebufferSizeCallback(window, vke_common::Engine::OnWindowResize);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        processInput(window, camp);
        engine->Update();
    }
    vke_common::Engine::WaitIdle();
    vke_common::Engine::Dispose();

    return 0;
}

#define CHECK_KEY(x) if (glfwGetKey(window, x) == GLFW_PRESS)

float moveSpeed = 2.5f;
float rotateSpeed = 1.0f;

void processInput(GLFWwindow *window, vke_common::GameObject *target)
{
    float time_delta = vke_common::TimeManager::GetInstance()->deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    CHECK_KEY(GLFW_KEY_W)
    {
        target->TranslateLocal(glm::vec3(0, 0, -moveSpeed * time_delta));
    }
    CHECK_KEY(GLFW_KEY_A)
    {
        target->TranslateLocal(glm::vec3(-moveSpeed * time_delta, 0, 0));
    }
    CHECK_KEY(GLFW_KEY_S)
    {
        target->TranslateLocal(glm::vec3(0, 0, moveSpeed * time_delta));
    }
    CHECK_KEY(GLFW_KEY_D)
    {
        target->TranslateLocal(glm::vec3(moveSpeed * time_delta, 0, 0));
    }
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    float time_delta = vke_common::TimeManager::GetInstance()->deltaTime;
    static bool firstMouse = true;
    static float lastX, lastY;
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camp->RotateGlobal(-xoffset * rotateSpeed * time_delta, glm::vec3(0.0f, 1.0f, 0.0f));
    camp->RotateLocal(yoffset * rotateSpeed * time_delta, glm::vec3(1.0f, 0.0f, 0.0f));
}