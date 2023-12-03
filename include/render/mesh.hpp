#ifndef MESH_H
#define MESH_H

#include <render/environment.hpp>
#include <render/buffer.hpp>
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
        DeviceBuffer vertexBuffer;
        DeviceBuffer indexBuffer;
        size_t indexCnt;

        Mesh() = default;

        Mesh(size_t vertSize, void *vertData, std::vector<uint32_t> &index)
            : indexCnt(index.size()),
              vertexBuffer(vertSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), indexBuffer(index.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        {
            vertexBuffer.ToBuffer(0, vertData, vertSize);
            indexBuffer.ToBuffer(0, index.data(), index.size() * sizeof(uint32_t));
        }

        ~Mesh()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vertexBuffer.Dispose();
            indexBuffer.Dispose();
        }

        void Render(VkCommandBuffer &commandBuffer)
        {
            VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, indexCnt, 1, 0, 0, 0);
        }
    };
}

#endif