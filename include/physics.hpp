#ifndef PHYSICS_H
#define PHYSICS_H

#include <common.hpp>
#include <event.hpp>
#include <physx/PxPhysicsAPI.h>
#include <vector>
#include <functional>

namespace vke_physics
{
    struct PhysicsMaterial
    {
        vke_common::AssetHandle handle;
        physx::PxMaterial *material;

        PhysicsMaterial() = default;
        PhysicsMaterial(const vke_common::AssetHandle hdl, physx::PxMaterial *mat)
            : handle(hdl), material(mat) {}
    };

    class PhysicsManager
    {
    private:
        static PhysicsManager *instance;
        PhysicsManager() : accumulator(0.0f), stepSize(1.0f / 60.0f) {}
        ~PhysicsManager() {}

    public:
        physx::PxPhysics *gPhysics;
        physx::PxScene *gScene;
        float stepSize;

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

    private:
        void init();
        void dispose();
        void update();

        float accumulator;

        physx::PxDefaultAllocator gAllocator;
        physx::PxDefaultErrorCallback gErrorCallback;
        physx::PxFoundation *gFoundation;
        physx::PxDefaultCpuDispatcher *gDispatcher;

        vke_common::EventHub<void> updates;
    };
}

#endif