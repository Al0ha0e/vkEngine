#include <editor/editor.hpp>
#include <editor/physics_shape_editor.hpp>
#include <component/rigidbody.hpp>
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

    static void ApplyRigidBodyMassSettings(vke_component::RigidBody &body)
    {
        if (body.hasMassOverride)
        {
            body.mass = std::max(body.mass, 0.001f);
            body.settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
            body.settings.mMassPropertiesOverride.mMass = body.mass;
        }
        else
        {
            body.settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
        }

        if (body.settings.mMotionType != JPH::EMotionType::Static)
            ApplyEditorMassProperties(body.settings, body.bodyID);
    }

    void Editor::drawRigidBodyComponent()
    {
        if (selectedEntity == entt::null ||
            !sceneManager->registry.all_of<vke_component::RigidBody>(selectedEntity))
            return;

        if (!ImGui::TreeNodeEx("RigidBody", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        vke_component::RigidBody &body = sceneManager->registry.get<vke_component::RigidBody>(selectedEntity);
        JPH::BodyInterface &bodyInterface = vke_physics::PhysicsManager::GetBodyInterface();
        const JPH::EMotionType currentMotionType = bodyInterface.GetMotionType(body.bodyID);

        const uint32_t objectLayer = bodyInterface.GetObjectLayer(body.bodyID);
        DrawReadOnlyUInt("Object Layer", objectLayer);

        DrawPhysicsShapeEditor("RigidBodyShape", body.shape, body.settings, bodyInterface, body.bodyID, true, currentMotionType == JPH::EMotionType::Static);

        float restitution = bodyInterface.GetRestitution(body.bodyID);
        if (ImGui::InputFloat("Restitution", &restitution, 0.05f, 0.25f, "%.3f"))
        {
            body.restitution = std::max(restitution, 0.0f);
            bodyInterface.SetRestitution(body.bodyID, body.restitution);
        }

        float friction = bodyInterface.GetFriction(body.bodyID);
        if (ImGui::InputFloat("Friction", &friction, 0.05f, 0.25f, "%.3f"))
        {
            body.friction = std::max(friction, 0.0f);
            bodyInterface.SetFriction(body.bodyID, body.friction);
        }

        float gravityFactor = bodyInterface.GetGravityFactor(body.bodyID);
        if (ImGui::InputFloat("Gravity Factor", &gravityFactor, 0.05f, 0.25f, "%.3f"))
        {
            body.settings.mGravityFactor = gravityFactor;
            bodyInterface.SetGravityFactor(body.bodyID, gravityFactor);
        }

        bool hasMassOverride = body.hasMassOverride;
        if (ImGui::Checkbox("Override Mass", &hasMassOverride))
        {
            body.hasMassOverride = hasMassOverride;
            if (body.hasMassOverride && body.mass <= 0.0f)
                body.mass = std::max(body.settings.GetMassProperties().mMass, 0.001f);
            ApplyRigidBodyMassSettings(body);
        }

        ImGui::BeginDisabled(!body.hasMassOverride);
        float mass = body.mass;
        if (ImGui::InputFloat("Mass", &mass, 0.1f, 1.0f, "%.3f"))
        {
            body.mass = std::max(mass, 0.001f);
            ApplyRigidBodyMassSettings(body);
        }
        ImGui::EndDisabled();

        int motionType = static_cast<int>(currentMotionType);
        const char *motionTypeItems[] = {"Static", "Kinematic", "Dynamic"};
        if (ImGui::Combo("Motion Type", &motionType, motionTypeItems, IM_ARRAYSIZE(motionTypeItems)))
        {
            motionType = std::clamp(motionType, 0, 2);
            body.settings.mMotionType = static_cast<JPH::EMotionType>(motionType);
            if (body.settings.mMotionType != JPH::EMotionType::Static)
                EnsureDynamicBodyShape(body.shape, body.settings, bodyInterface, body.bodyID);
            bodyInterface.SetMotionType(body.bodyID, body.settings.mMotionType, JPH::EActivation::Activate);
            ApplyRigidBodyMassSettings(body);
        }

        int motionQuality = static_cast<int>(bodyInterface.GetMotionQuality(body.bodyID));
        const char *motionQualityItems[] = {"Discrete", "LinearCast"};
        if (ImGui::Combo("Motion Quality", &motionQuality, motionQualityItems, IM_ARRAYSIZE(motionQualityItems)))
        {
            motionQuality = std::clamp(motionQuality, 0, 1);
            body.settings.mMotionQuality = static_cast<JPH::EMotionQuality>(motionQuality);
            bodyInterface.SetMotionQuality(body.bodyID, body.settings.mMotionQuality);
        }

        ImGui::TreePop();
    }
}
