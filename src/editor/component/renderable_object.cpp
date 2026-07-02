#include <editor/editor.hpp>
#include <component/renderable_object.hpp>

namespace vke_editor
{
    template <typename AssetMap>
    static std::string GetAssetDisplayName(vke_common::AssetHandle handle, const AssetMap &assets)
    {
        if (handle == 0)
            return "0  <none>";

        auto it = assets.find(handle);
        const std::string name = it == assets.end() ? "<missing>" : it->second.name;
        return std::to_string(handle) + "  " + name;
    }

    static void DrawReadOnlyAsset(const char *label, const std::string &displayName)
    {
        std::vector<char> value(displayName.begin(), displayName.end());
        value.push_back('\0');

        ImGui::BeginDisabled();
        ImGui::InputText(label, value.data(), value.size(), ImGuiInputTextFlags_ReadOnly);
        ImGui::EndDisabled();
    }

    void Editor::drawRenderableObjectComponent(vke_common::Scene *scene)
    {
        if (scene == nullptr || selectedEntity == entt::null ||
            !scene->registry.all_of<vke_component::RenderableObject>(selectedEntity))
            return;

        if (!ImGui::TreeNodeEx("RenderableObject", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        vke_component::RenderableObject &renderable =
            scene->registry.get<vke_component::RenderableObject>(selectedEntity);
        const vke_common::AssetHandle materialHandle =
            renderable.material == nullptr ? 0 : renderable.material->handle;
        const vke_common::AssetHandle meshHandle =
            renderable.renderUnit == nullptr || renderable.renderUnit->mesh == nullptr
                ? 0
                : renderable.renderUnit->mesh->handle;
        vke_common::AssetManager *assetManager = vke_common::AssetManager::GetInstance();

        DrawReadOnlyAsset(
            "Material",
            GetAssetDisplayName(materialHandle, assetManager->materialCache));

        const std::string selectedMesh = GetAssetDisplayName(meshHandle, assetManager->meshCache);
        if (ImGui::BeginCombo("Mesh", selectedMesh.c_str()))
        {
            for (const auto &[assetHandle, asset] : assetManager->meshCache)
            {
                const bool selected = assetHandle == meshHandle;
                const std::string meshLabel = std::to_string(assetHandle) + "  " + asset.name;
                if (ImGui::Selectable(meshLabel.c_str(), selected))
                {
                    std::shared_ptr<const vke_render::Mesh> mesh =
                        vke_common::AssetManager::LoadMesh(assetHandle);
                    renderable.SetMesh(mesh);
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }
}
