#ifndef RENDER_INFO_H
#define RENDER_INFO_H

#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/pipeline.hpp>
#include <ds/id_allocator.hpp>

namespace vke_render
{
    struct CameraInfo
    {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 viewPos;

        CameraInfo() = default;

        CameraInfo(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos)
            : view(view), projection(projection), viewPos(viewPos, 1.0) {}
    };

    struct PushConstantInfo
    {
        uint32_t size;
        uint32_t offset;
        void *pValues;

        PushConstantInfo() : offset(0), size(0), pValues(nullptr) {}
        PushConstantInfo(uint32_t size, void *pValues, uint32_t offset = 0) : size(size), offset(offset), pValues(pValues) {}
    };

    struct RenderUnit
    {
        std::shared_ptr<const Mesh> mesh;
        std::vector<PushConstantInfo> pushConstantInfos;
        uint32_t perPrimitiveStart;

        RenderUnit() = default;

        RenderUnit(std::shared_ptr<const Mesh> &msh, void *pValues, uint32_t constantSize)
            : mesh(msh), pushConstantInfos(1, PushConstantInfo(constantSize, pValues)), perPrimitiveStart(1) {}

        RenderUnit(std::shared_ptr<const Mesh> &msh, std::vector<PushConstantInfo> &&cInfos, uint32_t perPrimitiveStart)
            : mesh(msh), pushConstantInfos(std::move(cInfos)), perPrimitiveStart(std::min(perPrimitiveStart, (uint32_t)pushConstantInfos.size())) {}

        void Render(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, int descriptorSetOffset)
        {
            if (mesh->infos.size() == 0)
                return;

            for (int i = 0; i < perPrimitiveStart; i++)
            {
                auto &info = pushConstantInfos[i];
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, info.offset, info.size, info.pValues);
            }

            if (mesh->infos.size() == 1)
            {
                for (int i = perPrimitiveStart; i < pushConstantInfos.size(); i++)
                {
                    auto &info = pushConstantInfos[i];
                    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, info.offset, info.size, info.pValues);
                }
                mesh->Render(commandBuffer);
                return;
            }

            VkIndexType prevIndexType = VK_INDEX_TYPE_MAX_ENUM;
            size_t innerOffset = 0;
            for (int i = 0; i < mesh->infos.size(); i++)
            {
                for (int j = perPrimitiveStart; j < pushConstantInfos.size(); j++)
                {
                    auto &info = pushConstantInfos[j];
                    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, info.offset, info.size,
                                       ((char *)info.pValues) + i * info.size);
                }
                mesh->RenderPrimitive(commandBuffer, i, prevIndexType);
            }
        }
    };

    class RenderInfo
    {
    public:
        std::shared_ptr<Material> material;
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        VkDescriptorSet commonDescriptorSet;
        bool hasCommonDescriptorSet;
        std::map<vke_ds::id64_t, RenderUnit> units;

        RenderInfo(std::shared_ptr<Material> &mat)
            : material(mat),
              commonDescriptorSet(nullptr)
        {
            hasCommonDescriptorSet = material->textures.size() > 0;
            if (hasCommonDescriptorSet)
            {
                commonDescriptorSet = material->shader->CreateDescriptorSet(1);
                material->UpdateDescriptorSet(commonDescriptorSet);
            }
        }

        ~RenderInfo() {}

        void CreatePipeline(std::vector<uint32_t> &vertexAttributeSizes,
                            VkVertexInputRate vertexInputRate,
                            VkGraphicsPipelineCreateInfo &pipelineInfo)
        {
            renderPipeline = std::make_unique<GraphicsPipeline>(material->shader, vertexAttributeSizes, VK_VERTEX_INPUT_RATE_VERTEX, pipelineInfo);
        }

        vke_ds::id64_t AddUnit(std::shared_ptr<const Mesh> mesh, void *pushConstants, uint32_t constantSize)
        {
            vke_ds::id64_t id = allocator.Alloc();
            units[id] = RenderUnit(mesh, pushConstants, constantSize);
            return id;
        }

        vke_ds::id64_t AddUnit(std::shared_ptr<const Mesh> mesh, std::vector<PushConstantInfo> &&pushConstantInfos, uint32_t perPrimitiveStart)
        {
            vke_ds::id64_t id = allocator.Alloc();
            units[id] = RenderUnit(mesh, std::move(pushConstantInfos), perPrimitiveStart);
            return id;
        }

        void RemoveUnit(vke_ds::id64_t id)
        {
            units.erase(id);
        }

        void Render(VkCommandBuffer &commandBuffer, VkDescriptorSet globalDescriptorSet)
        {
            int setcnt = 0;
            if (globalDescriptorSet)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderPipeline->pipelineLayout,
                    setcnt++,
                    1,
                    &globalDescriptorSet,
                    0, nullptr);

            if (hasCommonDescriptorSet)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderPipeline->pipelineLayout,
                    setcnt++,
                    1,
                    &commonDescriptorSet,
                    0, nullptr);
            for (auto &unit : units)
                unit.second.Render(commandBuffer, renderPipeline->pipelineLayout, setcnt);
        }

    private:
        vke_ds::NaiveIDAllocator<vke_ds::id64_t> allocator;
    };
}

#endif