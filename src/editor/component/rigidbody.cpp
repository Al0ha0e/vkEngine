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
            body.mass = std::max(body.mass, 0.001f);
        const auto settings = body.GetSettings();
        if (settings.mMotionType != JPH::EMotionType::Static)
            ApplyEditorMassProperties(settings, body.bodyID);
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
        auto settings = body.GetSettings();
        vke_physics::PhyscisShape shape(settings.GetShape());
        const JPH::EMotionType currentMotionType = settings.mMotionType;

        const uint32_t objectLayer = settings.mObjectLayer;
        DrawReadOnlyUInt("Object Layer", objectLayer);

        DrawPhysicsShapeEditor("RigidBodyShape", shape, settings, bodyInterface, body.bodyID, true, currentMotionType == JPH::EMotionType::Static);

        float restitution = settings.mRestitution;
        if (ImGui::InputFloat("Restitution", &restitution, 0.05f, 0.25f, "%.3f"))
        {
            bodyInterface.SetRestitution(body.bodyID, std::max(restitution, 0.0f));
        }

        float friction = settings.mFriction;
        if (ImGui::InputFloat("Friction", &friction, 0.05f, 0.25f, "%.3f"))
        {
            bodyInterface.SetFriction(body.bodyID, std::max(friction, 0.0f));
        }

        float gravityFactor = settings.mGravityFactor;
        if (ImGui::InputFloat("Gravity Factor", &gravityFactor, 0.05f, 0.25f, "%.3f"))
        {
            body.SetGravityFactor(gravityFactor);
        }

        bool hasMassOverride = body.hasMassOverride;
        if (ImGui::Checkbox("Override Mass", &hasMassOverride))
        {
            body.hasMassOverride = hasMassOverride;
            if (body.hasMassOverride && body.mass <= 0.0f)
                body.mass = std::max(shape.shapeRef->MustBeStatic() ? 1.0f : settings.GetMassProperties().mMass, 0.001f);
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
            const auto newMotionType = static_cast<JPH::EMotionType>(motionType);
            if (newMotionType != JPH::EMotionType::Static)
                EnsureDynamicBodyShape(shape, settings, bodyInterface, body.bodyID);
            bodyInterface.SetMotionType(body.bodyID, newMotionType, JPH::EActivation::Activate);
            ApplyRigidBodyMassSettings(body);
        }

        int motionQuality = static_cast<int>(settings.mMotionQuality);
        const char *motionQualityItems[] = {"Discrete", "LinearCast"};
        if (ImGui::Combo("Motion Quality", &motionQuality, motionQualityItems, IM_ARRAYSIZE(motionQualityItems)))
        {
            motionQuality = std::clamp(motionQuality, 0, 1);
            body.SetMotionQuality(static_cast<JPH::EMotionQuality>(motionQuality));
        }

        ImGui::TreePop();
    }
}
