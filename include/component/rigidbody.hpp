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

        RigidBody(const vke_common::Transform &transform,
                  JPH::EMotionType motionType,
                  JPH::ObjectLayer layer,
                  float friction, float restitution,
                  std::shared_ptr<vke_physics::PhyscisShape> &shape)
            : shape(shape)
        {
            init(transform, motionType, layer, friction, restitution);
        }

        RigidBody(const vke_common::Transform &transform,
                  const nlohmann::json &json)
            : shape(new vke_physics::PhyscisShape(json["shape"]))
        {
            init(transform, json["motionType"], json["layer"], json["friction"], json["restitution"]);
        }

        ~RigidBody()
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
                {"motionType", (int)interface.GetMotionType(bodyID)},
                {"layer", (int)interface.GetObjectLayer(bodyID)},
                {"friction", interface.GetFriction(bodyID)},
                {"restitution", interface.GetRestitution(bodyID)},
                {"shape", shape->ToJSON()}};
            return ret;
        }

        void OnTransformed(vke_common::Transform &param) {} // TODO

    private:
        void init(const vke_common::Transform &transform,
                  JPH::EMotionType motionType, JPH::ObjectLayer layer,
                  float friction, float restitution)
        {
            const glm::vec3 position = transform.GetGlobalPosition();
            const glm::quat rotation = transform.GetGlobalRotation();

            JPH::BodyInterface &interface = vke_physics::PhysicsManager::GetBodyInterface();
            JPH::BodyCreationSettings settings(shape->shapeRef,
                                               JPH::RVec3(position.x, position.y, position.z),
                                               JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                               motionType, layer);
            bodyID = interface.CreateAndAddBody(settings, JPH::EActivation::Activate);
            interface.SetFriction(bodyID, friction);
            interface.SetRestitution(bodyID, restitution);
        }
    };
}

#endif