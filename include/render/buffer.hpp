#ifndef BUFFER_H
#define BUFFER_H

#include <render/environment.hpp>

namespace vke_render
{
    class Buffer
    {
    public:
        size_t bufferSize;
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags properties;
        VmaAllocationCreateFlags vmaFlags;
        VmaMemoryUsage vmaUsage;
        VkBuffer buffer;
        VmaAllocation vmaAllocation;
        VmaAllocationInfo vmaAllocationInfo;

        Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
               VmaAllocationCreateFlags vmaFlags, VmaMemoryUsage vmaUsage)
            : bufferSize(size), usage(usage), properties(properties),
              vmaFlags(vmaFlags), vmaUsage(vmaUsage), buffer(nullptr), vmaAllocation(nullptr), vmaAllocationInfo{}
        {
            vke_render::RenderEnvironment::CreateBuffer(
                size, usage, properties,
                vmaFlags, vmaUsage,
                &buffer, &vmaAllocation, &vmaAllocationInfo);
        }

        Buffer(Buffer &&ori)
            : bufferSize(ori.bufferSize),
              usage(ori.usage),
              properties(ori.properties),
              vmaFlags(ori.vmaFlags),
              vmaUsage(ori.vmaUsage),
              buffer(ori.buffer),
              vmaAllocation(ori.vmaAllocation),
              vmaAllocationInfo(ori.vmaAllocationInfo)
        {
            ori.buffer = nullptr;
            ori.vmaAllocation = nullptr;
        }

        ~Buffer()
        {
            if (buffer)
                vmaDestroyBuffer(RenderEnvironment::GetInstance()->vmaAllocator, buffer, vmaAllocation);
        }
    };

    class HostCoherentBuffer : public Buffer
    {
    public:
        void *data;

        HostCoherentBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
            : Buffer(size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                     VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            data = vmaAllocationInfo.pMappedData;
        }

        HostCoherentBuffer(HostCoherentBuffer &&ori)
            : data(ori.data), Buffer(std::forward<HostCoherentBuffer>(ori)) {}

        void ToBuffer(size_t dstOffset, const void *src, size_t size)
        {
            memcpy((char *)data + dstOffset, src, size);
        }

        void FromBuffer(size_t srcOffset, void *dst, size_t size)
        {
            memcpy(dst, (char *)data + srcOffset, size);
        }
    };

    class DeviceBuffer : public Buffer
    {
    public:
        DeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
            : Buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     0, VMA_MEMORY_USAGE_GPU_ONLY) {}

        DeviceBuffer(DeviceBuffer &&ori) : Buffer(std::forward<DeviceBuffer>(ori)) {}

        ~DeviceBuffer() {}

        void ToBuffer(size_t dstOffset, const void *src, size_t size)
        {
            HostCoherentBuffer stagingBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            memcpy((char *)stagingBuffer.data, src, size);
            RenderEnvironment::CopyBuffer(stagingBuffer.buffer, buffer, size, 0, dstOffset);
        }

        void FromBuffer(size_t srcOffset, void *dst, size_t size)
        {
            HostCoherentBuffer stagingBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            RenderEnvironment::CopyBuffer(buffer, stagingBuffer.buffer, size, srcOffset, 0);
            memcpy(dst, (char *)stagingBuffer.data, size);
        }
    };

    class StagedBuffer : public Buffer
    {
    public:
        void *data;
        HostCoherentBuffer stagingBuffer;

        StagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
            : stagingBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
              Buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     0, VMA_MEMORY_USAGE_GPU_ONLY)
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            data = stagingBuffer.data;
        }

        StagedBuffer(StagedBuffer &&ori)
            : data(ori.data), stagingBuffer(std::move(ori.stagingBuffer)), Buffer(std::forward<StagedBuffer>(ori)) {}

        void ToBuffer(size_t dstOffset, const void *src, size_t size)
        {
            memcpy((char *)data + dstOffset, src, size);
            RenderEnvironment::CopyBuffer(stagingBuffer.buffer, buffer, size, dstOffset, dstOffset);
        }

        void FromBuffer(size_t srcOffset, void *dst, size_t size)
        {
            RenderEnvironment::CopyBuffer(buffer, stagingBuffer.buffer, size, srcOffset, srcOffset);
            memcpy(dst, (char *)data + srcOffset, size);
        }
    };
}

#endif