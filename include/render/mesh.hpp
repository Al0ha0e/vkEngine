#ifndef MESH_H
#define MESH_H

#include <render/environment.hpp>
#include <render/buffer.hpp>
#include <iostream>

namespace vke_render
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;

        Vertex() {}
        Vertex(glm::vec3 p, glm::vec3 n, glm::vec2 t) : pos(p), normal(n), texCoord(t) {}
    };

    class Mesh
    {
    public:
        std::string path;
        std::unique_ptr<DeviceBuffer> vertexBuffer;
        std::unique_ptr<DeviceBuffer> indexBuffer;
        size_t indexCnt;
        std::vector<std::shared_ptr<Mesh>> subMeshes;

        Mesh() : indexCnt(0), vertexBuffer(nullptr), indexBuffer(nullptr) {}

        Mesh(const std::string &path) : path(path), indexCnt(0), vertexBuffer(nullptr), indexBuffer(nullptr) {}

        Mesh(const std::string &path, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &index)
            : path(path),
              indexCnt(index.size()),
              vertexBuffer(std::make_unique<DeviceBuffer>(vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
              indexBuffer(std::make_unique<DeviceBuffer>(index.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            vertexBuffer->ToBuffer(0, vertices.data(), vertices.size() * sizeof(Vertex));
            indexBuffer->ToBuffer(0, index.data(), index.size() * sizeof(uint32_t));
        }

        Mesh(const std::string &path, size_t vertSize, const void *vertData, const std::vector<uint32_t> &index)
            : path(path),
              indexCnt(index.size()),
              vertexBuffer(std::make_unique<DeviceBuffer>(vertSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
              indexBuffer(std::make_unique<DeviceBuffer>(index.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            vertexBuffer->ToBuffer(0, vertData, vertSize);
            indexBuffer->ToBuffer(0, index.data(), index.size() * sizeof(uint32_t));
        }

        ~Mesh() {}

        void Render(VkCommandBuffer &commandBuffer) const
        {
            if (indexCnt > 0)
            {
                render(commandBuffer);
            }
            for (auto &submesh : subMeshes)
                submesh->Render(commandBuffer);
        }

    private:
        void render(VkCommandBuffer &commandBuffer) const
        {
            VkBuffer vertexBuffers[] = {vertexBuffer->buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, indexCnt, 1, 0, 0, 0);
        }
    };
}

#endif