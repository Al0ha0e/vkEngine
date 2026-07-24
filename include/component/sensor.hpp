#ifndef SENSOR_H
#define SENSOR_H

#include <physics/shape.hpp>
#include <component/transform.hpp>

namespace vke_component
{
    struct SensorData
    {
        vke_physics::PhyscisShapeData shape;
        bool isStatic;
        JPH::EMotionQuality motionQuality = JPH::EMotionQuality::Discrete;
        JPH::ObjectLayer layer;

        SensorData() = default;
        SensorData(const nlohmann::json &json)
            : shape(json["shape"]),
              isStatic(json.value("isStatic", true)),
              motionQuality(json.value("motionQuality", JPH::EMotionQuality::Discrete)),
              layer(json["layer"]) {}
        nlohmann::json ToJSON() const
        {
            return {{"type", "sensor"},
                    {"isStatic", isStatic},
                    {"motionQuality", motionQuality},
                    {"layer", (int)layer},
                    {"shape", shape.ToJSON()}};
        }
    };

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
               const SensorData &componentData)
            : shape(std::make_shared<vke_physics::PhyscisShape>(componentData.shape))
        {
            init(transform, componentData.isStatic, componentData.layer);
            settings.mMotionQuality = componentData.motionQuality;
        }

        ~Sensor() {}

        void FillData(SensorData &data) const
        {
            data.isStatic = settings.mMotionType == JPH::EMotionType::Static;
            data.motionQuality = settings.mMotionQuality;
            data.layer = settings.mObjectLayer;
            shape->FillData(data.shape);
        }

        void LoadToEngine(entt::entity entity)
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

        void OnTransformed(vke_common::Transform &param)
        {
            const glm::vec3 position = param.GetGlobalPosition();
            const glm::quat rotation = param.GetGlobalRotation();
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
            settings.mMotionQuality = JPH::EMotionQuality::Discrete;
        }
    };
}

#endif
