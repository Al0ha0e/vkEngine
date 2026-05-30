#include <physics/physics.hpp>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace vke_physics
{
    PhysicsManager *PhysicsManager::instance;

    static bool FillRaycastHit(const JPH::PhysicsSystem &physicsSystem, const JPH::RRayCast &ray, const JPH::RayCastResult &rayHit, RaycastHit &outHit)
    {
        outHit.bodyID = rayHit.mBodyID;
        outHit.subShapeID = rayHit.mSubShapeID2;
        outHit.fraction = rayHit.mFraction;
        outHit.point = ray.GetPointOnRay(rayHit.mFraction);
        outHit.distance = ray.mDirection.Length() * rayHit.mFraction;

        JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), rayHit.mBodyID);
        if (!lock.Succeeded())
        {
            outHit.normal = JPH::Vec3::sZero();
            return false;
        }

        outHit.normal = lock.GetBody().GetWorldSpaceSurfaceNormal(rayHit.mSubShapeID2, outHit.point);
        return true;
    }

    void PhysicsManager::init()
    {
        broad_phase_layer_interface.SetConfig(config);
        object_vs_broadphase_layer_filter.SetConfig(config);
        object_vs_object_layer_filter.SetConfig(config);

        JPH::RegisterDefaultAllocator();

        // // Install trace and assert callbacks
        // Trace = TraceImpl;
        // JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(config.tempAllocatorSize);
        jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

        physics_system.Init(config.maxBodies, config.numBodyMutexes, config.maxBodyPairs, config.maxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
        physics_system.SetGravity(config.gravity);
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

    void PhysicsManager::fixedUpdate()
    {
        physics_system.Update(config.stepTime, config.collisionSteps, tempAllocator.get(), jobSystem.get());
        updates.DispatchEvent(nullptr);
    }

    bool PhysicsManager::raycast(JPH::RVec3Arg origin, JPH::Vec3Arg direction, float maxDistance, RaycastHit &outHit, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask)
    {
        if (maxDistance <= 0.0f)
            return false;

        const float directionLength = direction.Length();
        if (directionLength <= 0.000001f)
            return false;

        const JPH::Vec3 castDirection = direction * (maxDistance / directionLength);
        const JPH::RRayCast ray(origin, castDirection);
        const MaskBroadPhaseLayerFilter broadPhaseLayerFilter(broadPhaseLayerMask);
        const MaskObjectLayerFilter objectLayerFilter(objectLayerMask);

        JPH::RayCastResult rayHit;
        if (!physics_system.GetNarrowPhaseQuery().CastRay(ray, rayHit, broadPhaseLayerFilter, objectLayerFilter))
            return false;

        return FillRaycastHit(physics_system, ray, rayHit, outHit);
    }

    uint32_t PhysicsManager::raycastAll(JPH::RVec3Arg origin, JPH::Vec3Arg direction, float maxDistance, RaycastHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask)
    {
        if (outHits == nullptr || maxHits == 0 || maxDistance <= 0.0f)
            return 0;

        const float directionLength = direction.Length();
        if (directionLength <= 0.000001f)
            return 0;

        const JPH::Vec3 castDirection = direction * (maxDistance / directionLength);
        const JPH::RRayCast ray(origin, castDirection);
        const JPH::RayCastSettings settings;
        const MaskBroadPhaseLayerFilter broadPhaseLayerFilter(broadPhaseLayerMask);
        const MaskObjectLayerFilter objectLayerFilter(objectLayerMask);
        JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
        physics_system.GetNarrowPhaseQuery().CastRay(ray, settings, collector, broadPhaseLayerFilter, objectLayerFilter);
        collector.Sort();

        uint32_t written = 0;
        for (const JPH::RayCastResult &rayHit : collector.mHits)
        {
            if (written >= maxHits)
                break;

            if (FillRaycastHit(physics_system, ray, rayHit, outHits[written]))
                ++written;
        }

        return written;
    }

    uint32_t PhysicsManager::collidePoint(JPH::RVec3Arg point, CollidePointHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask)
    {
        if (outHits == nullptr || maxHits == 0)
            return 0;

        JPH::AllHitCollisionCollector<JPH::CollidePointCollector> collector;
        const MaskBroadPhaseLayerFilter broadPhaseLayerFilter(broadPhaseLayerMask);
        const MaskObjectLayerFilter objectLayerFilter(objectLayerMask);
        physics_system.GetNarrowPhaseQuery().CollidePoint(point, collector, broadPhaseLayerFilter, objectLayerFilter);

        uint32_t written = 0;
        for (const JPH::CollidePointResult &hit : collector.mHits)
        {
            if (written >= maxHits)
                break;

            outHits[written].bodyID = hit.mBodyID;
            outHits[written].subShapeID = hit.mSubShapeID2;
            ++written;
        }

        return written;
    }

    uint32_t PhysicsManager::collideShape(const JPH::Shape *shape, JPH::Vec3Arg scale, JPH::RVec3Arg position, JPH::QuatArg rotation, CollideShapeHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask)
    {
        if (shape == nullptr || outHits == nullptr || maxHits == 0)
            return 0;

        const JPH::RMat44 worldTransform = JPH::RMat44::sRotationTranslation(rotation, position);
        const JPH::RMat44 centerOfMassTransform = worldTransform.PreTranslated(shape->GetCenterOfMass());
        const JPH::CollideShapeSettings settings;
        const MaskBroadPhaseLayerFilter broadPhaseLayerFilter(broadPhaseLayerMask);
        const MaskObjectLayerFilter objectLayerFilter(objectLayerMask);
        JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
        physics_system.GetNarrowPhaseQuery().CollideShape(shape, scale, centerOfMassTransform, settings, JPH::RVec3::sZero(), collector, broadPhaseLayerFilter, objectLayerFilter);
        collector.Sort();

        uint32_t written = 0;
        for (const JPH::CollideShapeResult &hit : collector.mHits)
        {
            if (written >= maxHits)
                break;

            outHits[written].bodyID = hit.mBodyID2;
            outHits[written].subShapeID1 = hit.mSubShapeID1;
            outHits[written].subShapeID2 = hit.mSubShapeID2;
            outHits[written].contactPointOn1 = hit.mContactPointOn1;
            outHits[written].contactPointOn2 = hit.mContactPointOn2;
            outHits[written].penetrationAxis = hit.mPenetrationAxis;
            outHits[written].penetrationDepth = hit.mPenetrationDepth;
            ++written;
        }

        return written;
    }

    uint32_t PhysicsManager::castShape(const JPH::Shape *shape, JPH::Vec3Arg scale, JPH::RVec3Arg position, JPH::QuatArg rotation, JPH::Vec3Arg direction, float maxDistance, ShapeCastHit *outHits, uint32_t maxHits, uint32_t broadPhaseLayerMask, uint32_t objectLayerMask)
    {
        if (shape == nullptr || outHits == nullptr || maxHits == 0 || maxDistance <= 0.0f)
            return 0;

        const float directionLength = direction.Length();
        if (directionLength <= 0.000001f)
            return 0;

        const JPH::Vec3 castDirection = direction * (maxDistance / directionLength);
        const JPH::RMat44 worldTransform = JPH::RMat44::sRotationTranslation(rotation, position);
        const JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(shape, scale, worldTransform, castDirection);
        const JPH::ShapeCastSettings settings;
        const MaskBroadPhaseLayerFilter broadPhaseLayerFilter(broadPhaseLayerMask);
        const MaskObjectLayerFilter objectLayerFilter(objectLayerMask);
        JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
        physics_system.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector, broadPhaseLayerFilter, objectLayerFilter);
        collector.Sort();

        uint32_t written = 0;
        for (const JPH::ShapeCastResult &hit : collector.mHits)
        {
            if (written >= maxHits)
                break;

            outHits[written].bodyID = hit.mBodyID2;
            outHits[written].subShapeID1 = hit.mSubShapeID1;
            outHits[written].subShapeID2 = hit.mSubShapeID2;
            outHits[written].contactPointOn1 = hit.mContactPointOn1;
            outHits[written].contactPointOn2 = hit.mContactPointOn2;
            outHits[written].penetrationAxis = hit.mPenetrationAxis;
            outHits[written].penetrationDepth = hit.mPenetrationDepth;
            outHits[written].fraction = hit.mFraction;
            outHits[written].distance = maxDistance * hit.mFraction;
            outHits[written].isBackFaceHit = hit.mIsBackFaceHit;
            ++written;
        }

        return written;
    }
}