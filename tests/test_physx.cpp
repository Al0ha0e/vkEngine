#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <engine.hpp>
#include <gameobject.hpp>
#include <time.hpp>
#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <component/rigidbody.hpp>

const uint32_t WIDTH = 1024;
const uint32_t HEIGHT = 768;

vke_common::Engine *engine;
vke_common::TransformParameter camParam(glm::vec3(-5.0f, 4.0f, 10.0f), glm::vec3(1), glm::quat(1.0, 0.0, 0.0, 0.0));
vke_common::GameObject *camp = nullptr;
vke_common::GameObject *planep = nullptr, *spherep = nullptr;

void processInput(GLFWwindow *window, vke_common::GameObject *target);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

GLFWwindow *initWindow(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
}

int main()
{
    std::vector<vke_render::PassType> passes = {
        vke_render::BASE_RENDERER,
        vke_render::OPAQUE_RENDERER};
    std::vector<std::unique_ptr<vke_render::SubpassBase>> customPasses;
    std::vector<vke_render::RenderPassInfo> customPassInfo;
    GLFWwindow *window = initWindow(WIDTH, HEIGHT);
    vke_common::EventSystem::Init();
    vke_common::TimeManager::Init();
    vke_render::RenderEnvironment *environment = vke_render::RenderEnvironment::Init(window);
    engine = vke_common::Engine::Init(&(environment->rootRenderContext), passes, customPasses, customPassInfo);
    vke_common::AssetManager::LoadAssetLUT("./tests/scene/test_physx_desc.json");
    vke_common::SceneManager::LoadScene("./tests/scene/test_physx_scene.json");
    camp = vke_common::SceneManager::GetInstance()->currentScene->objects[1].get();
    planep = vke_common::SceneManager::GetInstance()->currentScene->objects[2].get();
    spherep = vke_common::SceneManager::GetInstance()->currentScene->objects[3].get();

    // {
    //     auto pmat = vke_common::AssetManager::LoadPhysicsMaterial("./tests/material/physx_mat1.json");
    //     auto staticRigid = std::make_unique<vke_physics::RigidBody>(std::make_unique<physx::PxPlaneGeometry>(), pmat, planep);
    //     planep->AddComponent(std::move(staticRigid));

    //     auto dynamicRigid = std::make_unique<vke_physics::RigidBody>(std::make_unique<physx::PxSphereGeometry>(1.0f), pmat, spherep);
    //     spherep->AddComponent(std::move(dynamicRigid));
    // }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, vke_common::Engine::OnWindowResize);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        vke_common::TimeManager::Update();

        processInput(window, camp);

        engine->Update();
    }
    vkDeviceWaitIdle(engine->environment->logicalDevice);

    // engine->MainLoop();

    vke_common::Engine::Dispose();
    vke_render::RenderEnvironment::Dispose();
    vke_common::EventSystem::Dispose();
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

    CHECK_KEY(GLFW_KEY_ENTER)
    {
        vke_common::SceneManager::SaveScene("./tests/scene/test_physx_scene.json");
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
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