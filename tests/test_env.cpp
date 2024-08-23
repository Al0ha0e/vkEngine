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
vke_common::TransformParameter camParam(glm::vec3(-5.0f, 4.0f, 10.0f), glm::vec3(1), glm::quat(1.0, 0.0, 0.0, 0.0));
vke_common::GameObject *camp = nullptr;
vke_common::GameObject *objp = nullptr, *obj2p = nullptr;

void processInput(GLFWwindow *window, vke_common::GameObject *target, vke_common::GameObject *obj);
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
    engine = vke_common::Engine::Init(window, passes, customPasses, customPassInfo);

    // vke_common::TransformParameter targetParam(glm::vec3(-0.5f, 0.5f, -1.0f), glm::vec3(1), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    // vke_common::TransformParameter targetParam2(glm::vec3(-10.0f, 1.0f, 0.0f), glm::vec3(1), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    // std::unique_ptr<vke_common::GameObject> targetGameObj = std::make_unique<vke_common::GameObject>(targetParam);
    // std::unique_ptr<vke_common::GameObject> targetGameObj2 = std::make_unique<vke_common::GameObject>(targetParam2);
    // objp = targetGameObj.get();
    // obj2p = targetGameObj2.get();

    // std::unique_ptr<vke_common::GameObject> cameraGameObj = std::make_unique<vke_common::GameObject>(camParam);
    // camp = cameraGameObj.get();
    // cameraGameObj->AddComponent(std::make_unique<vke_component::Camera>(105, WIDTH, HEIGHT, 0.01, 1000, camp));

    // vke_common::ResourceManager *manager = vke_common::ResourceManager::GetInstance();

    // {
    //     std::shared_ptr<vke_render::Material> material = manager->LoadMaterial("./tests/material/mat1.json");
    //     // std::shared_ptr<const vke_render::Mesh> mesh = manager->LoadMesh("./tests/model/nanosuit.obj");
    //     std::shared_ptr<const vke_render::Mesh> mesh = manager->LoadMesh(vke_common::BuiltinMonkeyPath);

    //     targetGameObj->AddComponent(std::make_unique<vke_component::RenderableObject>(material, mesh, targetGameObj.get()));
    //     targetGameObj2->AddComponent(std::make_unique<vke_component::RenderableObject>(material, mesh, targetGameObj2.get()));
    // }

    // std::unique_ptr<vke_common::Scene> scene = std::make_unique<vke_common::Scene>();
    // scene->AddObject(std::move(cameraGameObj));
    // scene->AddObject(std::move(targetGameObj));
    // scene->AddObject(std::move(targetGameObj2));

    // vke_common::SceneManager::SetCurrentScene(std::move(scene));

    vke_common::SceneManager::LoadScene("./tests/scene/test_scene.json");
    camp = vke_common::SceneManager::GetInstance()->currentScene->objects[1].get();
    objp = vke_common::SceneManager::GetInstance()->currentScene->objects[2].get();
    obj2p = vke_common::SceneManager::GetInstance()->currentScene->objects[3].get();

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, vke_common::Engine::OnWindowResize);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float now = glfwGetTime();
        if (time_prev == 0)
            time_prev = glfwGetTime();
        time_delta = now - time_prev;
        time_prev = now;
        // std::cout << 1 / time_delta << std::endl;

        processInput(window, camp, objp);

        engine->renderer->Update();
    }
    vkDeviceWaitIdle(engine->environment->logicalDevice);

    // engine->MainLoop();

    vke_common::Engine::Dispose();
    return 0;
}

#define CHECK_KEY(x) if (glfwGetKey(window, x) == GLFW_PRESS)

float moveSpeed = 2.5f;
float rotateSpeed = 1.0f;

void processInput(GLFWwindow *window, vke_common::GameObject *target, vke_common::GameObject *obj)
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
    CHECK_KEY(GLFW_KEY_0)
    {
        vke_common::SceneManager::SaveScene("./tests/scene/test_scene.json");
    }
    CHECK_KEY(GLFW_KEY_1)
    {
        obj->RotateLocal(rotateSpeed * time_delta, glm::vec3(1, 0, 0));
    }
    CHECK_KEY(GLFW_KEY_2)
    {
        obj->RotateLocal(rotateSpeed * time_delta, glm::vec3(0, 1, 0));
    }
    CHECK_KEY(GLFW_KEY_3)
    {
        obj->RotateLocal(rotateSpeed * time_delta, glm::vec3(0, 0, 1));
    }
    CHECK_KEY(GLFW_KEY_4)
    {
        obj->RotateGlobal(rotateSpeed * time_delta, glm::vec3(1, 0, 0));
    }
    CHECK_KEY(GLFW_KEY_5)
    {
        obj->RotateGlobal(rotateSpeed * time_delta, glm::vec3(0, 1, 0));
    }
    CHECK_KEY(GLFW_KEY_6)
    {
        obj->RotateGlobal(rotateSpeed * time_delta, glm::vec3(0, 0, 1));
    }
    CHECK_KEY(GLFW_KEY_8)
    {
        obj->Scale(glm::vec3(0.2 * time_delta, 0, 0));
    }
    CHECK_KEY(GLFW_KEY_9)
    {
        obj->Scale(glm::vec3(-0.2 * time_delta, 0, 0));
    }

    CHECK_KEY(GLFW_KEY_T)
    {
        obj2p->TranslateGlobal(glm::vec3(moveSpeed * time_delta, 0, 0));
        // obj2p->RotateLocal(rotateSpeed * time_delta, glm::vec3(1, 0, 0));
    }
    CHECK_KEY(GLFW_KEY_Y)
    {
        obj2p->TranslateGlobal(glm::vec3(0, moveSpeed * time_delta, 0));
        // obj2p->RotateLocal(rotateSpeed * time_delta, glm::vec3(0, 1, 0));
    }
    CHECK_KEY(GLFW_KEY_U)
    {
        obj2p->TranslateGlobal(glm::vec3(0, 0, moveSpeed * time_delta));
        // obj2p->RotateLocal(rotateSpeed * time_delta, glm::vec3(0, 0, 1));
    }
    CHECK_KEY(GLFW_KEY_G)
    {
        obj2p->RotateGlobal(rotateSpeed * time_delta, glm::vec3(1, 0, 0));
    }
    CHECK_KEY(GLFW_KEY_H)
    {
        obj2p->RotateGlobal(rotateSpeed * time_delta, glm::vec3(0, 1, 0));
    }
    CHECK_KEY(GLFW_KEY_J)
    {
        obj2p->RotateGlobal(rotateSpeed * time_delta, glm::vec3(0, 0, 1));
    }
    static bool pressed = false;
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS && !pressed)
    {
        pressed = true;
        obj2p->SetParent(obj2p->parent ? nullptr : objp);
    }
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_RELEASE)
    {
        pressed = false;
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