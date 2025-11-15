#include <render/light.hpp>

namespace vke_render
{
    LightManager *LightManager::instance = nullptr;

    void LightManager::init()
    {
        directionalLightBuffer = std::make_unique<DeviceBuffer>(sizeof(DirectionalLight) * MAX_DIRECTIONAL_LIGHT_CNT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        DirectionalLight light(glm::normalize(glm::vec4(0, -1, 0, 0)), glm::vec4(1, 1, 1, 2));
        directionalLightBuffer->ToBuffer(0, &light, sizeof(DirectionalLight));
    }

    void LightManager::dispose() {}
}
