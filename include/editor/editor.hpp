#ifndef EDITOR_H
#define EDITOR_H

#include <editor/render.hpp>
#include <game_config.hpp>
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
        entt::entity selectedEntity;

    public:
        static Editor *GetInstance();

        static Editor *Init(GLFWwindow *window,
                            const vke_common::GameConfig &gameConfig,
                            vke_render::RenderContext *ctx,
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
        void MainLoop();
        void DrawGUI();

    private:
        void showMainMenuBar();
        void showHierarchy();
        void drawHierarchyEntity(entt::entity entity, ImGuiTreeNodeFlags commonFlags);
        void showInspector();
        void drawCameraComponent(vke_common::Scene *scene);
        void drawLightComponents(vke_common::Scene *scene);
        void drawRenderableObjectComponent(vke_common::Scene *scene);
        void drawSkeletonAnimatorComponent(vke_common::Scene *scene);
        void drawRigidBodyComponent(vke_common::Scene *scene);
        void drawSensorComponent(vke_common::Scene *scene);
        void drawUITextComponent(vke_common::Scene *scene);
        void showAssets();
        void createEmptyObject();
        void ensureSelectedEntityValid();
    };
}

#endif
