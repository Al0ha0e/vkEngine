#include <editor/editor.hpp>
#include <component/skeleton_animator.hpp>

namespace vke_editor
{
    static void DrawReadOnlyHandle(const char *label, vke_common::AssetHandle handle)
    {
        long long value = static_cast<long long>(handle);
        ImGui::BeginDisabled();
        ImGui::InputScalar(label, ImGuiDataType_S64, &value);
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

        DrawReadOnlyHandle("Material Handle", materialHandle);
        DrawReadOnlyHandle("Mesh Handle", meshHandle);
        DrawReadOnlyHandle("Animation Handle", animationHandle);
        DrawReadOnlyHandle("Skeleton Handle", skeletonHandle);

        ImGui::TreePop();
    }
}
