#include <render/render.hpp>
#include <render/subpass.hpp>
#include <algorithm>

namespace vke_render
{
    Renderer *Renderer::instance;

    void Renderer::cleanup()
    {
        for (auto framebuffer : frameBuffers)
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
    }

    void Renderer::recreate(RenderContext *ctx)
    {
        cleanup();
        context = *ctx;
        createFramebuffers();
    }

    void Renderer::createRenderPass(std::vector<RenderPassInfo> &passInfo)
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = context.colorFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = context.outColorLayout;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = context.depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::vector<VkSubpassDescription> subpasses(passcnt);
        for (int i = 0; i < passcnt; i++)
        {
            VkSubpassDescription &subpass = subpasses[i];
            subpass = VkSubpassDescription{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
        }

        VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = passcnt;
        renderPassInfo.pSubpasses = subpasses.data();

        std::vector<VkSubpassDependency> dependencies(passcnt);

        dependencies[0] = VkSubpassDependency{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstStageMask = passInfo[0].stageMask;   // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = passInfo[0].accessMask; // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        for (int i = 1; i < passcnt; i++)
        {
            VkSubpassDependency &dependency = dependencies[i];
            dependency = VkSubpassDependency{};
            dependency.srcSubpass = i - 1;
            dependency.dstSubpass = i;
            dependency.srcStageMask = passInfo[i - 1].stageMask;
            dependency.srcAccessMask = passInfo[i - 1].accessMask;
            dependency.dstStageMask = passInfo[i].stageMask;   // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = passInfo[i].accessMask; // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }

        renderPassInfo.dependencyCount = passcnt;
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void Renderer::createFramebuffers()
    {
        frameBuffers.resize(context.imageCnt);
        for (size_t i = 0; i < context.imageCnt; i++)
        {
            std::vector<VkImageView> &attachments = (*(context.imageViews))[i];

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = context.width;
            framebufferInfo.height = context.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void Renderer::render()
    {
        uint32_t imageIndex = context.AcquireNextImage(currentFrame);

        VkCommandBuffer commandBuffer = (*context.commandBuffers)[currentFrame];
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = VkExtent2D{context.width, context.height};
        VkClearValue clearValues[2];
        clearValues[0].color = {{1.0f, 0.5f, 0.3f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        subPasses[0]->Render(commandBuffer);
        for (int i = 1; i < subPasses.size(); i++)
        {
            vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
            subPasses[i]->Render(commandBuffer);
        }

        vkCmdEndRenderPass(commandBuffer);
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }

        std::vector<VkSemaphore> presentWaitSemaphores;
        std::vector<VkPipelineStageFlags> presentWaitStages;
        context.Present(currentFrame, imageIndex, presentWaitSemaphores, presentWaitStages);
    }

    void Renderer::Update()
    {
        render();
        currentFrame = (currentFrame + 1) % context.imageCnt;
    }
}