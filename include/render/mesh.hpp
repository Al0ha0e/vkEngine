#ifndef MESH_H
#define MESH_H

#include <render/buffer.hpp>
#include <concepts>
#include <iostream>

namespace vke_render
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec4 tangent;
        glm::vec2 texCoord;

        Vertex() {}
        Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec4 tangent, glm::vec2 texCoord)
            : pos(pos), normal(normal), tangent(tangent), texCoord(texCoord) {}
    };

    struct MeshInfo
    {
        size_t vertexOffset;
        size_t vertexSize;
        size_t indexOffset;
        size_t indexSize;
        size_t indexCnt;

        MeshInfo() {}
        MeshInfo(size_t voffset, size_t vsize,
                 size_t ioffset, size_t isize, size_t indexCnt)
            : vertexOffset(voffset), vertexSize(vsize), indexOffset(ioffset), indexSize(isize), indexCnt(indexCnt) {}
    };

    template <typename T>
    concept AllowedIndexType = std::same_as<T, unsigned short> || std::same_as<T, uint16_t> ||
                               std::same_as<T, unsigned int> || std::same_as<T, uint32_t>;

    template <typename VT>
    class BaseMesh
    {
    public:
        vke_common::AssetHandle handle;
        std::unique_ptr<DeviceBuffer> vertexBuffer;
        std::unique_ptr<DeviceBuffer> indexBuffer;
        std::vector<MeshInfo> infos;

        BaseMesh() : handle(0), vertexBuffer(nullptr), indexBuffer(nullptr) {}

        BaseMesh(const vke_common::AssetHandle hdl) : handle(hdl), vertexBuffer(nullptr), indexBuffer(nullptr) {}

        BaseMesh(const vke_common::AssetHandle hdl, const CPUBuffer &vbuffer, const CPUBuffer &ibuffer, std::vector<MeshInfo> &&infos)
            : handle(hdl),
              vertexBuffer(std::make_unique<DeviceBuffer>(vbuffer.size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
              indexBuffer(std::make_unique<DeviceBuffer>(ibuffer.size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)),
              infos(std::move(infos))
        {
            vertexBuffer->ToBuffer(0, vbuffer.data, vbuffer.size);
            indexBuffer->ToBuffer(0, ibuffer.data, ibuffer.size);
        }

        template <AllowedIndexType IT>
        BaseMesh(const vke_common::AssetHandle hdl, const std::vector<VT> &vertices, const std::vector<IT> &indices, std::vector<MeshInfo> &&infos)
            : handle(hdl),
              vertexBuffer(std::make_unique<DeviceBuffer>(vertices.size() * sizeof(VT), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
              indexBuffer(std::make_unique<DeviceBuffer>(indices.size() * sizeof(IT), VK_BUFFER_USAGE_INDEX_BUFFER_BIT)),
              infos(std::move(infos))
        {
            vertexBuffer->ToBuffer(0, vertices.data(), vertices.size() * sizeof(VT));
            indexBuffer->ToBuffer(0, indices.data(), indices.size() * sizeof(IT));
        }

        template <AllowedIndexType IT>
        BaseMesh(const vke_common::AssetHandle hdl, const std::vector<VT> &vertices, const std::vector<IT> &indices)
            : handle(hdl),
              vertexBuffer(std::make_unique<DeviceBuffer>(vertices.size() * sizeof(VT), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
              indexBuffer(std::make_unique<DeviceBuffer>(indices.size() * sizeof(IT), VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            vertexBuffer->ToBuffer(0, vertices.data(), vertices.size() * sizeof(VT));
            indexBuffer->ToBuffer(0, indices.data(), indices.size() * sizeof(IT));
            infos.emplace_back(0, vertexBuffer->bufferSize, 0, indexBuffer->bufferSize, indices.size());
        }

        BaseMesh(const vke_common::AssetHandle hdl, std::istream &binary) : handle(hdl)
        {
            uint32_t infoCnt;
            binary.read((char *)&infoCnt, sizeof(uint32_t));
            infos.resize(infoCnt);
            binary.read((char *)(infos.data()), infoCnt * sizeof(MeshInfo));

            size_t vsize;
            binary.read((char *)&vsize, sizeof(size_t));
            CPUBuffer vertices(vsize);
            binary.read((char *)(vertices.data), vsize);
            vertexBuffer = std::make_unique<DeviceBuffer>(vsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            vertexBuffer->ToBuffer(0, vertices.data, vsize);

            size_t isize;
            binary.read((char *)&isize, sizeof(size_t));
            CPUBuffer indices(isize);
            binary.read((char *)(indices.data), isize);
            indexBuffer = std::make_unique<DeviceBuffer>(isize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            indexBuffer->ToBuffer(0, indices.data, isize);
        }

        ~BaseMesh() {}

        void Render(VkCommandBuffer &commandBuffer) const
        {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(vertexBuffer->buffer), &offset);

            VkIndexType prevIndexType = VK_INDEX_TYPE_MAX_ENUM;
            for (auto &info : infos)
            {
                VkIndexType indexType = getIndexType(info);
                if (indexType != prevIndexType)
                {
                    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, 0, indexType);
                    prevIndexType = indexType;
                }

                vkCmdDrawIndexed(commandBuffer, info.indexCnt, 1,
                                 info.indexOffset / (info.indexSize / info.indexCnt),
                                 info.vertexOffset / sizeof(VT), 0);
            }
        }

        void RenderPrimitive(VkCommandBuffer &commandBuffer, uint32_t idx, VkIndexType &prevIndexType) const
        {
            if (idx == 0)
            {
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(vertexBuffer->buffer), &offset);
            }
            auto &info = infos[idx];
            VkIndexType indexType = getIndexType(info);
            if (indexType != prevIndexType)
            {
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, 0, indexType);
                prevIndexType = indexType;
            }

            vkCmdDrawIndexed(commandBuffer, info.indexCnt, 1,
                             info.indexOffset / (info.indexSize / info.indexCnt),
                             info.vertexOffset / sizeof(VT), 0);
        }

        template <AllowedIndexType IT>
        static void MeshDataToBinary(std::ostream &binary, const std::vector<MeshInfo> &infos, const std::vector<VT> &vertices, const std::vector<IT> &indices)
        {
            uint32_t infoCnt = infos.size();
            size_t vsize = vertices.size() * sizeof(VT);
            size_t isize = indices.size() * sizeof(IT);
            binary.write((const char *)&infoCnt, sizeof(uint32_t));
            binary.write((const char *)(infos.data()), infoCnt * sizeof(MeshInfo));
            binary.write((const char *)&vsize, sizeof(size_t));
            binary.write((const char *)(vertices.data()), vsize);
            binary.write((const char *)&isize, sizeof(size_t));
            binary.write((const char *)(indices.data()), isize);
        }

        static void MeshDataToBinary(std::ostream &binary, const std::vector<MeshInfo> &infos, const CPUBuffer &vertices, const CPUBuffer &indices)
        {
            uint32_t infoCnt = infos.size();
            binary.write((const char *)&infoCnt, sizeof(uint32_t));
            binary.write((const char *)(infos.data()), infoCnt * sizeof(MeshInfo));
            binary.write((const char *)&(vertices.size), sizeof(size_t));
            binary.write((const char *)(vertices.data), vertices.size);
            binary.write((const char *)&(indices.size), sizeof(size_t));
            binary.write((const char *)(indices.data), indices.size);
        }

    private:
        VkIndexType getIndexType(const MeshInfo &info) const
        {
            size_t size = info.indexSize / info.indexCnt;
            switch (size)
            {
            case 1:
                return VK_INDEX_TYPE_UINT8;
            case 2:
                return VK_INDEX_TYPE_UINT16;
            default:
                return VK_INDEX_TYPE_UINT32;
            }
        }
    };

    using Mesh = BaseMesh<Vertex>;
}

#endif