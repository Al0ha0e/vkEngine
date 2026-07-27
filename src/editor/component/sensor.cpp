#include <editor/editor.hpp>
#include <editor/physics_shape_editor.hpp>
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

    void Editor::drawSensorComponent()
    {
        if (selectedEntity == entt::null ||
            !sceneManager->registry.all_of<vke_component::Sensor>(selectedEntity))
            return;

        if (!ImGui::TreeNodeEx("Sensor", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        vke_component::Sensor &sensor = sceneManager->registry.get<vke_component::Sensor>(selectedEntity);
        JPH::BodyInterface &bodyInterface = vke_physics::PhysicsManager::GetBodyInterface();

        const uint32_t objectLayer = bodyInterface.GetObjectLayer(sensor.bodyID);
        DrawReadOnlyUInt("Object Layer", objectLayer);

        DrawPhysicsShapeEditor("SensorShape", sensor.shape, sensor.settings, bodyInterface, sensor.bodyID, false);

        const JPH::EMotionType currentMotionType = bodyInterface.GetMotionType(sensor.bodyID);
        int motionType = currentMotionType == JPH::EMotionType::Kinematic ? 1 : 0;
        const char *motionTypeItems[] = {"Static", "Kinematic"};
        if (ImGui::Combo("Motion Type", &motionType, motionTypeItems, IM_ARRAYSIZE(motionTypeItems)))
        {
            motionType = std::clamp(motionType, 0, 1);
            sensor.settings.mMotionType = motionType == 0 ? JPH::EMotionType::Static : JPH::EMotionType::Kinematic;
            bodyInterface.SetMotionType(sensor.bodyID, sensor.settings.mMotionType, JPH::EActivation::Activate);
        }

        int motionQuality = static_cast<int>(bodyInterface.GetMotionQuality(sensor.bodyID));
        const char *motionQualityItems[] = {"Discrete", "LinearCast"};
        if (ImGui::Combo("Motion Quality", &motionQuality, motionQualityItems, IM_ARRAYSIZE(motionQualityItems)))
        {
            motionQuality = std::clamp(motionQuality, 0, 1);
            sensor.settings.mMotionQuality = static_cast<JPH::EMotionQuality>(motionQuality);
            bodyInterface.SetMotionQuality(sensor.bodyID, sensor.settings.mMotionQuality);
        }

        ImGui::TreePop();
    }
}
