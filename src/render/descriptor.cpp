#include <render/descriptor.hpp>

namespace vke_render
{
    void InitDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding &dst,
                                        uint32_t binding,
                                        VkDescriptorType descriptorType,
                                        uint32_t descriptorCount,
                                        VkShaderStageFlags stageFlags,
                                        const VkSampler *pImmutableSamplers)
    {
        dst.binding = binding;
        dst.descriptorType = descriptorType;
        dst.descriptorCount = descriptorCount;
        dst.stageFlags = stageFlags;
        dst.pImmutableSamplers = pImmutableSamplers;
    }

    void InitDescriptorBufferInfo(VkDescriptorBufferInfo &bufferInfo,
                                  VkBuffer buffer,
                                  VkDeviceSize offset,
                                  VkDeviceSize range)
    {
        bufferInfo.buffer = buffer;
        bufferInfo.offset = offset;
        bufferInfo.range = range;
    }

#define COMMON_DS_WRITE_CONSTRUCT(binding, type)                    \
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; \
    descriptorWrite.pNext = nullptr;                                \
    descriptorWrite.dstSet = descriptorSet;                         \
    descriptorWrite.dstBinding = binding;                           \
    descriptorWrite.dstArrayElement = st;                           \
    descriptorWrite.descriptorType = type;                          \
    descriptorWrite.descriptorCount = cnt;

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     uint32_t binding,
                                                     VkDescriptorType descriptorType,
                                                     VkDescriptorBufferInfo *bufferInfo,
                                                     int st,
                                                     int cnt)
    {
        COMMON_DS_WRITE_CONSTRUCT(binding, descriptorType)
        descriptorWrite.pBufferInfo = bufferInfo;
        descriptorWrite.pImageInfo = nullptr;       // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional
        return descriptorWrite;
    }

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     uint32_t binding,
                                                     VkDescriptorType descriptorType,
                                                     VkDescriptorImageInfo *imageInfo,
                                                     int st,
                                                     int cnt)
    {
        COMMON_DS_WRITE_CONSTRUCT(binding, descriptorType)
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = imageInfo;     // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional
        return descriptorWrite;
    }
}