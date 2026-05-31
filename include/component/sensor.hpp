#ifndef SENSOR_H
#define SENSOR_H

#include <physics/shape.hpp>
#include <asset.hpp>
#include <component/transform.hpp>

namespace vke_component
{
    class Sensor
    {
    public:
        JPH::BodyID bodyID;
        std::shared_ptr<vke_physics::PhyscisShape> shape;
        JPH::BodyCreationSettings settings;

        Sensor(const vke_common::Transform &transform,
               bool isStatic,
               JPH::ObjectLayer layer,
               std::shared_ptr<vke_physics::PhyscisShape> &shape)
            : shape(shape)
        {
            init(transform, isStatic, layer);
        }

        Sensor(const vke_common::Transform &transform,
               const nlohmann::json &json)
            : shape(new vke_physics::PhyscisShape(json["shape"]))
        {
            init(transform, json.value("isStatic", true), json["layer"]);
        }

        ~Sensor() {}

        void LoadToEngine(uint32_t entity = 0)
        {
            settings.mUserData = static_cast<uint64_t>(entity);
            JPH::BodyInterface &interface = vke_physics::PhysicsManager::GetBodyInterface();
            bodyID = interface.CreateAndAddBody(settings, JPH::EActivation::Activate);
        }

        void UnloadFromEngine()
        {
            JPH::BodyInterface &interface = vke_physics::PhysicsManager::GetBodyInterface();
            interface.RemoveBody(bodyID);
            interface.DestroyBody(bodyID);
        }

        nlohmann::json ToJSON()
        {
            return {
                {"type", "sensor"},
                {"isStatic", settings.mMotionType == JPH::EMotionType::Static},
                {"layer", (int)settings.mObjectLayer},
                {"shape", shape->ToJSON()}};
        }

        void OnTransformed(vke_common::Transform &param)
        {
        }

    private:
        void init(const vke_common::Transform &transform,
                  bool isStatic, JPH::ObjectLayer layer)
        {
            const glm::vec3 position = transform.GetGlobalPosition();
            const glm::quat rotation = transform.GetGlobalRotation();
            const JPH::EMotionType motionType = isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Kinematic;

            settings = JPH::BodyCreationSettings(shape->shapeRef,
                                                 JPH::RVec3(position.x, position.y, position.z),
                                                 JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                                 motionType, layer);
            settings.mIsSensor = true;
            settings.mAllowDynamicOrKinematic = true;
        }
    };
}

#endif
