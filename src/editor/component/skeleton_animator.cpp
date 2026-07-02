#include <editor/editor.hpp>
#include <component/skeleton_animator.hpp>

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

    void Editor::drawSkeletonAnimatorComponent(vke_common::Scene *scene)
    {
        if (scene == nullptr || selectedEntity == entt::null ||
            !scene->registry.all_of<vke_component::SkeletonAnimator>(selectedEntity))
            return;

        if (!ImGui::TreeNodeEx("SkeletonAnimator", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const vke_component::SkeletonAnimator &animator =
            scene->registry.get<vke_component::SkeletonAnimator>(selectedEntity);
        const vke_common::AssetHandle materialHandle =
            animator.material == nullptr ? 0 : animator.material->handle;
        const vke_common::AssetHandle meshHandle =
            animator.renderUnit == nullptr || animator.renderUnit->mesh == nullptr
                ? 0
                : animator.renderUnit->mesh->handle;
        const vke_common::AssetHandle animationHandle =
            animator.animation == nullptr ? 0 : animator.animation->handle;
        const vke_common::AssetHandle skeletonHandle =
            animator.skeleton == nullptr ? 0 : animator.skeleton->handle;
        vke_common::AssetManager *assetManager = vke_common::AssetManager::GetInstance();

        DrawReadOnlyAsset(
            "Material",
            GetAssetDisplayName(materialHandle, assetManager->materialCache));
        DrawReadOnlyAsset(
            "Mesh",
            GetAssetDisplayName(meshHandle, assetManager->meshCache));
        DrawReadOnlyAsset(
            "Skeleton",
            GetAssetDisplayName(skeletonHandle, assetManager->skeletonCache));
        DrawReadOnlyAsset(
            "Animation",
            GetAssetDisplayName(animationHandle, assetManager->animationCache));

        ImGui::TreePop();
    }
}
