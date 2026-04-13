#include <engine.hpp>

GLFWwindow *initWindow(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
}

int main(int argc, char **argv)
{
    VKE_FATAL_IF(argc != 2, "arg count mismatch")
    const std::string configPath(argv[1]);
    const nlohmann::json &configJSON = vke_common::AssetManager::LoadJSON(configPath);
    vke_common::GameConfig::Init(configJSON);
    const vke_common::GameConfig *gameConfig = vke_common::GameConfig::GetInstance();

    std::vector<vke_render::PassType> passes = {
        vke_render::GBUFFER_PASS,
        vke_render::DEFERRED_LIGHTING_PASS,
        vke_render::SKYBOX_RENDERER,
        vke_render::UI_RENDERER};
    std::vector<std::unique_ptr<vke_render::RenderPassBase>> customPasses;

    GLFWwindow *window = initWindow(gameConfig->windowWidth, gameConfig->windowHeight);
    vke_common::Engine *engine = vke_common::Engine::Init(window, nullptr, passes, customPasses);

    vke_common::AssetManager::LoadAssetLUT(gameConfig->assetLUTPath);
    auto scene = vke_common::SceneManager::LoadScene(gameConfig->defaultScenePath);
    vke_common::SceneManager::SetCurrentScene(std::move(scene));

    glfwSetFramebufferSizeCallback(window, vke_common::Engine::OnWindowResize);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (!engine->Update())
            glfwSetWindowShouldClose(vke_render::RenderEnvironment::GetInstance()->window, GLFW_TRUE);
    }
    vke_common::Engine::WaitIdle();
    vke_common::Engine::Dispose();
    vke_common::GameConfig::Dispose();

    return 0;
}
