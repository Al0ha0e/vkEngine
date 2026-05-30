#ifndef PHYSICS_H
#define PHYSICS_H

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/SubShapeID.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <common.hpp>
#include <event.hpp>
#include <logger.hpp>
#include <physics/physics_config.hpp>
#include <vector>
#include <functional>

namespace vke_physics
{
    struct RaycastHit
    {
        JPH::BodyID bodyID;
        JPH::SubShapeID subShapeID;
        JPH::RVec3 point;
        JPH::Vec3 normal;
        float distance;
        float fraction;
    };

    struct CollidePointHit
    {
        JPH::BodyID bodyID;
        JPH::SubShapeID subShapeID;
    };

    struct CollideShapeHit
    {
        JPH::BodyID bodyID;
        JPH::SubShapeID subShapeID1;
        JPH::SubShapeID subShapeID2;
        JPH::Vec3 contactPointOn1;
        JPH::Vec3 contactPointOn2;
        JPH::Vec3 penetrationAxis;
        float penetrationDepth;
    };

    struct ShapeCastHit : public CollideShapeHit
    {
        float fraction;
        float distance;
        bool isBackFaceHit;
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        ObjectLayerPairFilterImpl() : mConfig(nullptr) {}

        void SetConfig(const PhysicsConfig &config) { mConfig = &config; }

        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            if (mConfig == nullptr || inObject1 >= mConfig->objectLayerCount || inObject2 >= mConfig->objectLayerCount)
                return false;

            return mConfig->objectLayerPairs[inObject1][inObject2];
        }

    private:
        const PhysicsConfig *mConfig;
    };

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl() : mConfig(nullptr) {}

        void SetConfig(const PhysicsConfig &config) { mConfig = &config; }

        virtual uint32_t GetNumBroadPhaseLayers() const override
        {
            return mConfig == nullptr ? DefaultBroadPhaseLayers::NUM_LAYERS : mConfig->broadPhaseLayerCount;
        }

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            if (mConfig == nullptr || inLayer >= mConfig->objectLayerCount)
                return JPH::BroadPhaseLayer(0);

            return mConfig->objectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char *GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
        {
            if (mConfig != nullptr && inLayer.GetValue() < mConfig->broadPhaseLayerCount)
                return mConfig->broadPhaseLayerNames[inLayer.GetValue()].c_str();

            switch ((BroadPhaseLayer::Type)inLayer)
            {
            case (BroadPhaseLayer::Type)DefaultBroadPhaseLayers::NON_MOVING:
                return "NON_MOVING";
            case (BroadPhaseLayer::Type)DefaultBroadPhaseLayers::MOVING:
                return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
            }
        }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

    private:
        const PhysicsConfig *mConfig;
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        ObjectVsBroadPhaseLayerFilterImpl() : mConfig(nullptr) {}

        void SetConfig(const PhysicsConfig &config) { mConfig = &config; }

        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            if (mConfig == nullptr || inLayer1 >= mConfig->objectLayerCount || inLayer2.GetValue() >= mConfig->broadPhaseLayerCount)
                return false;

            return mConfig->objectVsBroadPhaseLayerPairs[inLayer1][inLayer2.GetValue()];
        }

    private:
        const PhysicsConfig *mConfig;
    };

    class MaskBroadPhaseLayerFilter final : public JPH::BroadPhaseLayerFilter
    {
    public:
        explicit MaskBroadPhaseLayerFilter(uint32_t mask) : mMask(mask) {}

        virtual bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
        {
            const uint32_t layer = inLayer.GetValue();
            return layer < MaxBroadPhaseLayers && (mMask & (1u << layer)) != 0;
        }

    private:
        uint32_t mMask;
    };

    class MaskObjectLayerFilter final : public JPH::ObjectLayerFilter
    {
    public:
        explicit MaskObjectLayerFilter(uint32_t mask) : mMask(mask) {}

        virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override
        {
            return inLayer < MaxObjectLayers && (mMask & (1u << inLayer)) != 0;
        }

    private:
        uint32_t mMask;
    };

    // An example contact listener
    class ContactListener : public JPH::ContactListener
    {
    public:
        // See: ContactListener
        virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override
        {
            // VKE_LOG_INFO("Contact validate callback")
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
        {
            // VKE_LOG_INFO("A contact was added")
        }

        virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
        {
            // VKE_LOG_INFO("A contact was persisted")
        }

        virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
        {
            // VKE_LOG_INFO("A contact was removed")
        }
    };

    class BodyActivationListener : public JPH::BodyActivationListener
    {
    public:
        virtual void OnBodyActivated(const JPH::BodyID &inBodyID, uint64_t inBodyUserData) override
        {
            VKE_LOG_INFO("A body got activated")
        }

        virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, uint64_t inBodyUserData) override
        {
            VKE_LOG_INFO("A body went to sleep")
        }
    };

    class PhysicsManager
    {
    private:
        static PhysicsManager *instance;
        explicit PhysicsManager(const PhysicsConfig &physicsConfig) : config(physicsConfig) {}
        ~PhysicsManager() {}

    public:
        static PhysicsManager *GetInstance()
        {
            return instance;
        }

        static PhysicsManager *Init(const PhysicsConfig &config = PhysicsConfig())
        {
            instance = new PhysicsManager(config);
            instance->init();
            return instance;
        }

        static void Dispose()
        {
            instance->dispose();
            delete instance;
        }

        static void FixedUpdate()
        {
            instance->fixedUpdate();
        }

        static vke_ds::id32_t RegisterUpdateListener(void *rigidbody, vke_common::EventHub<void>::callback_t &callback)
        {
            return instance->updates.AddEventListener(rigidbody, callback);
        }

        static vke_ds::id32_t RegisterUpdateListener(void *rigidbody, vke_common::EventHub<void>::callback_t &&callback)
        {
            return instance->updates.AddEventListener(rigidbody, std::move(callback));
        }

        static void RemoveUpdateListener(vke_ds::id32_t id)
        {
            instance->updates.RemoveEventListener(id);
        }

        static JPH::BodyInterface &GetBodyInterface()
        {
            return instance->physics_system.GetBodyInterface();
        }

        static JPH::PhysicsSystem &GetPhysicsSystem()
        {
            return instance->physics_system;
        }

        static JPH::TempAllocator &GetTempAllocator()
        {
            return *instance->tempAllocator;
        }

        static const PhysicsConfig &GetConfig()
        {
            return instance->config;
        }

        static JPH::ObjectLayer AddObjectLayer(const std::string &name, JPH::BroadPhaseLayer broadPhaseLayer = JPH::BroadPhaseLayer(0))
        {
            return instance->config.AddObjectLayer(name, broadPhaseLayer);
        }

        static JPH::BroadPhaseLayer AddBroadPhaseLayer(const std::string &name)
        {
            return instance->config.AddBroadPhaseLayer(name);
        }

        static void SetObjectLayerBroadPhaseLayer(JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer)
        {
            instance->config.SetObjectLayerBroadPhaseLayer(objectLayer, broadPhaseLayer);
        }

        static void SetObjectLayerCollision(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2, bool shouldCollide)
        {
            instance->config.SetObjectLayerCollision(layer1, layer2, shouldCollide);
        }

        static void SetObjectVsBroadPhaseLayerCollision(JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer, bool shouldCollide)
        {
            instance->config.SetObjectVsBroadPhaseLayerCollision(objectLayer, broadPhaseLayer, shouldCollide);
        }

        static bool Raycast(JPH::RVec3Arg origin, JPH::Vec3Arg direction, float maxDistance, RaycastHit &outHit, uint32_t broadPhaseLayerMask = AllPhysicsLayersMask, uint32_t objectLayerMask = AllPhysicsLayersMask)
        {
            return instance->raycast(origin, direction, maxDistance, outHit, broadPhaseLayerMask, objectLayerMask);
        }

        static uint32_t RaycastAll(JPH::RVec3Arg origin, JPH::Vec3Arg direction, float maxDistance, RaycastHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask = AllPhysicsLayersMask, uint32_t objectLayerMask = AllPhysicsLayersMask)
        {
            return instance->raycastAll(origin, direction, maxDistance, outHits, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        static uint32_t CollidePoint(JPH::RVec3Arg point, CollidePointHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask = AllPhysicsLayersMask, uint32_t objectLayerMask = AllPhysicsLayersMask)
        {
            return instance->collidePoint(point, outHits, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        static uint32_t CollideShape(const JPH::Shape *shape, JPH::Vec3Arg scale, JPH::RVec3Arg position, JPH::QuatArg rotation, CollideShapeHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask = AllPhysicsLayersMask, uint32_t objectLayerMask = AllPhysicsLayersMask)
        {
            return instance->collideShape(shape, scale, position, rotation, outHits, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

        static uint32_t CastShape(const JPH::Shape *shape, JPH::Vec3Arg scale, JPH::RVec3Arg position, JPH::QuatArg rotation, JPH::Vec3Arg direction, float maxDistance, ShapeCastHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask = AllPhysicsLayersMask, uint32_t objectLayerMask = AllPhysicsLayersMask)
        {
            return instance->castShape(shape, scale, position, rotation, direction, maxDistance, outHits, maxHits, broadPhaseLayerMask, objectLayerMask);
        }

    private:
        void init();
        void dispose();
        void fixedUpdate();
        bool raycast(JPH::RVec3Arg origin, JPH::Vec3Arg direction, float maxDistance, RaycastHit &outHit, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask);
        uint32_t raycastAll(JPH::RVec3Arg origin, JPH::Vec3Arg direction, float maxDistance, RaycastHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask);
        uint32_t collidePoint(JPH::RVec3Arg point, CollidePointHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask);
        uint32_t collideShape(const JPH::Shape *shape, JPH::Vec3Arg scale, JPH::RVec3Arg position, JPH::QuatArg rotation, CollideShapeHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask);
        uint32_t castShape(const JPH::Shape *shape, JPH::Vec3Arg scale, JPH::RVec3Arg position, JPH::QuatArg rotation, JPH::Vec3Arg direction, float maxDistance, ShapeCastHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask);

        PhysicsConfig config;
        std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
        std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;
        BPLayerInterfaceImpl broad_phase_layer_interface;
        ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
        ObjectLayerPairFilterImpl object_vs_object_layer_filter;
        BodyActivationListener body_activation_listener;
        ContactListener contact_listener;
        JPH::PhysicsSystem physics_system;
        vke_common::EventHub<void> updates;
    };
}

#endif