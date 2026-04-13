#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <physics/shape.hpp>
#include <asset.hpp>
#include <component/transform.hpp>

namespace vke_component
{
    class RigidBody
    {
    public:
        JPH::BodyID bodyID;
        std::shared_ptr<vke_physics::PhyscisShape> shape;
        JPH::BodyCreationSettings settings;
        float friction;
        float restitution;

        RigidBody(const vke_common::Transform &transform,
                  JPH::EMotionType motionType,
                  JPH::ObjectLayer layer,
                  float friction, float restitution,
                  std::shared_ptr<vke_physics::PhyscisShape> &shape)
            : friction(friction), restitution(restitution), shape(shape)
        {
            init(transform, motionType, layer);
        }

        RigidBody(const vke_common::Transform &transform,
                  const nlohmann::json &json)
            : friction(json["friction"]), restitution(json["restitution"]), shape(new vke_physics::PhyscisShape(json["shape"]))
        {
            init(transform, json["motionType"], json["layer"]);
        }

        ~RigidBody() {}

        void LoadToEngine()
        {
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

        nlohmann::json ToJSON()
        {
            JPH::BodyInterface &interface = vke_physics::PhysicsManager::GetBodyInterface();

            nlohmann::json ret = {
                {"type", "rigidbody"},
                {"motionType", settings.mMotionType},
                {"layer", (int)settings.mObjectLayer},
                {"friction", friction},
                {"restitution", restitution},
                {"shape", shape->ToJSON()}};
            return ret;
        }

        void OnTransformed(vke_common::Transform &param) // TODO: syncing Transform back to physics here needs a guard to avoid physics <-> scene feedback loops.
        {
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
        }
    };
}

#endif