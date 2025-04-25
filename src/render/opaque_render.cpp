#include <render/render.hpp>
#include <render/opaque_render.hpp>

namespace vke_render
{
    void OpaqueRenderer::createGlobalDescriptorSet()
    {
        VkDescriptorSetLayoutBinding vpLayoutBinding{};
        vpLayoutBinding.binding = 0;
        vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        vpLayoutBinding.descriptorCount = 1;
        vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vpLayoutBinding.pImmutableSamplers = nullptr; // Optional

        globalDescriptorInfos.push_back(DescriptorInfo(vpLayoutBinding, sizeof(CameraInfo)));

        globalDescriptorSetInfo.uniformDescriptorCnt = 0;
        std::vector<VkDescriptorSetLayoutBinding> globalBindings;
        for (auto &dInfo : globalDescriptorInfos)
        {
            globalDescriptorSetInfo.AddCnt(dInfo.bindingInfo);
            globalBindings.push_back(dInfo.bindingInfo);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = globalBindings.size();
        layoutInfo.pBindings = globalBindings.data();

        if (vkCreateDescriptorSetLayout(environment->logicalDevice,
                                        &layoutInfo,
                                        nullptr,
                                        &(globalDescriptorSetInfo.layout)) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        globalDescriptorSet = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(globalDescriptorSetInfo);
    }

    void OpaqueRenderer::createGraphicsPipeline(RenderInfo &renderInfo)
    {
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.pNext = nullptr;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &(context->colorFormat);
        pipelineRenderingCreateInfo.depthAttachmentFormat = context->depthFormat;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {};  // Optional

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.pNext = &pipelineRenderingCreateInfo;
        pipelineInfo.pDepthStencilState = &depthStencil;

        renderInfo.CreatePipeline(pipelineInfo);
    }

    void OpaqueRenderer::Render(VkCommandBuffer commandBuffer, uint32_t currentFrame)
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

        for (auto &kv : renderInfoMap)
        {
            std::unique_ptr<RenderInfo> &renderInfo = kv.second;
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderInfo->pipeline);
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(context->width);
            viewport.height = static_cast<float>(context->height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = {context->width, context->height};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            renderInfo->Render(commandBuffer, &globalDescriptorSet);
        }

        vkCmdEndRendering(commandBuffer);
    }
}