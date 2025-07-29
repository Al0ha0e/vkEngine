#include <physics/physics.hpp>
#include <time.hpp>

namespace vke_physics
{
    PhysicsManager *PhysicsManager::instance;

    void PhysicsManager::init()
    {
        JPH::RegisterDefaultAllocator();

        // // Install trace and assert callbacks
        // Trace = TraceImpl;
        // JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
        jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

        physics_system.Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
        physics_system.SetGravity(gravity);
        physics_system.SetBodyActivationListener(&body_activation_listener);
        physics_system.SetContactListener(&contact_listener);

        JPH::BodyInterface &body_interface = physics_system.GetBodyInterface();

        physics_system.OptimizeBroadPhase();
    }

    void PhysicsManager::dispose()
    {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void PhysicsManager::update()
    {
        accumulator += vke_common::TimeManager::GetInstance()->deltaTime;
        if (accumulator < stepTime)
            return;

        accumulator -= stepTime;
        physics_system.Update(stepTime, collisionSteps, tempAllocator.get(), jobSystem.get());
        updates.DispatchEvent(nullptr);
    }
}