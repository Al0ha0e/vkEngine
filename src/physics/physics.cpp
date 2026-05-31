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
#include <algorithm>

namespace vke_physics
{
    PhysicsManager *PhysicsManager::instance;

    static constexpr uint32_t InvalidEntityID = UINT32_MAX;

    void ContactListener::OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)
    {
        PhysicsManager::RecordContactEvent(ContactEventType::Added, inBody1, inBody2, inManifold, ioSettings);
    }

    void ContactListener::OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)
    {
        PhysicsManager::RecordContactEvent(ContactEventType::Persisted, inBody1, inBody2, inManifold, ioSettings);
    }

    void ContactListener::OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair)
    {
        PhysicsManager::RecordContactRemoved(inSubShapePair);
    }

    static bool FillBodyMetadata(const JPH::PhysicsSystem &physicsSystem, JPH::BodyID bodyID, uint32_t &outEntity, bool &outIsSensor)
    {
        JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), bodyID);
        if (!lock.Succeeded())
        {
            outEntity = InvalidEntityID;
            outIsSensor = false;
            return false;
        }

        const JPH::Body &body = lock.GetBody();
        outEntity = static_cast<uint32_t>(body.GetUserData());
        outIsSensor = body.IsSensor();
        return true;
    }

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
            outHit.entity = InvalidEntityID;
            outHit.isSensor = false;
            outHit.normal = JPH::Vec3::sZero();
            return false;
        }

        const JPH::Body &body = lock.GetBody();
        outHit.entity = static_cast<uint32_t>(body.GetUserData());
        outHit.isSensor = body.IsSensor();
        outHit.normal = body.GetWorldSpaceSurfaceNormal(rayHit.mSubShapeID2, outHit.point);
        return true;
    }

    void PhysicsManager::init()
    {
        broadPhaseLayerInterface.SetConfig(config);
        objectVsBroadphaseLayerFilter.SetConfig(config);
        objectVsObjectLayerFilter.SetConfig(config);
        contactEvents.resize(config.maxContactConstraints);
        contactEventsReadIndex = 0;
        contactEventsTailIndex = 0;

        JPH::RegisterDefaultAllocator();

        // // Install trace and assert callbacks
        // Trace = TraceImpl;
        // JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(config.tempAllocatorSize);
        jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

        physicsSystem.Init(config.maxBodies, config.numBodyMutexes, config.maxBodyPairs, config.maxContactConstraints, broadPhaseLayerInterface, objectVsBroadphaseLayerFilter, objectVsObjectLayerFilter);
        physicsSystem.SetGravity(config.gravity);
        physicsSystem.SetBodyActivationListener(&bodyActivationListener);
        physicsSystem.SetContactListener(&contactListener);

        JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

        physicsSystem.OptimizeBroadPhase();
    }

    void PhysicsManager::dispose()
    {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void PhysicsManager::fixedUpdate()
    {
        physicsSystem.Update(config.stepTime, config.collisionSteps, tempAllocator.get(), jobSystem.get());
        updates.DispatchEvent(nullptr);
    }

    void PhysicsManager::recordContactEvent(ContactEventType type, const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &manifold, const JPH::ContactSettings &settings)
    {
        ContactEvent event{};
        event.type = type;
        event.bodyID1 = body1.GetID();
        event.bodyID2 = body2.GetID();
        event.entity1 = static_cast<uint32_t>(body1.GetUserData());
        event.entity2 = static_cast<uint32_t>(body2.GetUserData());
        event.subShapeID1 = manifold.mSubShapeID1;
        event.subShapeID2 = manifold.mSubShapeID2;
        event.normal = manifold.mWorldSpaceNormal;
        event.penetrationDepth = manifold.mPenetrationDepth;
        event.isSensor = settings.mIsSensor;

        if (manifold.mRelativeContactPointsOn1.size() > 0 && manifold.mRelativeContactPointsOn2.size() > 0)
        {
            event.point1 = manifold.GetWorldSpaceContactPointOn1(0);
            event.point2 = manifold.GetWorldSpaceContactPointOn2(0);
        }
        else
        {
            event.point1 = JPH::RVec3::sZero();
            event.point2 = JPH::RVec3::sZero();
        }

        const JPH::SubShapeIDPair key(event.bodyID1, event.subShapeID1, event.bodyID2, event.subShapeID2);
        std::lock_guard<std::mutex> lock(contactEventsTailMutex);
        if (!contactEvents.empty())
        {
            contactEvents[contactEventsTailIndex % contactEvents.size()] = event;
            ++contactEventsTailIndex;
        }
        activeContacts[key] = event;
    }

    void PhysicsManager::recordContactRemoved(const JPH::SubShapeIDPair &subShapePair)
    {
        std::lock_guard<std::mutex> lock(contactEventsTailMutex);
        auto it = activeContacts.find(subShapePair);
        if (it == activeContacts.end())
            return;

        ContactEvent event = it->second;
        event.type = ContactEventType::Removed;
        if (!contactEvents.empty())
        {
            contactEvents[contactEventsTailIndex % contactEvents.size()] = event;
            ++contactEventsTailIndex;
        }
        activeContacts.erase(it);
    }

    uint32_t PhysicsManager::getContactEventCount()
    {
        std::lock_guard<std::mutex> lock(contactEventsTailMutex);
        const uint64_t unreadCount = contactEventsTailIndex - contactEventsReadIndex;
        return static_cast<uint32_t>(std::min<uint64_t>(unreadCount, contactEvents.size()));
    }

    uint32_t PhysicsManager::getContactEvents(ContactEvent *outEvents, uint32_t maxEvents)
    {
        if (outEvents == nullptr || maxEvents == 0)
            return 0;

        std::lock_guard<std::mutex> lock(contactEventsTailMutex);
        if (contactEvents.empty())
            return 0;

        const uint64_t oldestAvailableIndex = contactEventsTailIndex > contactEvents.size() ? contactEventsTailIndex - contactEvents.size() : 0;
        const uint64_t readIndex = std::max(contactEventsReadIndex, oldestAvailableIndex);
        const uint64_t unreadCount = contactEventsTailIndex - readIndex;
        const uint32_t count = std::min<uint32_t>(maxEvents, static_cast<uint32_t>(unreadCount));
        for (uint32_t i = 0; i < count; ++i)
            outEvents[i] = contactEvents[(readIndex + i) % contactEvents.size()];

        contactEventsReadIndex = readIndex + count;
        return count;
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
        if (!physicsSystem.GetNarrowPhaseQuery().CastRay(ray, rayHit, broadPhaseLayerFilter, objectLayerFilter))
            return false;

        return FillRaycastHit(physicsSystem, ray, rayHit, outHit);
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
        physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector, broadPhaseLayerFilter, objectLayerFilter);
        collector.Sort();

        uint32_t written = 0;
        for (const JPH::RayCastResult &rayHit : collector.mHits)
        {
            if (written >= maxHits)
                break;

            if (FillRaycastHit(physicsSystem, ray, rayHit, outHits[written]))
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
        physicsSystem.GetNarrowPhaseQuery().CollidePoint(point, collector, broadPhaseLayerFilter, objectLayerFilter);

        uint32_t written = 0;
        for (const JPH::CollidePointResult &hit : collector.mHits)
        {
            if (written >= maxHits)
                break;

            outHits[written].bodyID = hit.mBodyID;
            outHits[written].subShapeID = hit.mSubShapeID2;
            if (FillBodyMetadata(physicsSystem, hit.mBodyID, outHits[written].entity, outHits[written].isSensor))
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
        physicsSystem.GetNarrowPhaseQuery().CollideShape(shape, scale, centerOfMassTransform, settings, JPH::RVec3::sZero(), collector, broadPhaseLayerFilter, objectLayerFilter);
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
            if (FillBodyMetadata(physicsSystem, hit.mBodyID2, outHits[written].entity, outHits[written].isSensor))
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
        physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector, broadPhaseLayerFilter, objectLayerFilter);
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
            if (FillBodyMetadata(physicsSystem, hit.mBodyID2, outHits[written].entity, outHits[written].isSensor))
                ++written;
        }

        return written;
    }
}