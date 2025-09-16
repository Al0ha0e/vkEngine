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

    struct RenderUnit
    {
        std::shared_ptr<const Mesh> mesh;
        void *pushConstants;
        uint32_t constantSize;

        RenderUnit() = default;

        RenderUnit(std::shared_ptr<const Mesh> &msh, void *pushConstants, uint32_t constantSize)
            : mesh(msh), pushConstants(pushConstants), constantSize(constantSize) {}

        void Render(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, int descriptorSetOffset)
        {
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, constantSize, pushConstants);
            mesh->Render(commandBuffer);
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