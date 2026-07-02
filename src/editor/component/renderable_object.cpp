#include <editor/editor.hpp>
#include <component/renderable_object.hpp>

namespace vke_editor
{
    static void DrawReadOnlyHandle(const char *label, vke_common::AssetHandle handle)
    {
        long long value = static_cast<long long>(handle);
        ImGui::BeginDisabled();
        ImGui::InputScalar(label, ImGuiDataType_S64, &value);
        ImGui::EndDisabled();
    }

    void Editor::drawRenderableObjectComponent(vke_common::Scene *scene)
    {
        if (scene == nullptr || selectedEntity == entt::null ||
            !scene->registry.all_of<vke_component::RenderableObject>(selectedEntity))
            return;

        if (!ImGui::TreeNodeEx("RenderableObject", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const vke_component::RenderableObject &renderable =
            scene->registry.get<vke_component::RenderableObject>(selectedEntity);
        const vke_common::AssetHandle materialHandle =
            renderable.material == nullptr ? 0 : renderable.material->handle;
        const vke_common::AssetHandle meshHandle =
            renderable.renderUnit == nullptr || renderable.renderUnit->mesh == nullptr
                ? 0
                : renderable.renderUnit->mesh->handle;

        DrawReadOnlyHandle("Material Handle", materialHandle);
        DrawReadOnlyHandle("Mesh Handle", meshHandle);

        ImGui::TreePop();
    }
}
