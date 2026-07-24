#ifndef CHARACTER_CONTROLLER_H
#define CHARACTER_CONTROLLER_H

#include <physics/shape.hpp>
#include <component/transform.hpp>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <memory>

namespace vke_component
{
    struct CharacterControllerData
    {
        vke_physics::PhyscisShapeData shape;
        JPH::ObjectLayer layer;
        float mass = 70.0f;
        float maxStrength = 100.0f;
        float maxSlopeAngleRadians = glm::radians(50.0f);
        float predictiveContactDistance = 0.1f;
        float characterPadding = 0.02f;
        bool createInnerBody = true;

        CharacterControllerData() = default;
        CharacterControllerData(const nlohmann::json &json)
            : shape(json["shape"]),
              layer(json.value("layer", (int)vke_physics::DefaultObjectLayers::MOVING))
        {
            mass = json.value("mass", mass);
            maxStrength = json.value("maxStrength", maxStrength);
            maxSlopeAngleRadians =
                glm::radians(json.value("maxSlopeAngle",
                                        glm::degrees(maxSlopeAngleRadians)));
            predictiveContactDistance = json.value("predictiveContactDistance", predictiveContactDistance);
            characterPadding = json.value("characterPadding", characterPadding);
            createInnerBody = json.value("createInnerBody", true);
        }
        nlohmann::json ToJSON() const
        {
            return {{"type", "characterController"},
                    {"layer", (int)layer},
                    {"mass", mass},
                    {"maxStrength", maxStrength},
                    {"maxSlopeAngle", glm::degrees(maxSlopeAngleRadians)},
                    {"predictiveContactDistance", predictiveContactDistance},
                    {"characterPadding", characterPadding},
                    {"createInnerBody", createInnerBody},
                    {"shape", shape.ToJSON()}};
        }
    };

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
                            const CharacterControllerData &componentData)
            : shape(std::make_shared<vke_physics::PhyscisShape>(componentData.shape)),
              settings(new JPH::CharacterVirtualSettings()),
              layer(componentData.layer),
              desiredVelocity(JPH::Vec3::sZero()), verticalVelocity(0.0f)
        {
            settings->mMass = componentData.mass;
            settings->mMaxStrength = componentData.maxStrength;
            settings->mMaxSlopeAngle = componentData.maxSlopeAngleRadians;
            settings->mPredictiveContactDistance = componentData.predictiveContactDistance;
            settings->mCharacterPadding = componentData.characterPadding;
            if (componentData.createInnerBody)
                settings->mInnerBodyShape = shape->shapeRef;
            init(transform);
        }

        ~CharacterController() {}

        void FillData(CharacterControllerData &data) const
        {
            data.layer = layer;
            data.mass = settings->mMass;
            data.maxStrength = settings->mMaxStrength;
            data.maxSlopeAngleRadians = settings->mMaxSlopeAngle;
            data.predictiveContactDistance = settings->mPredictiveContactDistance;
            data.characterPadding = settings->mCharacterPadding;
            data.createInnerBody = settings->mInnerBodyShape != nullptr;
            shape->FillData(data.shape);
        }

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
            verticalVelocity += gravity.GetY() * deltaTime;
            if (character->IsSupported() && verticalVelocity < 0.0f)
                verticalVelocity = 0.0f;

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
