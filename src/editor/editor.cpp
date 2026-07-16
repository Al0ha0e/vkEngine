#include <editor/editor.hpp>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <cstring>

namespace vke_editor
{
    EditorConfig *EditorConfig::instance = nullptr;
    EditorStateManager *EditorStateManager::instance;
    Editor *Editor::instance = nullptr;

    static inline glm::vec3 TransformForward(const vke_common::Transform &transform)
    {
        return transform.GetGlobalRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    static std::shared_ptr<vke_physics::PhyscisShape> CreateDefaultPhysicsBoxShape()
    {
        auto shape = std::make_shared<vke_physics::PhyscisShape>(vke_physics::PHYSICS_SHAPE_BOX);
        shape->shapeRef = new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
        return shape;
    }

    Editor *Editor::GetInstance()
    {
        VKE_FATAL_IF(instance == nullptr, "Editor not initialized!")
        return instance;
    }

    Editor *Editor::Init(GLFWwindow *window,
                         const EditorConfig &editorConfig,
                         uint32_t sceneViewportWidth,
                         uint32_t sceneViewportHeight,
                         std::vector<vke_render::PassType> &passes,
                         std::vector<std::unique_ptr<vke_render::RenderPassBase>> &customPasses)
    {
        PartialInit1(window, editorConfig, sceneViewportWidth, sceneViewportHeight);
        PartialInit2(editorConfig, passes, customPasses);
        return instance;
    }

    Editor *Editor::PartialInit1(GLFWwindow *window,
                                 const EditorConfig &editorConfig,
                                 uint32_t sceneViewportWidth,
                                 uint32_t sceneViewportHeight)
    {
        instance = new Editor();
        instance->selectedEntity = entt::null;
        instance->selectedAssetType = vke_common::ASSET_CNT_FLAG;
        instance->selectedAsset = 0;
        instance->assetBrowserMode = AssetBrowserMode::ByType;
        vke_common::EventSystem::Init();
        vke_common::TimeManager::Init();
        vke_common::InputManager::Init(window);
        vke_common::EngineStateManager::Init();
        vke_common::EngineStateManager::SetState(vke_common::EngineState::Paused);
        vke_editor::EditorStateManager::Init(vke_editor::EditorState::PartialInited);
        vke_render::RenderEnvironment::Init(window, editorConfig.gameConfig->enableVulkanValidationLayers);
        vke_common::AssetManager::Init(std::make_unique<vke_common::AssetDBJSON>("", vke_common::CUSTOM_ASSET_ID_ST), "");
        vke_common::AssetManager::BulkLoad(EditorAssetLUTPath, true);
        vke_render::DescriptorSetAllocator::Init();
        vke_render::RenderContext *ctx = &(vke_render::RenderEnvironment::GetInstance()->rootRenderContext);
        EditorRenderer::Init(window, ctx, sceneViewportWidth, sceneViewportHeight,
                             []()
                             { instance->DrawGUI(); });

        return instance;
    }

    void Editor::PartialInit2(const EditorConfig &editorConfig,
                              std::vector<vke_render::PassType> &passes,
                              std::vector<std::unique_ptr<vke_render::RenderPassBase>> &customPasses)
    {
        vke_physics::PhysicsManager::Init(editorConfig.gameConfig->physicsConfig);
        vke_common::Spatial2DLayerManager::Init();
        vke_render::Renderer::Init(EditorRenderer::GetInstance()->GetSceneRenderContext(),
                                   passes, customPasses, editorConfig.gameConfig->renderConfig);
        vke_common::ScriptManager::Init();
        vke_common::SceneManager::Init();
        vke_editor::EditorStateManager::SetState(vke_editor::EditorState::Edit);
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
        instance->disposeTexturePreviewDescriptorSets();
        vke_common::SceneManager::Dispose();
        vke_common::ScriptManager::Dispose();
        vke_render::Renderer::Dispose();
        vke_common::Spatial2DLayerManager::Dispose();
        vke_physics::PhysicsManager::Dispose();
        Editor::PartialDispose();
    }

    void Editor::PartialDispose()
    {
        EditorRenderer::Dispose();
        vke_render::DescriptorSetAllocator::Dispose();
        vke_common::AssetManager::Dispose();
        vke_render::RenderEnvironment::Dispose();
        vke_editor::EditorStateManager::Dispose();
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
        const vke_common::EngineState engineState = vke_common::EngineStateManager::GetState();
        if (engineState == vke_common::EngineState::Terminated)
        {
            Shutdown();
            return false;
        }

        vke_common::TimeManager::Update();

        if (vke_editor::EditorStateManager::GetState() == vke_editor::EditorState::Run)
        {
            vke_common::ScriptManager::GetInstance()->Update();
            fixedUpdateAccumulator += vke_common::TimeManager::GetDeltaTime();
            const float fixedStepTime = vke_physics::PhysicsManager::GetConfig().stepTime;
            while (fixedUpdateAccumulator >= fixedStepTime)
            {
                FixedUpdate();
                fixedUpdateAccumulator -= fixedStepTime;
            }
        }

        WireframeCollisionPass *wireframePass = vke_render::Renderer::GetWireframeCollisionPass();
        if (wireframePass)
            wireframePass->SetSelectedEntity(selectedEntity);

        vke_render::Renderer::GetInstance()->Update();
        EditorRenderer::GetInstance()->Update();
        vke_common::InputManager::EndFrame();
        return true;
    }

    bool Editor::PartialUpdate(bool &projectCreated, std::filesystem::path &projectPath)
    {
        if (projectCreationActive)
        {
            static uint32_t currentFrame = 1;
            currentFrame = (currentFrame + 1) % vke_render::MAX_FRAMES_IN_FLIGHT;
            if (projectCancelRequested)
            {
                projectCreated = false;
                return false;
            }

            if (projectCreationPending)
            {
                projectCreationActive = false;
                projectCreationPending = false;
                projectPath = finalizeProjectCreation();
                projectCreated = true;
                return false;
            }
            else
            {
                uint32_t imageIndex = EditorRenderer::AcquireSceneNextImage(currentFrame);
                EditorRenderer::PresentScene(currentFrame, imageIndex);
                EditorRenderer::GetInstance()->Update();
                vke_common::InputManager::EndFrame();
                return true;
            }
        }

        return false;
    }

    void Editor::FixedUpdate()
    {
        vke_common::ScriptManager::FixedUpdate();
        vke_physics::PhysicsManager::FixedUpdate();
    }

    void Editor::DrawGUI()
    {
        if (projectCreationActive)
        {
            showProjectCreationDialog();
            return;
        }

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

        const vke_editor::EditorState editorState = vke_editor::EditorStateManager::GetState();
        if (editorState == vke_editor::EditorState::Edit || editorState == vke_editor::EditorState::Run)
        {
            const bool currentIsEdit = editorState == vke_editor::EditorState::Edit;
            const char *buttonLabel = currentIsEdit ? "Start" : "Pause";
            const ImGuiStyle &style = ImGui::GetStyle();
            const float buttonWidth = ImGui::CalcTextSize(buttonLabel).x + style.FramePadding.x * 2.0f;
            const float centeredX = (ImGui::GetWindowWidth() - buttonWidth) * 0.5f;
            const float nextX = glm::max(ImGui::GetCursorPosX() + style.ItemSpacing.x, centeredX);

            ImGui::SameLine(nextX);
            if (ImGui::Button(buttonLabel, ImVec2(buttonWidth, 0.0f)))
            {
                if (currentIsEdit)
                {
                    vke_editor::EditorStateManager::SetState(vke_editor::EditorState::Run);
                    vke_common::EngineStateManager::SetState(vke_common::EngineState::Running);
                }
                else
                {
                    vke_editor::EditorStateManager::SetState(vke_editor::EditorState::Edit);
                    vke_common::EngineStateManager::SetState(vke_common::EngineState::Paused);
                }
                vke_common::InputManager::SetCursorMode(currentIsEdit ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                ImGuiIO &io = ImGui::GetIO();
                if (currentIsEdit)
                    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
                else
                    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }
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
        {
            clearSelectedAsset();
            selectedEntity = entt::null;
        }

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
        {
            clearSelectedAsset();
            selectedEntity = entity;
        }

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
        if (selectedAsset != 0)
        {
            if (selectedAssetType == vke_common::ASSET_TEXTURE)
                showSelectedTextureInspector();
            else if (selectedAssetType == vke_common::ASSET_MATERIAL)
                showSelectedMaterialInspector();
            else
                showSelectedAssetInspector();
            ImGui::End();
            return;
        }

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
            const bool updatePhysicsComponents =
                vke_editor::EditorStateManager::GetState() == vke_editor::EditorState::Edit;

            if (ImGui::InputFloat3("Position", glm::value_ptr(position)))
                scene->transformSystem.SetLocalPosition(selectedEntity, position, updatePhysicsComponents);
            if (ImGui::InputFloat3("Rotation", glm::value_ptr(rotation)))
                scene->transformSystem.SetLocalRotation(selectedEntity, glm::quat(glm::radians(rotation)), updatePhysicsComponents);
            if (ImGui::InputFloat3("Scale", glm::value_ptr(scale)))
                scene->transformSystem.SetLocalScale(selectedEntity, scale, updatePhysicsComponents);

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
        showAddComponentMenu();

        ImGui::End();
    }

    void Editor::showAddComponentMenu()
    {
        ImGui::Separator();

        struct ComponentMenuItem
        {
            const char *name;
            vke_common::ComponentType type;
        };

        static const ComponentMenuItem componentMenuItems[] = {
            {"Transform", vke_common::ComponentType::Transform},
            {"Camera", vke_common::ComponentType::Camera},
            {"RenderableObject", vke_common::ComponentType::RenderableObject},
            {"SkeletonAnimator", vke_common::ComponentType::SkeletonAnimator},
            {"RigidBody", vke_common::ComponentType::RigidBody},
            {"Sensor", vke_common::ComponentType::Sensor},
            {"CharacterController", vke_common::ComponentType::CharacterController},
            {"DirectionalLight", vke_common::ComponentType::DirectionalLight},
            {"PointLight", vke_common::ComponentType::PointLight},
            {"SpotLight", vke_common::ComponentType::SpotLight},
            {"Script", vke_common::ComponentType::Script},
            {"UIText", vke_common::ComponentType::UIText}};

        if (!ImGui::BeginCombo("Add Component", "Select component"))
            return;

        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        bool hasAvailableComponent = false;
        for (const ComponentMenuItem &item : componentMenuItems)
        {
            if (scene == nullptr || scene->HasComponent(selectedEntity, item.type))
                continue;

            hasAvailableComponent = true;
            if (ImGui::Selectable(item.name, false))
                addComponent(item.type);
        }

        if (!hasAvailableComponent)
            ImGui::TextDisabled("No components available");

        ImGui::EndCombo();
    }

    void Editor::addComponent(vke_common::ComponentType componentType)
    {
        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        if (scene == nullptr || selectedEntity == entt::null || !scene->registry.valid(selectedEntity) ||
            scene->HasComponent(selectedEntity, componentType) ||
            !scene->registry.all_of<vke_common::Transform>(selectedEntity))
            return;

        vke_render::LightManager *lightManager = vke_render::Renderer::GetInstance()->lightManager.get();
        const vke_common::Transform &transform = scene->registry.get<vke_common::Transform>(selectedEntity);
        const glm::vec3 position = transform.GetGlobalPosition();
        const glm::vec3 direction = glm::normalize(TransformForward(transform));
        constexpr float defaultRadius = 5.0f;
        const glm::vec4 defaultColor(1.0f, 1.0f, 1.0f, 1.0f);

        switch (componentType)
        {
        case vke_common::ComponentType::RenderableObject:
        {
            std::shared_ptr<vke_render::Material> material =
                vke_common::AssetManager::LoadMaterial(vke_common::BUILTIN_MATERIAL_DEFAULT_ID);
            std::shared_ptr<const vke_render::Mesh> mesh =
                vke_common::AssetManager::LoadMesh(vke_common::BUILTIN_MESH_SPHERE_ID);
            auto &renderable = scene->registry.emplace<vke_component::RenderableObject>(
                selectedEntity, transform, material, mesh);
            renderable.LoadToEngine();
            break;
        }
        case vke_common::ComponentType::RigidBody:
        {
            auto shape = CreateDefaultPhysicsBoxShape();
            auto &body = scene->registry.emplace<vke_component::RigidBody>(
                selectedEntity,
                transform,
                JPH::EMotionType::Dynamic,
                vke_physics::DefaultObjectLayers::MOVING,
                0.2f,
                0.0f,
                shape);
            if (scene->loadedToEngine)
                body.LoadToEngine(static_cast<uint32_t>(selectedEntity));
            break;
        }
        case vke_common::ComponentType::Sensor:
        {
            auto shape = CreateDefaultPhysicsBoxShape();
            auto &sensor = scene->registry.emplace<vke_component::Sensor>(
                selectedEntity,
                transform,
                true,
                vke_physics::DefaultObjectLayers::NON_MOVING,
                shape);
            if (scene->loadedToEngine)
                sensor.LoadToEngine(static_cast<uint32_t>(selectedEntity));
            break;
        }
        case vke_common::ComponentType::DirectionalLight:
            lightManager->AddLight<vke_render::DirectionalLight>(
                selectedEntity,
                glm::vec4(direction, 0.0f),
                defaultColor);
            break;
        case vke_common::ComponentType::PointLight:
            lightManager->AddLight<vke_render::PointLight>(
                selectedEntity,
                glm::vec4(position, defaultRadius),
                defaultColor);
            break;
        case vke_common::ComponentType::SpotLight:
            lightManager->AddLight<vke_render::SpotLight>(
                selectedEntity,
                glm::vec4(position, defaultRadius),
                glm::vec4(direction, 0.0f),
                defaultColor,
                glm::vec4(glm::cos(glm::radians(15.0f)), glm::cos(glm::radians(30.0f)), 0.0f, 0.0f));
            break;
        default:
            break;
        }
    }

    void Editor::createEmptyObject()
    {
        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        if (scene == nullptr)
            return;

        std::string name = "GameObject";
        clearSelectedAsset();
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
