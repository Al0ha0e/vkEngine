#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <render/material.hpp>
#include <set>

namespace vke_render
{
    class RenderPipeline
    {
    public:
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;

        RenderPipeline() {}
        RenderPipeline(VkPipelineLayout pipelineLayout, VkPipeline pipeline) : pipelineLayout(pipelineLayout), pipeline(pipeline) {}
        RenderPipeline(const VertFragShader &shader,
                       std::vector<uint32_t> &vertexAttributeSizes,
                       VkVertexInputRate vertexInputRate,
                       VkGraphicsPipelineCreateInfo &pipelineInfo)
        {
            shader.CreatePipeline(vertexAttributeSizes, vertexInputRate, pipelineLayout, pipelineInfo, pipeline);
        }

        RenderPipeline(const RenderPipeline &) = delete;
        RenderPipeline &operator=(const RenderPipeline &) = delete;

        RenderPipeline(RenderPipeline &&other) noexcept : pipelineLayout(other.pipelineLayout), pipeline(other.pipeline)
        {
            other.pipelineLayout = nullptr;
            other.pipeline = nullptr;
        }
        RenderPipeline &operator=(RenderPipeline &&other) noexcept
        {
            if (this != &other)
            {
                VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
                if (pipelineLayout != nullptr)
                {
                    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
                    vkDestroyPipeline(logicalDevice, pipeline, nullptr);
                }
                pipelineLayout = other.pipelineLayout;
                pipeline = other.pipeline;
                other.pipelineLayout = nullptr;
                other.pipeline = nullptr;
            }
            return *this;
        }

        ~RenderPipeline()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            if (pipelineLayout != nullptr)
            {
                vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
                vkDestroyPipeline(logicalDevice, pipeline, nullptr);
            }
        }

        void Bind(VkCommandBuffer commandBuffer)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        }
    };

    class RenderInfo2
    {
    public:
        std::shared_ptr<Material> material;
        VkDescriptorSet descriptorSet;
        uint32_t setID;
        std::function<void(const RenderInfo2 &, VkCommandBuffer)> render;

        RenderInfo2() : descriptorSet(nullptr), render(nullptr) {}
        RenderInfo2(std::shared_ptr<Material> &material, std::function<void(const RenderInfo2 &, VkCommandBuffer)> render)
            : material(material), descriptorSet(nullptr), setID(0), render(render) {}

        RenderInfo2(const RenderInfo2 &) = delete;
        RenderInfo2 &operator=(const RenderInfo2 &) = delete;

        RenderInfo2(RenderInfo2 &&other) noexcept : material(other.material), descriptorSet(other.descriptorSet), setID(other.setID), render(other.render)
        {
            other.descriptorSet = nullptr;
            other.render = nullptr;
        }

        RenderInfo2 &operator=(RenderInfo2 &&other) noexcept
        {
            if (this != &other)
            { // TODO release descriptor set
                material = other.material;
                descriptorSet = other.descriptorSet;
                setID = other.setID;
                render = other.render;
                other.descriptorSet = nullptr;
                other.render = nullptr;
            }
            return *this;
        }
        ~RenderInfo2() {} // TODO release descriptor set

        void Render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
        {
            if (descriptorSet)
            {
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    setID,
                    1,
                    &descriptorSet,
                    0, nullptr);
            }
            render(*this, commandBuffer);
        }
    };

    class RenderPass
    {
    public:
        uint32_t passID;
        std::function<void(RenderPass &, RenderInfo2 &)> initBindingInfo;

        RenderPass() {}
        RenderPass(const RenderPass &) = delete;
        RenderPass &operator=(const RenderPass &) = delete;

        void RegisterMaterial(std::shared_ptr<Material> &material, std::function<void(const RenderInfo2 &, VkCommandBuffer &)> renderCallback)
        {
            std::shared_ptr<VertFragShader> &shader = material->shader;
            auto it = pipelineToRenderInfoMap.find(shader.get());
            if (it == pipelineToRenderInfoMap.end())
            {
                pipelineMap[shader.get()] = std::make_unique<RenderPipeline>(
                    *shader, vertexAttributeSizes, vertexInputRate, pipelineInfo);
                pipelineToRenderInfoMap[shader.get()] = std::map<Material *, std::shared_ptr<RenderInfo2>>();
                it = pipelineToRenderInfoMap.find(shader.get());
            }

            auto &renderInfoMap = it->second;
            if (renderInfoMap.find(material.get()) == renderInfoMap.end())
            {
                auto renderInfo = std::make_shared<RenderInfo2>(material, renderCallback);
                initBindingInfo(*this, *renderInfo);
                renderInfoMap[material.get()] = renderInfo;
            }
        }

        void Render(VkCommandBuffer commandBuffer)
        {
            VkRenderingAttachmentInfo colorAttachmentInfo{};
            colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorAttachmentInfo.pNext = nullptr;
            colorAttachmentInfo.imageView = (*context->colorImageViews)[currentFrame];
            colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;

            VkRenderingAttachmentInfo depthAttachmentInfo{};
            depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachmentInfo.pNext = nullptr;
            depthAttachmentInfo.imageView = (*context->depthImageViews)[currentFrame];
            depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;

            VkRenderingInfo renderingInfo{};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            renderingInfo.pNext = nullptr;
            renderingInfo.renderArea = {{0, 0}, {context->width, context->height}};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachmentInfo;
            renderingInfo.pDepthAttachment = &depthAttachmentInfo;

            vkCmdBeginRendering(commandBuffer, &renderingInfo);
            ////////////////////////////////////////////////////////////////////////////////
            for (auto &kv : pipelineToRenderInfoMap)
            {
                auto &pipeline = pipelineMap[kv.first];
                pipeline->Bind(commandBuffer);
                for (auto &renderInfo : kv.second)
                    renderInfo.second->Render(commandBuffer, pipeline->pipelineLayout);
            }
            vkCmdEndRendering(commandBuffer);
        }

    private:
        std::vector<uint32_t> vertexAttributeSizes;
        VkVertexInputRate vertexInputRate;
        VkGraphicsPipelineCreateInfo pipelineInfo;

        std::map<VertFragShader *, std::unique_ptr<RenderPipeline>> pipelineMap;
        std::map<VertFragShader *, std::map<Material *, std::shared_ptr<RenderInfo2>>> pipelineToRenderInfoMap;
    };
}

#endif