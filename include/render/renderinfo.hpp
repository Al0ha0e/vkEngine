#ifndef RENDER_INFO_H
#define RENDER_INFO_H

#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/pipeline.hpp>
#include <ds/id_allocator.hpp>

namespace vke_render
{
    struct RenderUnit
    {
        std::shared_ptr<const Mesh> mesh;
        std::vector<PushConstantInfo> pushConstantInfos;
        uint32_t perPrimitiveStart;
        VkDescriptorSet perUnitDescriptorSet;
        const glm::mat4 *modelMatrix;

        RenderUnit() : perPrimitiveStart(0), perUnitDescriptorSet(nullptr), modelMatrix(nullptr) {}

        RenderUnit(std::shared_ptr<const Mesh> &msh, const void *pValues, uint32_t constantSize, VkDescriptorSet descriptorSet = nullptr, bool constantIsFloat = true)
            : mesh(msh), pushConstantInfos(1, PushConstantInfo(constantSize, pValues, constantIsFloat)),
              perPrimitiveStart(1), perUnitDescriptorSet(descriptorSet),
              modelMatrix(constantSize == sizeof(glm::mat4) ? static_cast<const glm::mat4 *>(pValues) : nullptr) {}

        RenderUnit(std::shared_ptr<const Mesh> &msh, std::vector<PushConstantInfo> &&cInfos, uint32_t perPrimitiveStart, VkDescriptorSet descriptorSet = nullptr)
            : mesh(msh), pushConstantInfos(std::move(cInfos)), perPrimitiveStart(std::min(perPrimitiveStart, (uint32_t)pushConstantInfos.size())),
              perUnitDescriptorSet(descriptorSet), modelMatrix(nullptr)
        {
            if (!pushConstantInfos.empty() && pushConstantInfos[0].offset == 0 && pushConstantInfos[0].size == sizeof(glm::mat4))
                modelMatrix = static_cast<const glm::mat4 *>(pushConstantInfos[0].pValues);
        }

        void Render(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, int descriptorSetOffset)
        {
            if (mesh->infos.size() == 0)
                return;

            if (perUnitDescriptorSet != nullptr)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    descriptorSetOffset,
                    1,
                    &perUnitDescriptorSet,
                    0, nullptr);

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
        std::map<vke_ds::id64_t, RenderUnit *> units;

        RenderInfo(std::shared_ptr<Material> &mat)
            : material(mat),
              commonDescriptorSet(nullptr)
        {
            if (material->textures.size() > 0)
            {
                commonDescriptorSet = material->shader->CreateDescriptorSet(1);
                material->UpdateDescriptorSet(commonDescriptorSet);
            }
        }

        ~RenderInfo() {}

        void CreatePipeline(const std::vector<uint32_t> &vertexAttributeSizes,
                            VkVertexInputRate vertexInputRate,
                            VkGraphicsPipelineCreateInfo &pipelineInfo)
        {
            renderPipeline = std::make_unique<GraphicsPipeline>(
                material->shader, vertexAttributeSizes, vertexInputRate, pipelineInfo);
        }

        vke_ds::id64_t AddUnit(RenderUnit *unit)
        {
            vke_ds::id64_t id = allocator.Alloc();
            units[id] = unit;
            return id;
        }

        RenderUnit *GetUnit(vke_ds::id64_t id)
        {
            const auto &kv = units.find(id);
            if (kv == units.end())
                return nullptr;
            return kv->second;
        }

        void RemoveUnit(vke_ds::id64_t id)
        {
            units.erase(id);
        }

        void Render(VkCommandBuffer &commandBuffer, VkDescriptorSet globalDescriptorSet)
        {
            int setcnt = 0;
            if (globalDescriptorSet != nullptr)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderPipeline->pipelineLayout,
                    setcnt++,
                    1,
                    &globalDescriptorSet,
                    0, nullptr);

            if (commonDescriptorSet != nullptr)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderPipeline->pipelineLayout,
                    setcnt++,
                    1,
                    &commonDescriptorSet,
                    0, nullptr);

            material->SetPushConstants(commandBuffer, renderPipeline->pipelineLayout);
            for (auto &unit : units)
                unit.second->Render(commandBuffer, renderPipeline->pipelineLayout, setcnt);
        }

    private:
        vke_ds::NaiveIDAllocator<vke_ds::id64_t> allocator;
    };
}

#endif
