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
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;

        Buffer() = default;
        Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
            : bufferSize(size), usage(usage), properties(properties)
        {
            vke_render::RenderEnvironment::CreateBuffer(
                size, usage, properties,
                buffer, bufferMemory);
        }
    };

    class HostCoherentBuffer : public Buffer
    {
    public:
        void *data;
        HostCoherentBuffer() = default;
        HostCoherentBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
            : Buffer(size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkMapMemory(logicalDevice, bufferMemory, 0, size, 0, &data);
        }

        void ToBuffer(size_t dstOffset, void *src, size_t size)
        {
            memcpy((char *)data + dstOffset, src, size);
        }

        void FromBuffer(size_t srcOffset, void *dst, size_t size)
        {
            memcpy(dst, (char *)data + srcOffset, size);
        }

        void Dispose()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkUnmapMemory(logicalDevice, bufferMemory);
            vkDestroyBuffer(logicalDevice, buffer, nullptr);
            vkFreeMemory(logicalDevice, bufferMemory, nullptr);
        }
    };

    class DeviceBuffer : public Buffer
    {
    public:
        DeviceBuffer() = default;
        DeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
            : Buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
        }

        void ToBuffer(size_t dstOffset, void *src, size_t size)
        {
            Buffer stagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            void *data;
            vkMapMemory(logicalDevice, stagingBuffer.bufferMemory, 0, bufferSize, 0, &data);
            memcpy((char *)data + dstOffset, src, size);
            RenderEnvironment::CopyBuffer(stagingBuffer.buffer, buffer, bufferSize);
            vkUnmapMemory(logicalDevice, stagingBuffer.bufferMemory);
            vkDestroyBuffer(logicalDevice, stagingBuffer.buffer, nullptr);
            vkFreeMemory(logicalDevice, stagingBuffer.bufferMemory, nullptr);
        }

        void FromBuffer(size_t srcOffset, void *dst, size_t size)
        {
            Buffer stagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            RenderEnvironment::CopyBuffer(buffer, stagingBuffer.buffer, bufferSize);
            void *data;
            vkMapMemory(logicalDevice, stagingBuffer.bufferMemory, 0, bufferSize, 0, &data);
            memcpy(dst, (char *)data + srcOffset, size);
            vkUnmapMemory(logicalDevice, stagingBuffer.bufferMemory);
            vkDestroyBuffer(logicalDevice, stagingBuffer.buffer, nullptr);
            vkFreeMemory(logicalDevice, stagingBuffer.bufferMemory, nullptr);
        }

        void Dispose()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkDestroyBuffer(logicalDevice, buffer, nullptr);
            vkFreeMemory(logicalDevice, bufferMemory, nullptr);
        }
    };

    class StagedBuffer : public Buffer
    {
    public:
        void *data;
        Buffer stagingBuffer;

        StagedBuffer() = default;
        StagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
            : stagingBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
              Buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkMapMemory(logicalDevice, stagingBuffer.bufferMemory, 0, size, 0, &data);
        }

        void ToBuffer(size_t dstOffset, void *src, size_t size)
        {
            memcpy((char *)data + dstOffset, src, size);
            RenderEnvironment::CopyBuffer(stagingBuffer.buffer, buffer, bufferSize);
        }

        void FromBuffer(size_t srcOffset, void *dst, size_t size)
        {
            RenderEnvironment::CopyBuffer(buffer, stagingBuffer.buffer, bufferSize);
            memcpy(dst, (char *)data + srcOffset, size);
        }

        void Dispose()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkUnmapMemory(logicalDevice, stagingBuffer.bufferMemory);
            vkDestroyBuffer(logicalDevice, stagingBuffer.buffer, nullptr);
            vkFreeMemory(logicalDevice, stagingBuffer.bufferMemory, nullptr);
            vkDestroyBuffer(logicalDevice, buffer, nullptr);
            vkFreeMemory(logicalDevice, bufferMemory, nullptr);
        }
    };
}

#endif