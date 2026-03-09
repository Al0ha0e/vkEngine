#ifndef MESH_H
#define MESH_H

#include <render/buffer.hpp>
#include <concepts>
#include <iostream>
#include <span>
#include <vector>
#include <ozz/base/maths/simd_math.h>

namespace vke_render
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec4 tangent;
        glm::vec2 texCoord;

        Vertex() = default;
        Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec4 tangent, glm::vec2 texCoord)
            : pos(pos), normal(normal), tangent(tangent), texCoord(texCoord) {}
    };

    struct SkinVertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec4 tangent;
        glm::vec2 texCoord;
        glm::vec4 weights;
        glm::uvec4 jointIDs;
    };

    struct MeshInfo
    {
        uint64_t vertexOffset;
        uint64_t vertexSize;
        uint64_t vertexCnt;
        uint64_t indexOffset;
        uint64_t indexSize;
        uint64_t indexCnt;

        MeshInfo() = default;
        MeshInfo(uint64_t voffset, uint64_t vsize, uint64_t vcnt,
                 uint64_t ioffset, uint64_t isize, uint64_t icnt)
            : vertexOffset(voffset), vertexSize(vsize), vertexCnt(vcnt), indexOffset(ioffset), indexSize(isize), indexCnt(icnt) {}

        uint64_t getVertexUnitSize() const { return vertexSize / vertexCnt; }
        uint64_t getIndexUnitSize() const { return indexSize / indexCnt; }
    };

    template <typename T>
    concept AllowedIndexType = std::same_as<T, uint16_t> || std::same_as<T, uint32_t>;

    class Mesh
    {
    public:
        vke_common::AssetHandle handle;
        std::unique_ptr<DeviceBuffer> vertexBuffer;
        std::unique_ptr<DeviceBuffer> indexBuffer;
        std::vector<ozz::math::Float4x4> invBindMatrices;
        std::vector<int> joints;
        std::vector<MeshInfo> infos;

        Mesh() : handle(0), vertexBuffer(nullptr), indexBuffer(nullptr) {}

        Mesh(const vke_common::AssetHandle hdl) : handle(hdl), vertexBuffer(nullptr), indexBuffer(nullptr) {}

        Mesh(const vke_common::AssetHandle hdl, const CPUBuffer<> &vbuffer, const CPUBuffer<> &ibuffer, std::vector<MeshInfo> &&infos)
            : handle(hdl),
              vertexBuffer(std::make_unique<DeviceBuffer>(vbuffer.size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
              indexBuffer(std::make_unique<DeviceBuffer>(ibuffer.size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)),
              infos(std::move(infos))
        {
            vertexBuffer->ToBuffer(0, vbuffer.data, vbuffer.size);
            indexBuffer->ToBuffer(0, ibuffer.data, ibuffer.size);
        }

        template <typename VT, AllowedIndexType IT>
        Mesh(const vke_common::AssetHandle hdl, const std::span<const VT> vertices, const std::span<const IT> indices, std::vector<MeshInfo> &&infos)
            : handle(hdl),
              vertexBuffer(std::make_unique<DeviceBuffer>(vertices.size_bytes(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
              indexBuffer(std::make_unique<DeviceBuffer>(indices.size_bytes(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT)),
              infos(std::move(infos))
        {
            vertexBuffer->ToBuffer(0, vertices.data(), vertices.size_bytes());
            indexBuffer->ToBuffer(0, indices.data(), indices.size_bytes());
        }

        template <typename VT, AllowedIndexType IT>
        Mesh(const vke_common::AssetHandle hdl, const std::span<const VT> vertices, const std::span<const IT> indices)
            : handle(hdl),
              vertexBuffer(std::make_unique<DeviceBuffer>(vertices.size_bytes(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
              indexBuffer(std::make_unique<DeviceBuffer>(indices.size_bytes(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            vertexBuffer->ToBuffer(0, vertices.data(), vertices.size_bytes());
            indexBuffer->ToBuffer(0, indices.data(), indices.size_bytes());
            infos.emplace_back(0, vertexBuffer->bufferSize, vertices.size(), 0, indexBuffer->bufferSize, indices.size());
        }

        Mesh(const vke_common::AssetHandle hdl, std::istream &binary) : handle(hdl)
        {
            uint32_t infoCnt;
            binary.read((char *)&infoCnt, sizeof(uint32_t));
            infos.resize(infoCnt);
            binary.read((char *)(infos.data()), infoCnt * sizeof(MeshInfo));
            uint64_t vsize;
            binary.read((char *)&vsize, sizeof(uint64_t));
            CPUBuffer<> vertices(vsize);
            binary.read((char *)(vertices.data), vsize);
            vertexBuffer = std::make_unique<DeviceBuffer>(vsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            vertexBuffer->ToBuffer(0, vertices.data, vsize);
            uint64_t isize;
            binary.read((char *)&isize, sizeof(uint64_t));
            CPUBuffer<> indices(isize);
            binary.read((char *)(indices.data), isize);
            indexBuffer = std::make_unique<DeviceBuffer>(isize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            indexBuffer->ToBuffer(0, indices.data, isize);

            uint64_t ibmSize;
            if (binary.read((char *)(&ibmSize), sizeof(uint64_t)))
            {
                uint32_t matCnt = ibmSize >> 6;
                invBindMatrices.resize(matCnt);
                CPUBuffer<16> ibmBuffer(ibmSize);
                binary.read((char *)(ibmBuffer.data), ibmSize);
                VKE_LOG_INFO("matCnt {}", matCnt)
                for (int i = 0; i < matCnt; ++i)
                    for (int j = 0; j < 4; ++j)
                        invBindMatrices[i].cols[j] = ozz::math::simd_float4::LoadPtr(((float *)ibmBuffer.data) + (i << 4) + (j << 2));

                uint32_t jointCnt;
                binary.read((char *)(&jointCnt), sizeof(uint32_t));
                joints.resize(jointCnt);
                binary.read((char *)(joints.data()), jointCnt << 2);
            }
        }

        ~Mesh() {}

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
                                 info.indexOffset / info.getIndexUnitSize(),
                                 info.vertexOffset / info.getVertexUnitSize(), 0);
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
                             info.indexOffset / info.getIndexUnitSize(),
                             info.vertexOffset / info.getVertexUnitSize(), 0);
        }

        template <typename VT, AllowedIndexType IT>
        static void MeshDataToBinary(std::ostream &binary, const std::vector<MeshInfo> &infos,
                                     const std::span<const VT> &vertices, const std::span<const IT> &indices)
        {
            uint32_t infoCnt = infos.size();
            uint64_t vsize = vertices.size_bytes();
            uint64_t isize = indices.size_bytes();
            binary.write((const char *)&infoCnt, sizeof(uint32_t));
            binary.write((const char *)(infos.data()), infoCnt * sizeof(MeshInfo));
            binary.write((const char *)&vsize, sizeof(uint64_t));
            binary.write((const char *)(vertices.data()), vsize);
            binary.write((const char *)&isize, sizeof(uint64_t));
            binary.write((const char *)(indices.data()), isize);
        }

        static void MeshDataToBinary(std::ostream &binary, const std::vector<MeshInfo> &infos,
                                     const CPUBuffer<> &vertices, const CPUBuffer<> &indices)
        {
            uint32_t infoCnt = infos.size();
            binary.write((const char *)&infoCnt, sizeof(uint32_t));
            binary.write((const char *)(infos.data()), infoCnt * sizeof(MeshInfo));
            vertices.WriteToBinaryStream(binary);
            indices.WriteToBinaryStream(binary);
        }

        static void MeshDataToBinary(std::ostream &binary, const std::vector<MeshInfo> &infos,
                                     const CPUBuffer<> &vertices, const CPUBuffer<> &indices,
                                     const CPUBuffer<> &invBindMatrices, const std::vector<int> &joints)
        {
            MeshDataToBinary(binary, infos, vertices, indices);
            invBindMatrices.WriteToBinaryStream(binary);
            uint32_t jointCnt = joints.size();
            binary.write((const char *)&jointCnt, sizeof(uint32_t));
            binary.write((const char *)(joints.data()), jointCnt * sizeof(int));
        }

    private:
        VkIndexType getIndexType(const MeshInfo &info) const
        {
            if (info.getIndexUnitSize() == 2)
                return VK_INDEX_TYPE_UINT16;
            return VK_INDEX_TYPE_UINT32;
        }
    };
}

#endif