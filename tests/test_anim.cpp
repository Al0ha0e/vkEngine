#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <engine.hpp>

const uint32_t WIDTH = 1024;
const uint32_t HEIGHT = 768;

vke_common::Engine *engine;
entt::entity camera = entt::null;
vke_common::Scene *currentScene = nullptr;

void processInput(GLFWwindow *window, entt::entity target);
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
    currentScene = vke_common::SceneManager::GetInstance()->currentScene.get();
    camera = currentScene->GetObjectEntity(1);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetFramebufferSizeCallback(window, vke_common::Engine::OnWindowResize);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        processInput(window, camera);
        engine->Update();
    }
    vke_common::Engine::WaitIdle();
    vke_common::Engine::Dispose();

    return 0;
}

#define CHECK_KEY(x) if (glfwGetKey(window, x) == GLFW_PRESS)

float moveSpeed = 2.5f;
float rotateSpeed = 1.0f;

void processInput(GLFWwindow *window, entt::entity target)
{
    float time_delta = vke_common::TimeManager::GetInstance()->deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    CHECK_KEY(GLFW_KEY_W)
    {
        currentScene->TranslateLocal(target, glm::vec3(0, 0, -moveSpeed * time_delta));
    }
    CHECK_KEY(GLFW_KEY_A)
    {
        currentScene->TranslateLocal(target, glm::vec3(-moveSpeed * time_delta, 0, 0));
    }
    CHECK_KEY(GLFW_KEY_S)
    {
        currentScene->TranslateLocal(target, glm::vec3(0, 0, moveSpeed * time_delta));
    }
    CHECK_KEY(GLFW_KEY_D)
    {
        currentScene->TranslateLocal(target, glm::vec3(moveSpeed * time_delta, 0, 0));
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
    currentScene->RotateGlobal(camera, -xoffset * rotateSpeed * time_delta, glm::vec3(0.0f, 1.0f, 0.0f));
    currentScene->RotateLocal(camera, yoffset * rotateSpeed * time_delta, glm::vec3(1.0f, 0.0f, 0.0f));
}