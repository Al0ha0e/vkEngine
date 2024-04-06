#include <iostream>
#include <common.hpp>

#include <physx/PxPhysicsAPI.h>

static physx::PxDefaultAllocator gAllocator;
static physx::PxDefaultErrorCallback gErrorCallback;

int main()
{
    physx::PxFoundation *gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    physx::PxPvd *gPvd = PxCreatePvd(*gFoundation);
    std::cout << "??\n";
    gPvd->release();
    PX_RELEASE(gFoundation);
    return 0;
}