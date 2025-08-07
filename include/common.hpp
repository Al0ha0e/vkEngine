#ifndef COMMON_H
#define COMMON_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <stb/stb_image.h>

#define VKE_DEBUG

#ifdef VKE_DEBUG
const bool DEBUG_MODE = true;
#else
const bool DEBUG_MODE = false;
#endif

#ifndef COMPILE_TO_LIB
#define REL_DIR "."
#endif

namespace vke_common
{
    using AssetHandle = uint64_t;
}

#endif