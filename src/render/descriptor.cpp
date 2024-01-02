#include <render/render.hpp>
#include <iostream>
#include <fstream>
#include <render/resource.hpp>

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

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkDescriptorSet descriptorSet, DescriptorInfo &descriptorInfo, VkDescriptorBufferInfo *bufferInfo)
    {
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = descriptorInfo.bindingInfo.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = descriptorInfo.bindingInfo.descriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = bufferInfo;
        descriptorWrite.pImageInfo = nullptr;       // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional
        return descriptorWrite;
    }

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkDescriptorSet descriptorSet, DescriptorInfo &descriptorInfo, VkDescriptorImageInfo *imageInfo)
    {
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = descriptorInfo.bindingInfo.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = descriptorInfo.bindingInfo.descriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = imageInfo;     // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional
        return descriptorWrite;
    }
}