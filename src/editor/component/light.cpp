#include <editor/editor.hpp>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

namespace vke_editor
{
    static bool DrawColorIntensity(glm::vec4 &colorWithIntensity)
    {
        bool changed = false;
        glm::vec3 color(colorWithIntensity);
        float intensity = colorWithIntensity.w;

        changed |= ImGui::ColorEdit3("Color", glm::value_ptr(color));
        changed |= ImGui::InputFloat("Intensity", &intensity, 0.1f, 1.0f);

        if (changed)
            colorWithIntensity = glm::vec4(color, glm::max(intensity, 0.0f));
        return changed;
    }

    template <vke_render::AllowedLightType T>
    static bool DrawCommonLightProperties(T &light)
    {
        return DrawColorIntensity(light.colorWithIntensity);
    }

    static bool DrawRadius(float &radius)
    {
        float value = radius;
        if (!ImGui::InputFloat("Radius", &value, 0.25f, 1.0f))
            return false;

        radius = glm::max(value, 0.0f);
        return true;
    }

    static bool DrawSpotCone(vke_render::SpotLight &light)
    {
        bool changed = false;
        float innerCone = glm::degrees(glm::acos(glm::clamp(light.cone.x, -1.0f, 1.0f)));
        float outerCone = glm::degrees(glm::acos(glm::clamp(light.cone.y, -1.0f, 1.0f)));

        changed |= ImGui::SliderFloat("Inner Cone", &innerCone, 0.0f, 89.0f, "%.1f deg");
        changed |= ImGui::SliderFloat("Outer Cone", &outerCone, 0.0f, 89.0f, "%.1f deg");

        if (!changed)
            return false;

        innerCone = glm::clamp(innerCone, 0.0f, 89.0f);
        outerCone = glm::clamp(outerCone, innerCone, 89.0f);
        light.cone.x = glm::cos(glm::radians(innerCone));
        light.cone.y = glm::cos(glm::radians(outerCone));
        return true;
    }

    void Editor::drawLightComponents(vke_common::Scene *scene)
    {
        if (scene == nullptr || selectedEntity == entt::null)
            return;

        auto *lightManager = vke_render::Renderer::GetInstance()->lightManager.get();
        if (lightManager == nullptr)
            return;

        if (lightManager->HasLight<vke_render::DirectionalLight>(selectedEntity) &&
            ImGui::TreeNodeEx("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &light = lightManager->GetLightWithoutCheckByEntity<vke_render::DirectionalLight>(selectedEntity);
            if (DrawCommonLightProperties(light))
                lightManager->MarkDirty<vke_render::DirectionalLight>();
            ImGui::TreePop();
        }

        if (lightManager->HasLight<vke_render::PointLight>(selectedEntity) &&
            ImGui::TreeNodeEx("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &light = lightManager->GetLightWithoutCheckByEntity<vke_render::PointLight>(selectedEntity);
            bool changed = DrawCommonLightProperties(light);
            changed |= DrawRadius(light.positionWithRadius.w);
            if (changed)
                lightManager->MarkDirty<vke_render::PointLight>();
            ImGui::TreePop();
        }

        if (lightManager->HasLight<vke_render::SpotLight>(selectedEntity) &&
            ImGui::TreeNodeEx("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &light = lightManager->GetLightWithoutCheckByEntity<vke_render::SpotLight>(selectedEntity);
            bool changed = DrawCommonLightProperties(light);
            changed |= DrawRadius(light.positionWithRadius.w);
            changed |= DrawSpotCone(light);

            bool castShadow = light.CastShadow();
            if (ImGui::Checkbox("Cast Shadow", &castShadow))
            {
                if (castShadow)
                    lightManager->ActivateSpotShadow(selectedEntity);
                else
                    lightManager->DeactivateSpotShadow(selectedEntity);
                changed = true;
            }

            if (changed)
            {
                lightManager->MarkDirty<vke_render::SpotLight>();
                if (light.CastShadow())
                    lightManager->UpdateSpotShadow(selectedEntity);
            }
            ImGui::TreePop();
        }
    }
}
