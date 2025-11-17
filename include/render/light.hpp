#ifndef LIGHT_H
#define LIGHT_H

#include <render/buffer.hpp>
#include <render/descriptor.hpp>

namespace vke_render
{
    const uint32_t MAX_DIRECTIONAL_LIGHT_CNT = 1;

    struct DirectionalLight
    {
        glm::vec4 direction; // xyz
        glm::vec4 color;     // w: intensity

        DirectionalLight() : direction(1, 0, 0, 0), color(0) {}
        DirectionalLight(glm::vec4 direction, glm::vec4 color) : direction(direction), color(color) {}
    };

    class LightManager
    {
    private:
        static LightManager *instance;
        LightManager() {}
        ~LightManager() {}
        LightManager(const LightManager &);
        LightManager &operator=(const LightManager);

    public:
        static LightManager *GetInstance()
        {
            return instance;
        }

        static LightManager *Init()
        {
            if (instance != nullptr)
                return instance;
            instance = new LightManager();
            instance->init();
            return instance;
        }

        static void Dispose()
        {
            instance->dispose();
            delete instance;
        }

        static void GetBindingInfo(std::vector<VkDescriptorSetLayoutBinding> &bindingInfos, DescriptorSetInfo &descriptorSetInfo)
        {
            VkDescriptorSetLayoutBinding directionalLightBinding{};
            directionalLightBinding.binding = 1;
            directionalLightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            directionalLightBinding.descriptorCount = 1;
            directionalLightBinding.stageFlags = VK_SHADER_STAGE_ALL;
            bindingInfos.push_back(directionalLightBinding);
            descriptorSetInfo.AddCnt(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
        }

        static void GetDescriptorSetWrite(std::vector<VkWriteDescriptorSet> &descriptorSetWrites, VkDescriptorSet descriptorSet)
        {
            VkDescriptorBufferInfo bufferInfo = instance->directionalLightBuffer->GetDescriptorBufferInfo();
            descriptorSetWrites.push_back(VkWriteDescriptorSet{});
            ConstructDescriptorSetWrite(descriptorSetWrites[descriptorSetWrites.size() - 1], descriptorSet, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        }

    private:
        void init();
        void dispose();

        std::unique_ptr<DeviceBuffer> directionalLightBuffer;
    };
}

#endif