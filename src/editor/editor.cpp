#include <editor/editor.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>

namespace vke_editor
{
    Editor *Editor::instance = nullptr;

    Editor *Editor::GetInstance()
    {
        VKE_FATAL_IF(instance == nullptr, "Editor not initialized!")
        return instance;
    }

    Editor *Editor::Init(GLFWwindow *window,
                         const vke_common::GameConfig &gameConfig,
                         vke_render::RenderContext *ctx,
                         uint32_t sceneViewportWidth,
                         uint32_t sceneViewportHeight,
                         std::vector<vke_render::PassType> &passes,
                         std::vector<std::unique_ptr<vke_render::RenderPassBase>> &customPasses)
    {
        instance = new Editor();
        instance->selectedEntity = entt::null;
        vke_common::EventSystem::Init();
        vke_common::TimeManager::Init();
        vke_common::InputManager::Init(window);
        vke_common::EngineStateManager::Init();
        vke_render::RenderEnvironment::Init(window, gameConfig.enableVulkanValidationLayers);
        vke_common::AssetManager::Init();
        vke_physics::PhysicsManager::Init(gameConfig.physicsConfig);
        vke_render::DescriptorSetAllocator::Init();
        vke_common::Spatial2DLayerManager::Init();
        if (ctx == nullptr)
            ctx = &(vke_render::RenderEnvironment::GetInstance()->rootRenderContext);
        EditorRenderer::Init(window, ctx, sceneViewportWidth, sceneViewportHeight,
                             []()
                             { instance->DrawGUI(); });
        vke_render::Renderer::Init(EditorRenderer::GetInstance()->GetSceneRenderContext(),
                                   passes, customPasses, gameConfig.renderConfig);
        vke_common::ScriptManager::Init();
        vke_common::SceneManager::Init();
        return instance;
    }

    void Editor::Shutdown()
    {
        vke_render::Renderer::Shutdown();
    }

    void Editor::WaitIdle()
    {
        vke_render::Renderer::WaitIdle();
    }

    void Editor::Dispose()
    {
        vke_common::SceneManager::Dispose();
        vke_common::ScriptManager::Dispose();
        vke_render::Renderer::Dispose();
        EditorRenderer::Dispose();
        vke_common::Spatial2DLayerManager::Dispose();
        vke_render::DescriptorSetAllocator::Dispose();
        vke_physics::PhysicsManager::Dispose();
        vke_common::AssetManager::Dispose();
        vke_render::RenderEnvironment::Dispose();
        vke_common::EngineStateManager::Dispose();
        vke_common::InputManager::Dispose();
        vke_common::TimeManager::Dispose();
        vke_common::EventSystem::Dispose();
        delete instance;
    }

    void Editor::OnWindowResize(GLFWwindow *window, int width, int height)
    {
        vke_common::EventSystem::DispatchEvent(vke_common::EVENT_WINDOW_RESIZE, nullptr);
    }

    bool Editor::Update()
    {
        const vke_common::EngineState state = vke_common::EngineStateManager::GetState();
        if (state == vke_common::EngineState::Terminated)
        {
            Shutdown();
            return false;
        }

        vke_common::TimeManager::Update();

        // if (state != vke_common::EngineState::Paused)
        // {
        //     vke_common::ScriptManager::GetInstance()->Update();
        //     fixedUpdateAccumulator += vke_common::TimeManager::GetDeltaTime();
        //     const float fixedStepTime = vke_physics::PhysicsManager::GetConfig().stepTime;
        //     while (fixedUpdateAccumulator >= fixedStepTime)
        //     {
        //         FixedUpdate();
        //         fixedUpdateAccumulator -= fixedStepTime;
        //     }
        // }

        vke_render::Renderer::GetInstance()->Update();
        EditorRenderer::GetInstance()->Update();
        vke_common::InputManager::EndFrame();
        return true;
    }

    void Editor::FixedUpdate()
    {
        vke_common::ScriptManager::FixedUpdate();
        vke_physics::PhysicsManager::FixedUpdate();
    }

    void Editor::MainLoop()
    {
        while (!glfwWindowShouldClose(vke_render::RenderEnvironment::GetInstance()->window))
        {
            glfwPollEvents();
            Update();
        }
        vkDeviceWaitIdle(vke_render::globalLogicalDevice);
    }

    void Editor::DrawGUI()
    {
        ensureSelectedEntityValid();
        showMainMenuBar();
        showHierarchy();
        showInspector();
        showAssets();
    }

    void Editor::showMainMenuBar()
    {
        if (!ImGui::BeginMainMenuBar())
            return;

        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem("Save Scene"))
            {
                vke_common::SceneManager *sceneManager = vke_common::SceneManager::GetInstance();
                if (sceneManager->currentScene != nullptr && !sceneManager->currentScene->path.empty())
                    vke_common::SceneManager::SaveScene(sceneManager->currentScene->path);
            }

            if (ImGui::BeginMenu("New Object"))
            {
                if (ImGui::MenuItem("Empty"))
                    createEmptyObject();
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void Editor::showHierarchy()
    {
        ImGui::Begin("Hierarchy");

        vke_common::SceneManager *sceneManager = vke_common::SceneManager::GetInstance();
        vke_common::Scene *scene = sceneManager->currentScene.get();
        if (scene == nullptr)
        {
            ImGui::TextUnformatted("No scene loaded");
            ImGui::End();
            return;
        }

        ImGuiTreeNodeFlags commonFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                         ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                         ImGuiTreeNodeFlags_SpanAvailWidth;

        for (auto &[id, entity] : scene->idToEntity)
        {
            if (!scene->registry.valid(entity) || !scene->registry.all_of<vke_common::GameObject, vke_common::Transform>(entity))
                continue;

            const vke_common::Transform &transform = scene->registry.get<vke_common::Transform>(entity);
            if (transform.parent == entt::null)
                drawHierarchyEntity(entity, commonFlags);
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
            selectedEntity = entt::null;

        ImGui::End();
    }

    void Editor::drawHierarchyEntity(entt::entity entity, ImGuiTreeNodeFlags commonFlags)
    {
        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        if (scene == nullptr || !scene->registry.valid(entity))
            return;

        auto [object, transform] = scene->registry.get<vke_common::GameObject, vke_common::Transform>(entity);
        ImGuiTreeNodeFlags flags = commonFlags;
        if (entity == selectedEntity)
            flags |= ImGuiTreeNodeFlags_Selected;
        if (transform.children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        ImGui::PushID(static_cast<int>(object.id));
        const bool opened = ImGui::TreeNodeEx(object.name.c_str(), flags);
        if (ImGui::IsItemClicked())
            selectedEntity = entity;

        if (opened && !transform.children.empty())
        {
            for (entt::entity child : transform.children)
                drawHierarchyEntity(child, commonFlags);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    void Editor::showInspector()
    {
        ImGui::Begin("Inspector");

        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        if (scene == nullptr || selectedEntity == entt::null || !scene->registry.valid(selectedEntity) ||
            !scene->registry.all_of<vke_common::GameObject, vke_common::Transform>(selectedEntity))
        {
            ImGui::TextUnformatted("No object selected");
            ImGui::End();
            return;
        }

        vke_common::GameObject &object = scene->registry.get<vke_common::GameObject>(selectedEntity);
        vke_common::Transform &transform = scene->registry.get<vke_common::Transform>(selectedEntity);

        char nameBuffer[128]{};
        std::strncpy(nameBuffer, object.name.c_str(), sizeof(nameBuffer) - 1);
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
            object.name = nameBuffer;

        ImGui::Checkbox("Static", &object.isStatic);
        if (!scene->layers.empty())
        {
            std::vector<const char *> layers;
            layers.reserve(scene->layers.size());
            for (const std::string &layer : scene->layers)
                layers.push_back(layer.c_str());

            int layerIndex = object.layer;
            if (layerIndex < 0 || layerIndex >= static_cast<int>(layers.size()))
                layerIndex = 0;
            if (ImGui::Combo("Layer", &layerIndex, layers.data(), static_cast<int>(layers.size())))
                object.layer = layerIndex;
        }

        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            glm::vec3 position = transform.localPosition;
            glm::vec3 rotation = glm::degrees(glm::eulerAngles(transform.localRotation));
            glm::vec3 scale = transform.localScale;

            if (ImGui::InputFloat3("Position", glm::value_ptr(position)))
                scene->transformSystem.SetLocalPosition(selectedEntity, position);
            if (ImGui::InputFloat3("Rotation", glm::value_ptr(rotation)))
                scene->transformSystem.SetLocalRotation(selectedEntity, glm::quat(glm::radians(rotation)));
            if (ImGui::InputFloat3("Scale", glm::value_ptr(scale)))
                scene->transformSystem.SetLocalScale(selectedEntity, scale);

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Components", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (scene->registry.all_of<vke_component::CharacterController>(selectedEntity))
                ImGui::BulletText("CharacterController");
            auto scriptIt = scene->csharpScriptStates.find(selectedEntity);
            if (scriptIt != scene->csharpScriptStates.end())
            {
                for (const auto &[className, state] : scriptIt->second)
                    ImGui::BulletText("Script: %s", className.c_str());
            }
            ImGui::TreePop();
        }

        drawCameraComponent(scene);
        drawLightComponents(scene);
        drawRenderableObjectComponent(scene);
        drawSkeletonAnimatorComponent(scene);
        drawRigidBodyComponent(scene);
        drawSensorComponent(scene);
        drawUITextComponent(scene);

        ImGui::End();
    }

    template <typename AssetMap>
    static void ShowAssetGroup(const char *label, const AssetMap &assets)
    {
        if (!ImGui::TreeNode(label))
            return;

        for (const auto &[id, asset] : assets)
            ImGui::Text("%llu  %s", static_cast<unsigned long long>(id), asset.name.c_str());

        ImGui::TreePop();
    }

    void Editor::showAssets()
    {
        ImGui::Begin("Assets");

        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        if (scene != nullptr)
            ImGui::Text("Scene: %s", scene->path.empty() ? "<unsaved>" : scene->path.c_str());

        vke_common::AssetManager *assetManager = vke_common::AssetManager::GetInstance();
        ShowAssetGroup("Textures", assetManager->textureCache);
        ShowAssetGroup("Meshes", assetManager->meshCache);
        ShowAssetGroup("Materials", assetManager->materialCache);
        ShowAssetGroup("Scenes", assetManager->sceneCache);
        ShowAssetGroup("Fonts", assetManager->fontCache);
        ShowAssetGroup("Animations", assetManager->animationCache);

        ImGui::End();
    }

    void Editor::createEmptyObject()
    {
        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        if (scene == nullptr)
            return;

        std::string name = "GameObject";
        selectedEntity = scene->AddObject(name, glm::vec3(0.0f), glm::vec3(1.0f), glm::quat(glm::vec3(0.0f)), 0, false);
    }

    void Editor::ensureSelectedEntityValid()
    {
        vke_common::SceneManager *sceneManager = vke_common::SceneManager::GetInstance();
        vke_common::Scene *scene = sceneManager->currentScene.get();
        if (scene == nullptr || selectedEntity == entt::null || !scene->registry.valid(selectedEntity))
            selectedEntity = entt::null;
    }
}
