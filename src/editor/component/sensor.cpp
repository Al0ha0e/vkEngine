#include <editor/editor.hpp>
#include <component/sensor.hpp>
#include <physics/physics.hpp>
#include <algorithm>

namespace vke_editor
{
    static void DrawReadOnlyUInt(const char *label, uint32_t value)
    {
        int displayValue = static_cast<int>(value);
        ImGui::BeginDisabled();
        ImGui::InputInt(label, &displayValue);
        ImGui::EndDisabled();
    }

    void Editor::drawSensorComponent(vke_common::Scene *scene)
    {
        if (scene == nullptr || selectedEntity == entt::null ||
            !scene->registry.all_of<vke_component::Sensor>(selectedEntity))
            return;

        if (!ImGui::TreeNodeEx("Sensor", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        vke_component::Sensor &sensor = scene->registry.get<vke_component::Sensor>(selectedEntity);
        JPH::BodyInterface &bodyInterface = vke_physics::PhysicsManager::GetBodyInterface();
        const bool loaded = scene->loadedToEngine;

        const uint32_t objectLayer = loaded
                                         ? bodyInterface.GetObjectLayer(sensor.bodyID)
                                         : static_cast<uint32_t>(sensor.settings.mObjectLayer);
        DrawReadOnlyUInt("Object Layer", objectLayer);

        const JPH::EMotionType currentMotionType = loaded
                                                       ? bodyInterface.GetMotionType(sensor.bodyID)
                                                       : sensor.settings.mMotionType;
        int motionType = currentMotionType == JPH::EMotionType::Kinematic ? 1 : 0;
        const char *motionTypeItems[] = {"Static", "Kinematic"};
        if (ImGui::Combo("Motion Type", &motionType, motionTypeItems, IM_ARRAYSIZE(motionTypeItems)))
        {
            motionType = std::clamp(motionType, 0, 1);
            sensor.settings.mMotionType = motionType == 0 ? JPH::EMotionType::Static : JPH::EMotionType::Kinematic;
            if (loaded)
                bodyInterface.SetMotionType(sensor.bodyID, sensor.settings.mMotionType, JPH::EActivation::Activate);
        }

        int motionQuality = static_cast<int>(loaded ? bodyInterface.GetMotionQuality(sensor.bodyID) : sensor.settings.mMotionQuality);
        const char *motionQualityItems[] = {"Discrete", "LinearCast"};
        if (ImGui::Combo("Motion Quality", &motionQuality, motionQualityItems, IM_ARRAYSIZE(motionQualityItems)))
        {
            motionQuality = std::clamp(motionQuality, 0, 1);
            sensor.settings.mMotionQuality = static_cast<JPH::EMotionQuality>(motionQuality);
            if (loaded)
                bodyInterface.SetMotionQuality(sensor.bodyID, sensor.settings.mMotionQuality);
        }

        ImGui::TreePop();
    }
}
