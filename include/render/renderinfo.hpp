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
        const void *pValues;

        PushConstantInfo() : offset(0), size(0), pValues(nullptr) {}
        PushConstantInfo(const uint32_t size, const void *pValues, const uint32_t offset = 0) : size(size), offset(offset), pValues(pValues) {}
    };

    struct RenderUnit
    {
        std::shared_ptr<const Mesh> mesh;
        std::vector<PushConstantInfo> pushConstantInfos;
        uint32_t perPrimitiveStart;
        VkDescriptorSet perUnitDescriptorSet;

        RenderUnit() = default;

        RenderUnit(std::shared_ptr<const Mesh> &msh, const void *pValues, uint32_t constantSize, VkDescriptorSet descriptorSet = nullptr)
            : mesh(msh), pushConstantInfos(1, PushConstantInfo(constantSize, pValues)), perPrimitiveStart(1), perUnitDescriptorSet(descriptorSet) {}

        RenderUnit(std::shared_ptr<const Mesh> &msh, std::vector<PushConstantInfo> &&cInfos, uint32_t perPrimitiveStart, VkDescriptorSet descriptorSet = nullptr)
            : mesh(msh), pushConstantInfos(std::move(cInfos)), perPrimitiveStart(std::min(perPrimitiveStart, (uint32_t)pushConstantInfos.size())), perUnitDescriptorSet(descriptorSet) {}

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
            renderPipeline = std::make_unique<GraphicsPipeline>(material->shader, vertexAttributeSizes, VK_VERTEX_INPUT_RATE_VERTEX, pipelineInfo);
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
            for (auto &unit : units)
                unit.second->Render(commandBuffer, renderPipeline->pipelineLayout, setcnt);
        }

    private:
        vke_ds::NaiveIDAllocator<vke_ds::id64_t> allocator;
    };
}

#endif