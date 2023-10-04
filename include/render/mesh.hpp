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
        glm::vec2 texCoord;
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
            CreateBufferAndTransferStaged(vertSize, vertData, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory);
            CreateBufferAndTransferStaged(index.size() * sizeof(uint32_t), index.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMemory);
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
    };
}

#endif