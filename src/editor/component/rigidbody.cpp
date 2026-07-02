#include <editor/editor.hpp>
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

    void Editor::drawRigidBodyComponent(vke_common::Scene *scene)
    {
        if (scene == nullptr || selectedEntity == entt::null ||
            !scene->registry.all_of<vke_component::RigidBody>(selectedEntity))
            return;

        if (!ImGui::TreeNodeEx("RigidBody", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        vke_component::RigidBody &body = scene->registry.get<vke_component::RigidBody>(selectedEntity);
        JPH::BodyInterface &bodyInterface = vke_physics::PhysicsManager::GetBodyInterface();
        const bool loaded = scene->loadedToEngine;

        const uint32_t objectLayer = loaded
                                         ? bodyInterface.GetObjectLayer(body.bodyID)
                                         : static_cast<uint32_t>(body.settings.mObjectLayer);
        DrawReadOnlyUInt("Object Layer", objectLayer);

        float restitution = loaded ? bodyInterface.GetRestitution(body.bodyID) : body.restitution;
        if (ImGui::InputFloat("Restitution", &restitution, 0.05f, 0.25f, "%.3f"))
        {
            body.restitution = std::max(restitution, 0.0f);
            if (loaded)
                bodyInterface.SetRestitution(body.bodyID, body.restitution);
        }

        float friction = loaded ? bodyInterface.GetFriction(body.bodyID) : body.friction;
        if (ImGui::InputFloat("Friction", &friction, 0.05f, 0.25f, "%.3f"))
        {
            body.friction = std::max(friction, 0.0f);
            if (loaded)
                bodyInterface.SetFriction(body.bodyID, body.friction);
        }

        float gravityFactor = loaded ? bodyInterface.GetGravityFactor(body.bodyID) : body.settings.mGravityFactor;
        if (ImGui::InputFloat("Gravity Factor", &gravityFactor, 0.05f, 0.25f, "%.3f"))
        {
            body.settings.mGravityFactor = gravityFactor;
            if (loaded)
                bodyInterface.SetGravityFactor(body.bodyID, gravityFactor);
        }

        int motionType = static_cast<int>(loaded ? bodyInterface.GetMotionType(body.bodyID) : body.settings.mMotionType);
        const char *motionTypeItems[] = {"Static", "Kinematic", "Dynamic"};
        if (ImGui::Combo("Motion Type", &motionType, motionTypeItems, IM_ARRAYSIZE(motionTypeItems)))
        {
            motionType = std::clamp(motionType, 0, 2);
            body.settings.mMotionType = static_cast<JPH::EMotionType>(motionType);
            if (loaded)
                bodyInterface.SetMotionType(body.bodyID, body.settings.mMotionType, JPH::EActivation::Activate);
        }

        int motionQuality = static_cast<int>(loaded ? bodyInterface.GetMotionQuality(body.bodyID) : body.settings.mMotionQuality);
        const char *motionQualityItems[] = {"Discrete", "LinearCast"};
        if (ImGui::Combo("Motion Quality", &motionQuality, motionQualityItems, IM_ARRAYSIZE(motionQualityItems)))
        {
            motionQuality = std::clamp(motionQuality, 0, 1);
            body.settings.mMotionQuality = static_cast<JPH::EMotionQuality>(motionQuality);
            if (loaded)
                bodyInterface.SetMotionQuality(body.bodyID, body.settings.mMotionQuality);
        }

        ImGui::TreePop();
    }
}
