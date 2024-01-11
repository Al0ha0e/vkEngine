#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <engine.hpp>
#include <gameobject.hpp>
#include <component/camera.hpp>
#include <component/renderable_object.hpp>

const uint32_t WIDTH = 1024;
const uint32_t HEIGHT = 768;

vke_common::Engine *engine;
float time_prev, time_delta;
vke_common::TransformParameter camParam(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, glm::radians(0.0f), 0.0f));
vke_common::GameObject *camp = nullptr;

void processInput(GLFWwindow *window, vke_common::GameObject *target);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

int main()
{
    std::vector<vke_render::PassType> passes = {
        vke_render::BASE_RENDERER,
        vke_render::OPAQUE_RENDERER};
    std::vector<std::unique_ptr<vke_render::SubpassBase>> customPasses;
    std::vector<vke_render::RenderPassInfo> customPassInfo;
    engine = vke_common::Engine::Init(WIDTH, HEIGHT, passes, customPasses, customPassInfo);

    vke_common::TransformParameter targetParam(glm::vec3(-0.5f, 0.5f, -1.0f), glm::vec3(0.0f, 0.0f, glm::radians(30.0f)));
    vke_common::TransformParameter targetParam2(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, glm::radians(30.0f)));
    std::unique_ptr<vke_common::GameObject> targetGameObj = std::make_unique<vke_common::GameObject>(targetParam);
    std::unique_ptr<vke_common::GameObject> targetGameObj2 = std::make_unique<vke_common::GameObject>(targetParam2);

    std::unique_ptr<vke_common::GameObject> cameraGameObj = std::make_unique<vke_common::GameObject>(camParam);
    camp = cameraGameObj.get();
    cameraGameObj->AddComponent(std::make_unique<vke_component::Camera>(105, WIDTH, HEIGHT, 0.01, 1000, camp));

    vke_render::RenderResourceManager *manager = vke_render::RenderResourceManager::GetInstance();

    {
        std::shared_ptr<vke_render::Material> material = manager->LoadMaterial("");
        std::shared_ptr<vke_render::Mesh> mesh = manager->LoadMesh("");

        targetGameObj->AddComponent(std::make_unique<vke_component::RenderableObject>(material, mesh, targetGameObj.get()));
        targetGameObj2->AddComponent(std::make_unique<vke_component::RenderableObject>(material, mesh, targetGameObj2.get()));
    }

    std::unique_ptr<vke_common::Scene> scene = std::make_unique<vke_common::Scene>();
    scene->AddObject(std::move(cameraGameObj));
    scene->AddObject(std::move(targetGameObj));
    scene->AddObject(std::move(targetGameObj2));

    vke_common::SceneManager::SetCurrentScene(std::move(scene));

    // glfwSetInputMode(engine->environment->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(engine->environment->window, mouse_callback);
    glfwSetFramebufferSizeCallback(engine->environment->window, vke_common::Engine::OnWindowResize);

    while (!glfwWindowShouldClose(engine->environment->window))
    {
        glfwPollEvents();

        float now = glfwGetTime();
        if (time_prev == 0)
            time_prev = glfwGetTime();
        time_delta = now - time_prev;
        time_prev = now;
        // std::cout << 1 / time_delta << std::endl;

        processInput(engine->environment->window, camp);

        engine->renderer->Update();
    }
    vkDeviceWaitIdle(engine->environment->logicalDevice);

    // engine->MainLoop();

    vke_common::Engine::Dispose();
    return 0;
}

#define CHECK_KEY(x) if (glfwGetKey(window, x) == GLFW_PRESS)

float moveSpeed = 0.5f;
float rotateSpeed = 1.0f;

void processInput(GLFWwindow *window, vke_common::GameObject *target)
{
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

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
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