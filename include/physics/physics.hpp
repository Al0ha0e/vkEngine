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

#include <common.hpp>
#include <event.hpp>
#include <vector>
#include <functional>

namespace vke_physics
{
    namespace Layers
    {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr uint32_t NUM_LAYERS(2);
    };

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual uint32_t GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char *GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
        {
            switch ((BroadPhaseLayer::Type)inLayer)
            {
            case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
                return "NON_MOVING";
            case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
                return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
            }
        }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

    private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    // An example contact listener
    class ContactListener : public JPH::ContactListener
    {
    public:
        // See: ContactListener
        virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override
        {
            std::cout << "Contact validate callback" << std::endl;
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
        {
            std::cout << "A contact was added" << std::endl;
        }

        virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
        {
            std::cout << "A contact was persisted" << std::endl;
        }

        virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
        {
            std::cout << "A contact was removed" << std::endl;
        }
    };

    class BodyActivationListener : public JPH::BodyActivationListener
    {
    public:
        virtual void OnBodyActivated(const JPH::BodyID &inBodyID, uint64_t inBodyUserData) override
        {
            std::cout << "A body got activated" << std::endl;
        }

        virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, uint64_t inBodyUserData) override
        {
            std::cout << "A body went to sleep" << std::endl;
        }
    };

    class PhysicsManager
    {
    private:
        static PhysicsManager *instance;
        PhysicsManager() : accumulator(0.0f) {}
        ~PhysicsManager() {}

    public:
        const uint32_t maxBodies = 65536 * 4;
        const uint32_t numBodyMutexes = 0;
        const uint32_t maxBodyPairs = 65536 * 4;
        const uint32_t maxContactConstraints = 10240;
        const uint32_t collisionSteps = 1;
        const float stepTime = 1.0f / 60.0f;
        const JPH::Vec3 gravity = JPH::Vec3(0, -9.8, 0);

        static PhysicsManager *GetInstance()
        {
            return instance;
        }

        static PhysicsManager *Init()
        {
            instance = new PhysicsManager();
            instance->init();
            return instance;
        }

        static void Dispose()
        {
            instance->dispose();
            delete instance;
        }

        static void Update()
        {
            instance->update();
        }

        static vke_ds::id32_t RegisterUpdateListener(void *rigidbody, vke_common::EventHub<void>::callback_t &callback)
        {
            return instance->updates.AddEventListener(rigidbody, callback);
        }

        static void RemoveUpdateListener(vke_ds::id32_t id)
        {
            instance->updates.RemoveEventListener(id);
        }

        static JPH::BodyInterface &GetBodyInterface()
        {
            return instance->physics_system.GetBodyInterface();
        }

    private:
        void init();
        void dispose();
        void update();

        float accumulator;
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