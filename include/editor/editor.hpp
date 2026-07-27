#ifndef EDITOR_H
#define EDITOR_H

#include <editor/assets.hpp>
#include <editor/render.hpp>
#include <editor/editor_config.hpp>
#include <editor/editor_state.hpp>
#include <render/render.hpp>
#include <physics/physics.hpp>
#include <scene.hpp>
#include <event.hpp>
#include <input.hpp>
#include <engine_state.hpp>
#include <time.hpp>
#include <script.hpp>
#include <spatial_2d.hpp>
#include <entt/entity/entity.hpp>
#include <imgui.h>
#include <map>
#include <vector>

namespace vke_editor
{
    class Editor
    {
    private:
        static Editor *instance;
        Editor() : fixedUpdateAccumulator(0.0f) {}
        ~Editor() {}
        Editor(const Editor &);
        Editor &operator=(const Editor &);

        float fixedUpdateAccumulator;
        vke_common::SceneManager *sceneManager;
        entt::entity selectedEntity;
        vke_common::AssetType selectedAssetType;
        vke_common::AssetHandle selectedAsset;
        std::map<vke_common::AssetHandle, VkDescriptorSet> texturePreviewDescriptorSets;
        AssetBrowserMode assetBrowserMode;
        AssetTreeNode assetDirectoryTree;

    public:
        static Editor *GetInstance();

        static entt::entity GetSelectedEntity() { return GetInstance()->selectedEntity; }

        static Editor *Init(GLFWwindow *window,
                            const EditorConfig &editorConfig,
                            uint32_t sceneViewportWidth,
                            uint32_t sceneViewportHeight,
                            std::vector<vke_render::PassType> &passes,
                            std::vector<std::unique_ptr<vke_render::RenderPassBase>> &customPasses);

        static void Shutdown();
        static void WaitIdle();
        static void Dispose();
        static void OnWindowResize(GLFWwindow *window, int width, int height);

        bool Update();
        void FixedUpdate();
        void DrawGUI();

    private:
        void showMainMenuBar();
        void showHierarchy();
        void drawHierarchyEntity(entt::entity entity, ImGuiTreeNodeFlags commonFlags);
        void showInspector();
        void drawCameraComponent();
        void drawLightComponents();
        void drawRenderableObjectComponent();
        void drawSkeletonAnimatorComponent();
        void drawRigidBodyComponent();
        void drawSensorComponent();
        void drawUITextComponent();
        void drawAudioSourceComponent();
        void drawAudioListenerComponent();
        void showAddComponentMenu();
        void addComponent(vke_common::ComponentType componentType);
        void showAssets();
        void showLog();
        void showAssetsByType(vke_common::AssetManager *assetManager);
        void showAssetsByDirectory(vke_common::AssetManager *assetManager);
        void rebuildAssetDirectoryTree(vke_common::AssetManager *assetManager);
        void drawAssetDirectoryNode(const AssetTreeNode &node);
        void drawAssetEntry(const AssetTreeEntry &entry);
        void selectAsset(vke_common::AssetType assetType, vke_common::AssetHandle asset);
        void clearSelectedAsset();
        void disposeTexturePreviewDescriptorSets();
        void showSelectedTextureInspector();
        VkDescriptorSet getTexturePreviewDescriptorSet(vke_common::AssetHandle textureAsset);
        void drawTexturePreview(vke_common::AssetHandle textureAsset, float maxSize);
        void showSelectedMaterialInspector();
        void showSelectedAssetInspector();
        void createEmptyObject();
        void ensureSelectedEntityValid();
        void showAssetImportDialog();
        void openAssetImport(vke_common::AssetType type);
    };
}

#endif
