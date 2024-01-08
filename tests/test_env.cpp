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
vke_common::GameObject cameraGameObj(camParam);

void processInput(GLFWwindow *window, vke_common::GameObject *target);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

int main()
{
    std::vector<vke_render::PassType> passes = {
        vke_render::BASE_RENDERER,
        vke_render::OPAQUE_RENDERER};
    std::vector<vke_render::SubpassBase *> customPasses;
    std::vector<vke_render::RenderPassInfo> customPassInfo;
    engine = vke_common::Engine::Init(WIDTH, HEIGHT, passes, customPasses, customPassInfo);

    vke_common::TransformParameter targetParam(glm::vec3(-0.5f, 0.5f, -1.0f), glm::vec3(0.0f, 0.0f, glm::radians(30.0f)));
    vke_common::TransformParameter targetParam2(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, glm::radians(30.0f)));
    vke_common::GameObject targetGameObj(targetParam);
    vke_common::GameObject targetGameObj2(targetParam2);

    vke_component::Camera camera(105, WIDTH, HEIGHT, 0.01, 1000, &cameraGameObj);
    cameraGameObj.components.push_back(&camera);

    vke_render::RenderResourceManager *manager = vke_render::RenderResourceManager::GetInstance();

    vke_render::Material *material = manager->LoadMaterial("");
    vke_render::Mesh *mesh = manager->LoadMesh("");

    vke_component::RenderableObject renderObj(material, mesh, &targetGameObj);
    vke_component::RenderableObject renderObj2(material, mesh, &targetGameObj2);

    targetGameObj.components.push_back(&renderObj);
    targetGameObj2.components.push_back(&renderObj2);

    glfwSetInputMode(engine->environment->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(engine->environment->window, mouse_callback);

    while (!glfwWindowShouldClose(engine->environment->window))
    {
        glfwPollEvents();

        float now = glfwGetTime();
        if (time_prev == 0)
            time_prev = glfwGetTime();
        time_delta = now - time_prev;
        time_prev = now;
        // std::cout << 1 / time_delta << std::endl;

        processInput(engine->environment->window, &cameraGameObj);

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
    cameraGameObj.RotateGlobal(-xoffset * rotateSpeed * time_delta, glm::vec3(0.0f, 1.0f, 0.0f));
    cameraGameObj.RotateLocal(yoffset * rotateSpeed * time_delta, glm::vec3(1.0f, 0.0f, 0.0f));
}