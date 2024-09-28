#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <physics.hpp>
#include <resource.hpp>
#include <gameobject.hpp>

namespace vke_physics
{
    class RigidBody : public vke_common::Component
    {
    public:
        physx::PxRigidActor *rigidActor;
        std::unique_ptr<physx::PxGeometry> geometry;
        std::shared_ptr<PhysicsMaterial> material;
        physx::PxShapeFlags shapeFlags;

        RigidBody(std::unique_ptr<physx::PxGeometry> &&geom,
                  std::shared_ptr<PhysicsMaterial> &mat,
                  vke_common::GameObject *obj)
            : geometry(std::forward<std::unique_ptr<physx::PxGeometry> &&>(geom)), material(mat), Component(obj)
        {
            init(obj->isStatic);
        }

        RigidBody(vke_common::GameObject *obj, nlohmann::json &json) : Component(obj)
        {
            parseGeometryJSON(json["shape"]);
            material = vke_common::ResourceManager::LoadPhysicsMaterial(json["material"]);
            bool isStatic = json["static"];
            init(isStatic);
        }

        ~RigidBody()
        {
            PhysicsManager::GetInstance()->gScene->removeActor(*rigidActor);
            PhysicsManager::RemoveUpdateListener(physicsUpdateListenerID);
        }

        std::string ToJSON() override
        {
            std::string ret = "{\n\"type\":\"rigidbody\",\n";
            ret += "\"static\": " + std::string((rigidActor->getType() == physx::PxActorType::eRIGID_STATIC) ? "true" : "false") + ",\n";
            ret += "\"material\": \"" + material->path + "\",\n";
            ret += "\"shape\": " + genGeometryJSON();
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
            physx::PxTransform transform = rigidbody->rigidActor->getGlobalPose();
            rigidbody->gameObject->SetLocalPosition(glm::vec3(transform.p.x, transform.p.y, transform.p.z));
            rigidbody->gameObject->SetLocalRotation(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
            // TODO Set Global
        }

    private:
        int physicsUpdateListenerID;

        void init(bool isStatic)
        {
            shapeFlags = physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE;
            physx::PxPhysics *gPhysics = PhysicsManager::GetInstance()->gPhysics;
            vke_common::TransformParameter &param = gameObject->transform;

            physx::PxTransform transform(param.position.x, param.position.y, param.position.z,
                                         physx::PxQuat(param.rotation.x, param.rotation.y, param.rotation.z, param.rotation.w));
            physx::PxShape *shape;

            if (isStatic)
            {
                rigidActor = gPhysics->createRigidStatic(transform);
                shape = physx::PxRigidActorExt::createExclusiveShape(*rigidActor, *geometry, *(material->material), shapeFlags);
                if (geometry->getType() == physx::PxGeometryType::Enum::eCAPSULE ||
                    geometry->getType() == physx::PxGeometryType::Enum::ePLANE)
                {
                    physx::PxTransform relativePose(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0, 0, 1)));
                    shape->setLocalPose(relativePose);
                }
            }
            else
            {
                physx::PxRigidDynamic *rigidDynamic = gPhysics->createRigidDynamic(transform);
                shape = physx::PxRigidActorExt::createExclusiveShape(*rigidDynamic, *geometry, *material->material, shapeFlags);

                if (geometry->getType() == physx::PxGeometryType::Enum::eCAPSULE)
                {
                    physx::PxTransform relativePose(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0, 0, 1)));
                    shape->setLocalPose(relativePose);
                    physx::PxRigidBodyExt::updateMassAndInertia(*rigidDynamic, 1.0f);
                }

                rigidActor = rigidDynamic;
            }

            PhysicsManager::GetInstance()->gScene->addActor(*rigidActor);
            physicsUpdateListenerID = PhysicsManager::RegisterUpdateListener(this, std::function<void(void *, void *)>(update));
        }

        std::string genGeometryJSON()
        {
            auto type = geometry->getType();
            std::string ret = "{ \"type\": " + std::to_string((int)type);
            switch (type)
            {
            case physx::PxGeometryType::eSPHERE:
            {
                float r = ((physx::PxSphereGeometry *)(geometry.get()))->radius;
                ret += ", \"r\": " + std::to_string(r);
            }
            break;
            case physx::PxGeometryType::eCAPSULE:
            {
                physx::PxCapsuleGeometry *capsule = (physx::PxCapsuleGeometry *)(geometry.get());
                ret += ", \"r\": " + std::to_string(capsule->radius) + ", \"h\": " + std::to_string(capsule->halfHeight); // TODO Direction
            }
            break;
            case physx::PxGeometryType::eBOX:
            {
                physx::PxBoxGeometry *box = (physx::PxBoxGeometry *)(geometry.get());
                ret += ", \"e\": [" + std::to_string(box->halfExtents.x) + ", " + std::to_string(box->halfExtents.y) + ", " + std::to_string(box->halfExtents.z) + "]";
            }
            break;
            case physx::PxGeometryType::ePLANE:
                break;
            default:
                break;
            }
            ret += " }";
            return ret;
        }

        void parseGeometryJSON(nlohmann::json &json)
        {
            physx::PxGeometryType::Enum type = json["type"];
            switch (type)
            {
            case physx::PxGeometryType::eSPHERE:
                geometry = std::make_unique<physx::PxSphereGeometry>((float)json["r"]);
                break;
            case physx::PxGeometryType::eCAPSULE:
                geometry = std::make_unique<physx::PxCapsuleGeometry>((float)json["r"], (float)json["h"]);
                break;
            case physx::PxGeometryType::eBOX:
            {
                auto &extent = json["e"];
                geometry = std::make_unique<physx::PxBoxGeometry>(extent[0], extent[1], extent[2]);
            }
            break;
            case physx::PxGeometryType::ePLANE:
                geometry = std::make_unique<physx::PxPlaneGeometry>();
                break;
            default:
                break;
            }
        }
    };
}

#endif