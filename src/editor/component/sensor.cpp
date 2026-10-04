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

        auto settings = sensor.GetSettings();
        vke_physics::PhyscisShape shape(settings.GetShape());
        const uint32_t objectLayer = settings.mObjectLayer;
        DrawReadOnlyUInt("Object Layer", objectLayer);

        DrawPhysicsShapeEditor("SensorShape", shape, settings, bodyInterface, sensor.bodyID, false);

        const JPH::EMotionType currentMotionType = settings.mMotionType;
        int motionType = currentMotionType == JPH::EMotionType::Kinematic ? 1 : 0;
        const char *motionTypeItems[] = {"Static", "Kinematic"};
        if (ImGui::Combo("Motion Type", &motionType, motionTypeItems, IM_ARRAYSIZE(motionTypeItems)))
        {
            motionType = std::clamp(motionType, 0, 1);
            const auto newMotionType = motionType == 0 ? JPH::EMotionType::Static : JPH::EMotionType::Kinematic;
            bodyInterface.SetMotionType(sensor.bodyID, newMotionType, JPH::EActivation::Activate);
        }

        int motionQuality = static_cast<int>(settings.mMotionQuality);
        const char *motionQualityItems[] = {"Discrete", "LinearCast"};
        if (ImGui::Combo("Motion Quality", &motionQuality, motionQualityItems, IM_ARRAYSIZE(motionQualityItems)))
        {
            motionQuality = std::clamp(motionQuality, 0, 1);
            bodyInterface.SetMotionQuality(sensor.bodyID, static_cast<JPH::EMotionQuality>(motionQuality));
        }

        ImGui::TreePop();
    }
}
