#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <physics/physics.hpp>
#include <physics/shape.hpp>
#include <asset.hpp>
#include <gameobject.hpp>

namespace vke_physics
{
    class RigidBody : public vke_common::Component
    {
    public:
        JPH::BodyID bodyID;
        std::shared_ptr<PhyscisShape> shape;

        RigidBody(JPH::EMotionType motionType,
                  JPH::ObjectLayer layer,
                  float friction, float restitution,
                  std::shared_ptr<PhyscisShape> &shape,
                  vke_common::GameObject *obj)
            : shape(shape), Component(obj)
        {
            init(motionType, layer, friction, restitution);
        }

        RigidBody(vke_common::GameObject *obj, nlohmann::json &json)
            : Component(obj), shape(new PhyscisShape(json["shape"]))
        {
            init(json["motionType"], json["layer"], json["friction"], json["restitution"]);
        }

        ~RigidBody()
        {
            PhysicsManager::RemoveUpdateListener(physicsUpdateListenerID);
            JPH::BodyInterface &interface = PhysicsManager::GetBodyInterface();
            interface.RemoveBody(bodyID);
            interface.DestroyBody(bodyID);
        }

        std::string ToJSON() override
        {
            JPH::BodyInterface &interface = PhysicsManager::GetBodyInterface();
            std::string ret = "{\n\"type\":\"rigidbody\",\n";
            ret += "\"motionType\": " + std::to_string((int)interface.GetMotionType(bodyID)) + ",\n";
            ret += "\"layer\": " + std::to_string((int)interface.GetObjectLayer(bodyID)) + ",\n";
            ret += "\"friction\": " + std::to_string(interface.GetFriction(bodyID)) + ",\n";
            ret += "\"restitution\": " + std::to_string(interface.GetRestitution(bodyID)) + ",\n";
            ret += "\"shape\": " + shape->ToJSON();
            ret += "\n}";
            return ret;
        }

        void OnTransformed(vke_common::TransformParameter &param) override
        {
            // TODO
        }

        static void update(void *self, void *info)
        {
            RigidBody *rigidbody = (RigidBody *)self;

            JPH::BodyInterface &interface = PhysicsManager::GetBodyInterface();
            JPH::RVec3 position;
            JPH::Quat rotation;
            interface.GetPositionAndRotation(rigidbody->bodyID, position, rotation);
            JPH::RVec3 cpos;
            cpos = interface.GetCenterOfMassPosition(rigidbody->bodyID);

            rigidbody->gameObject->SetLocalPosition(glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            rigidbody->gameObject->SetLocalRotation(glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
            // TODO Set Global
        }

    private:
        vke_ds::id32_t physicsUpdateListenerID;

        void init(JPH::EMotionType motionType, JPH::ObjectLayer layer, float friction, float restitution)
        {
            vke_common::TransformParameter &param = gameObject->transform;
            JPH::BodyInterface &interface = PhysicsManager::GetBodyInterface();
            JPH::BodyCreationSettings settings(shape->shapeRef,
                                               JPH::RVec3(param.position.x, param.position.y, param.position.z),
                                               JPH::Quat(param.rotation.x, param.rotation.y, param.rotation.z, param.rotation.w),
                                               motionType, layer);
            bodyID = interface.CreateAndAddBody(settings, JPH::EActivation::Activate);
            interface.SetFriction(bodyID, friction);
            interface.SetRestitution(bodyID, restitution);
            physicsUpdateListenerID = PhysicsManager::RegisterUpdateListener(this, std::function<void(void *, void *)>(update));
        }
    };
}

#endif