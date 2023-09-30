#ifndef MESH_H
#define MESH_H

#include <render/environment.hpp>
#include <iostream>

namespace vke_render
{
    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;
    };

    class Mesh
    {
    public:
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        size_t indexCnt;

        Mesh() = default;

        Mesh(size_t vertSize, void *vertData, std::vector<uint32_t> &index) : indexCnt(index.size())
        {
            createBufferAndTransfer(vertSize, vertData, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory);
            createBufferAndTransfer(index.size(), index.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMemory);
        }

        ~Mesh()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
            vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
            vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
            vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
        }

        void Render(VkCommandBuffer &commandBuffer)
        {
            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, indexCnt, 1, 0, 0, 0);
        }

    private:
        void createBufferAndTransfer(
            size_t size,
            void *srcdata,
            VkBufferUsageFlags flags,
            VkBuffer &buffer,
            VkDeviceMemory &bufferMemory)
        {
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            RenderEnvironment::CreateBuffer(size,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            stagingBuffer,
                                            stagingBufferMemory);

            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            void *dstdata;
            vkMapMemory(logicalDevice, stagingBufferMemory, 0, size, 0, &dstdata);
            memcpy(dstdata, srcdata, size);
            vkUnmapMemory(logicalDevice, stagingBufferMemory);

            RenderEnvironment::CreateBuffer(size,
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | flags,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                            buffer,
                                            bufferMemory);

            RenderEnvironment::CopyBuffer(stagingBuffer, buffer, size);

            vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
            vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
        }
    };
}

#endif