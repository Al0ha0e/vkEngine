#ifndef CHARACTER_CONTROLLER_H
#define CHARACTER_CONTROLLER_H

#include <physics/shape.hpp>
#include <component/transform.hpp>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <memory>

namespace vke_component
{
    class CharacterController
    {
    public:
        std::shared_ptr<vke_physics::PhyscisShape> shape;
        JPH::Ref<JPH::CharacterVirtualSettings> settings;
        JPH::Ref<JPH::CharacterVirtual> character;
        JPH::ObjectLayer layer;
        JPH::Vec3 desiredVelocity;
        float verticalVelocity;

        CharacterController(const vke_common::Transform &transform,
                            JPH::ObjectLayer layer,
                            std::shared_ptr<vke_physics::PhyscisShape> &shape)
            : shape(shape), settings(new JPH::CharacterVirtualSettings()), layer(layer),
              desiredVelocity(JPH::Vec3::sZero()), verticalVelocity(0.0f)
        {
            init(transform);
        }

        CharacterController(const vke_common::Transform &transform,
                            const nlohmann::json &json)
            : shape(new vke_physics::PhyscisShape(json["shape"])),
              settings(new JPH::CharacterVirtualSettings()),
              layer(json.value("layer", (int)vke_physics::Layers::MOVING)),
              desiredVelocity(JPH::Vec3::sZero()), verticalVelocity(0.0f)
        {
            settings->mMass = json.value("mass", settings->mMass);
            settings->mMaxStrength = json.value("maxStrength", settings->mMaxStrength);
            settings->mMaxSlopeAngle = glm::radians(json.value("maxSlopeAngle", glm::degrees(settings->mMaxSlopeAngle)));
            settings->mPredictiveContactDistance = json.value("predictiveContactDistance", settings->mPredictiveContactDistance);
            settings->mCharacterPadding = json.value("characterPadding", settings->mCharacterPadding);
            if (json.value("createInnerBody", true))
                settings->mInnerBodyShape = shape->shapeRef;
            init(transform);
        }

        ~CharacterController() {}

        void LoadToEngine()
        {
            if (character != nullptr)
                return;

            const glm::vec3 position = initialTransform.GetGlobalPosition();
            const glm::quat rotation = initialTransform.GetGlobalRotation();
            character = new JPH::CharacterVirtual(settings,
                                                  JPH::RVec3(position.x, position.y, position.z),
                                                  JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                                  &vke_physics::PhysicsManager::GetPhysicsSystem());
        }

        void UnloadFromEngine()
        {
            character = nullptr;
        }

        void SetDesiredVelocity(const glm::vec3 &velocity)
        {
            desiredVelocity = JPH::Vec3(velocity.x, velocity.y, velocity.z);
            if (velocity.y > 0.0f)
                verticalVelocity = velocity.y;
        }

        glm::vec3 GetLinearVelocity() const
        {
            if (character == nullptr)
                return glm::vec3(0.0f);

            const JPH::Vec3 velocity = character->GetLinearVelocity();
            return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
        }

        bool IsGrounded() const
        {
            return character != nullptr && character->IsSupported();
        }

        void Update(float deltaTime)
        {
            if (character == nullptr)
                return;

            const JPH::Vec3 gravity = vke_physics::PhysicsManager::GetPhysicsSystem().GetGravity();
            if (character->IsSupported() && verticalVelocity < 0.0f)
                verticalVelocity = 0.0f;
            verticalVelocity += gravity.GetY() * deltaTime;

            JPH::Vec3 velocity(desiredVelocity.GetX(),
                               desiredVelocity.GetY() + verticalVelocity,
                               desiredVelocity.GetZ());
            character->SetLinearVelocity(velocity);

            JPH::PhysicsSystem &physicsSystem = vke_physics::PhysicsManager::GetPhysicsSystem();
            JPH::DefaultBroadPhaseLayerFilter broadPhaseFilter = physicsSystem.GetDefaultBroadPhaseLayerFilter(layer);
            JPH::DefaultObjectLayerFilter objectLayerFilter = physicsSystem.GetDefaultLayerFilter(layer);
            JPH::BodyFilter bodyFilter;
            JPH::ShapeFilter shapeFilter;
            character->Update(deltaTime, gravity, broadPhaseFilter, objectLayerFilter, bodyFilter, shapeFilter,
                              vke_physics::PhysicsManager::GetTempAllocator());
        }

        nlohmann::json ToJSON()
        {
            nlohmann::json ret = {
                {"type", "characterController"},
                {"layer", (int)layer},
                {"mass", settings->mMass},
                {"maxStrength", settings->mMaxStrength},
                {"maxSlopeAngle", glm::degrees(settings->mMaxSlopeAngle)},
                {"predictiveContactDistance", settings->mPredictiveContactDistance},
                {"characterPadding", settings->mCharacterPadding},
                {"createInnerBody", settings->mInnerBodyShape != nullptr},
                {"shape", shape->ToJSON()}};
            return ret;
        }

        void OnTransformed(vke_common::Transform &transform)
        {
            if (character == nullptr)
                return;

            const glm::vec3 position = transform.GetGlobalPosition();
            const glm::quat rotation = transform.GetGlobalRotation();
            character->SetPosition(JPH::RVec3(position.x, position.y, position.z));
            character->SetRotation(JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w));
        }

    private:
        vke_common::Transform initialTransform;

        void init(const vke_common::Transform &transform)
        {
            initialTransform = transform;
            settings->mShape = shape->shapeRef;
            settings->mInnerBodyLayer = layer;
        }
    };
}

#endif
