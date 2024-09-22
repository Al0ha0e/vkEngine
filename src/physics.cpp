#include <physics.hpp>
#include <time.hpp>

namespace vke_physics
{
    PhysicsManager *PhysicsManager::instance;

    void PhysicsManager::init()
    {
        gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
        gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, physx::PxTolerancesScale(), true);
        gDispatcher = physx::PxDefaultCpuDispatcherCreate(2);

        physx::PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.8f, 0.0f);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

        gScene = gPhysics->createScene(sceneDesc);
    }

    void PhysicsManager::dispose()
    {
        gScene->release();
        gDispatcher->release();
        gPhysics->release();
        gFoundation->release();
    }

    void PhysicsManager::update()
    {
        accumulator += vke_common::TimeManager::GetInstance()->deltaTime;
        if (accumulator < stepSize)
            return;

        accumulator -= stepSize;

        gScene->simulate(stepSize);
        gScene->fetchResults(true);
        updates.DispatchEvent(nullptr);
    }
}