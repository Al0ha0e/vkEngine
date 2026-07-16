#include <editor/editor.hpp>

GLFWwindow *initWindow(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(width, height, "Vulkan editor", nullptr, nullptr);
}

std::vector<vke_render::PassType> defaultPasses{
    vke_render::SHADOW_PASS,
    vke_render::GBUFFER_PASS,
    vke_render::SSAO_PASS,
    vke_render::DEFERRED_LIGHTING_PASS,
    vke_render::SKYBOX_RENDERER,
    vke_render::ATMOSPHERE_PASS,
    vke_render::TRANSPARENT_PASS,
    vke_render::WIREFRAME_COLLISION_PASS,
    vke_render::BLOOM_PASS,
    vke_render::TONE_MAPPING_PASS,
    vke_render::LAYERED_2D_RENDERER,
};

std::vector<std::unique_ptr<vke_render::RenderPassBase>> customPasses;

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        const std::string configPath(argv[1]);
        const nlohmann::json &configJSON = vke_common::LoadJSON(configPath);
        vke_editor::EditorConfig *editorConfig = vke_editor::EditorConfig::Init(configJSON);

        GLFWwindow *window = initWindow(editorConfig->windowWidth, editorConfig->windowHeight);
        vke_editor::Editor *editor = vke_editor::Editor::Init(
            window, *editorConfig,
            editorConfig->windowWidth, editorConfig->windowHeight,
            defaultPasses, customPasses);

        vke_common::AssetManager::BulkLoad(editorConfig->gameConfig->assetLUTPath);
        auto scene = vke_common::SceneManager::LoadScene(editorConfig->gameConfig->defaultScenePath);
        vke_common::SceneManager::SetCurrentScene(std::move(scene));

        vke_common::InputManager::SetCursorMode(GLFW_CURSOR_NORMAL);

        glfwSetFramebufferSizeCallback(window, vke_editor::Editor::OnWindowResize);
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            if (!editor->Update())
                glfwSetWindowShouldClose(vke_render::RenderEnvironment::GetInstance()->window, GLFW_TRUE);
        }
        vke_editor::Editor::WaitIdle();
        vke_editor::Editor::Dispose();
        vke_editor::EditorConfig::Dispose();
    }
    else
    {
        vke_editor::EditorConfig *editorConfig = vke_editor::EditorConfig::Init();

        GLFWwindow *window = initWindow(editorConfig->windowWidth, editorConfig->windowHeight);
        vke_editor::Editor *editor = vke_editor::Editor::PartialInit1(
            window, *editorConfig,
            editorConfig->windowWidth, editorConfig->windowHeight);

        editor->EnterProjectCreationMode();
        glfwSetFramebufferSizeCallback(window, vke_editor::Editor::OnWindowResize);

        bool projectCreated = false;
        std::filesystem::path projectPath;
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            if (!editor->PartialUpdate(projectCreated, projectPath))
                break;
        }

        if (!projectCreated)
        {
            glfwSetWindowShouldClose(vke_render::RenderEnvironment::GetInstance()->window, GLFW_TRUE);
            vke_editor::Editor::PartialDispose();
            return 0;
        }

        vke_editor::EditorConfig::Dispose();
        const nlohmann::json editorConfigJSON =
            vke_common::LoadJSON((projectPath / "editorconfig.json").string());
        editorConfig = vke_editor::EditorConfig::Init(editorConfigJSON);

        vke_editor::Editor::PartialInit2(*editorConfig, defaultPasses, customPasses);

        vke_common::AssetManager::BulkLoad(editorConfig->gameConfig->assetLUTPath);

        auto scene = vke_common::SceneManager::LoadScene(editorConfig->gameConfig->defaultScenePath);
        vke_common::SceneManager::SetCurrentScene(std::move(scene));

        glfwSetFramebufferSizeCallback(window, vke_editor::Editor::OnWindowResize);
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            if (!editor->Update())
                glfwSetWindowShouldClose(vke_render::RenderEnvironment::GetInstance()->window, GLFW_TRUE);
        }

        vke_editor::Editor::WaitIdle();
        vke_editor::Editor::Dispose();
        vke_editor::EditorConfig::Dispose();
    }

    return 0;
}
