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

int main()
{
    engine = vke_common::Engine::Init(WIDTH, HEIGHT);

    vke_common::TransformParameter camParam(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, glm::radians(0.0f), 0.0f));
    vke_common::TransformParameter targetParam(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3());
    vke_common::GameObject cameraGameObj(camParam);
    vke_common::GameObject targetGameObj(targetParam);

    vke_component::Camera camera(90, WIDTH, HEIGHT, 0.01, 1000, &cameraGameObj);

    vke_render::RenderResourceManager *manager = vke_render::RenderResourceManager::GetInstance();

    vke_render::Material *material = manager->LoadMaterial("");
    vke_render::Mesh *mesh = manager->LoadMesh("");

    vke_component::RenderableObject renderObj(material, mesh, &targetGameObj);

    engine->MainLoop();

    vke_common::Engine::Dispose();
    return 0;
}
