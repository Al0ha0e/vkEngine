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

        const std::string selectedMaterial = GetAssetDisplayName(materialHandle, assetManager->materialCache);
        if (ImGui::BeginCombo("Material", selectedMaterial.c_str()))
        {
            for (const auto &[assetHandle, asset] : assetManager->materialCache)
            {
                const bool selected = assetHandle == materialHandle;
                const std::string materialLabel = std::to_string(assetHandle) + "  " + asset.name;
                if (ImGui::Selectable(materialLabel.c_str(), selected))
                {
                    std::shared_ptr<vke_render::Material> material =
                        vke_common::AssetManager::LoadMaterial(assetHandle);
                    renderable.SetMaterial(material);
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        bool castsShadow = renderable.castsShadow;
        if (ImGui::Checkbox("Cast Shadow", &castsShadow))
            renderable.SetCastShadow(castsShadow);

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
