#include <render/render.hpp>
#include <iostream>
#include <fstream>
#include <render/resource.hpp>

namespace vke_render
{
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
}