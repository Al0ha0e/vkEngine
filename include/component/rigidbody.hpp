#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <physics/shape.hpp>
#include <component/transform.hpp>

namespace vke_component
{
    struct RigidBodyData
    {
        vke_physics::PhyscisShapeData shape;
        JPH::EMotionType motionType;
        JPH::EMotionQuality motionQuality = JPH::EMotionQuality::Discrete;
        JPH::ObjectLayer layer;
        float friction;
        float restitution;
        float gravityFactor = 1.0f;
        bool hasMassOverride = false;
        float mass = 0.0f;

        RigidBodyData() = default;
        RigidBodyData(const nlohmann::json &json)
            : shape(json["shape"]), motionType(json["motionType"]),
              motionQuality(json.value("motionQuality", JPH::EMotionQuality::Discrete)),
              layer(json["layer"]), friction(json["friction"]), restitution(json["restitution"]),
              gravityFactor(json.value("gravityFactor", 1.0f)), hasMassOverride(json.contains("mass")),
              mass(hasMassOverride ? json["mass"].get<float>() : 0.0f) {}
        nlohmann::json ToJSON() const
        {
            nlohmann::json json = {{"type", "rigidbody"}, {"motionType", motionType},
                                   {"motionQuality", motionQuality}, {"layer", (int)layer},
                                   {"friction", friction}, {"restitution", restitution},
                                   {"gravityFactor", gravityFactor}, {"shape", shape.ToJSON()}};
            if (hasMassOverride)
                json["mass"] = mass;
            return json;
        }
    };

    class RigidBody
    {
    public:
        JPH::BodyID bodyID;
        std::shared_ptr<vke_physics::PhyscisShape> shape;
        JPH::BodyCreationSettings settings;
        float friction;
        float restitution;
        bool hasMassOverride;
        float mass;

        RigidBody(const vke_common::Transform &transform,
                  JPH::EMotionType motionType,
                  JPH::ObjectLayer layer,
                  float friction, float restitution,
                  std::shared_ptr<vke_physics::PhyscisShape> &shape)
            : shape(shape), friction(friction), restitution(restitution),
              hasMassOverride(false), mass(0.0f)
        {
            init(transform, motionType, layer);
        }

        RigidBody(const vke_common::Transform &transform,
                  const RigidBodyData &componentData)
            : shape(std::make_shared<vke_physics::PhyscisShape>(componentData.shape)),
              friction(componentData.friction), restitution(componentData.restitution),
              hasMassOverride(componentData.hasMassOverride), mass(componentData.mass)
        {
            init(transform, componentData.motionType, componentData.layer);
            settings.mMotionQuality = componentData.motionQuality;
            settings.mGravityFactor = componentData.gravityFactor;
        }

        ~RigidBody() {}

        void FillData(RigidBodyData &data) const
        {
            data.motionType = settings.mMotionType;
            data.motionQuality = settings.mMotionQuality;
            data.layer = settings.mObjectLayer;
            data.friction = friction;
            data.restitution = restitution;
            data.gravityFactor = settings.mGravityFactor;
            data.hasMassOverride = hasMassOverride;
            data.mass = mass;
            shape->FillData(data.shape);
        }

        void LoadToEngine(entt::entity entity)
        {
            settings.mUserData = static_cast<uint64_t>(entity);
            JPH::BodyInterface &interface = vke_physics::PhysicsManager::GetBodyInterface();
            bodyID = interface.CreateAndAddBody(settings, JPH::EActivation::Activate);
            interface.SetFriction(bodyID, friction);
            interface.SetRestitution(bodyID, restitution);
        }

        void UnloadFromEngine()
        {
            JPH::BodyInterface &interface = vke_physics::PhysicsManager::GetBodyInterface();
            interface.RemoveBody(bodyID);
            interface.DestroyBody(bodyID);
        }

        void OnTransformed(vke_common::Transform &transform)
        {
            const glm::vec3 position = transform.GetGlobalPosition();
            const glm::quat rotation = transform.GetGlobalRotation();
            settings.mPosition = JPH::RVec3(position.x, position.y, position.z);
            settings.mRotation = JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w);

            if (bodyID.IsInvalid() || vke_physics::PhysicsManager::GetInstance() == nullptr)
                return;

            JPH::BodyInterface &interface = vke_physics::PhysicsManager::GetBodyInterface();
            if (interface.IsAdded(bodyID))
                interface.SetPositionAndRotationWhenChanged(bodyID, settings.mPosition, settings.mRotation, JPH::EActivation::Activate);
        }

    private:
        void init(const vke_common::Transform &transform,
                  JPH::EMotionType motionType, JPH::ObjectLayer layer)
        {
            const glm::vec3 position = transform.GetGlobalPosition();
            const glm::quat rotation = transform.GetGlobalRotation();

            settings = JPH::BodyCreationSettings(shape->shapeRef,
                                                 JPH::RVec3(position.x, position.y, position.z),
                                                 JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                                 motionType, layer);
            settings.mMotionQuality = JPH::EMotionQuality::Discrete;
            settings.mGravityFactor = 1.0f;
            if (hasMassOverride)
            {
                settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                settings.mMassPropertiesOverride.mMass = mass;
            }
        }
    };
}

#endif
