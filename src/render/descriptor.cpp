#include <render/render.hpp>
#include <iostream>
#include <fstream>
#include <asset.hpp>

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

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkDescriptorSet descriptorSet,
                                                     DescriptorInfo &descriptorInfo,
                                                     int st,
                                                     int cnt,
                                                     VkDescriptorBufferInfo *bufferInfo)
    {
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = descriptorInfo.bindingInfo.binding;
        descriptorWrite.dstArrayElement = st;
        descriptorWrite.descriptorType = descriptorInfo.bindingInfo.descriptorType;
        descriptorWrite.descriptorCount = cnt;
        descriptorWrite.pBufferInfo = bufferInfo;
        descriptorWrite.pImageInfo = nullptr;       // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional
        return descriptorWrite;
    }

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkDescriptorSet descriptorSet,
                                                     DescriptorInfo &descriptorInfo,
                                                     int st,
                                                     int cnt,
                                                     VkDescriptorImageInfo *imageInfo)
    {
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = descriptorInfo.bindingInfo.binding;
        descriptorWrite.dstArrayElement = st;
        descriptorWrite.descriptorType = descriptorInfo.bindingInfo.descriptorType;
        descriptorWrite.descriptorCount = cnt;
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = imageInfo;     // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional
        return descriptorWrite;
    }

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkDescriptorSet descriptorSet, DescriptorInfo &descriptorInfo, VkDescriptorBufferInfo *bufferInfo)
    {
        return ConstructDescriptorSetWrite(descriptorSet, descriptorInfo, 0, 1, bufferInfo);
    }

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkDescriptorSet descriptorSet, DescriptorInfo &descriptorInfo, VkDescriptorImageInfo *imageInfo)
    {
        return ConstructDescriptorSetWrite(descriptorSet, descriptorInfo, 0, 1, imageInfo);
    }
}