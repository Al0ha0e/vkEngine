#include <render/render.hpp>

namespace vke_render
{
    Renderer *Renderer::instance;

    void Renderer::createRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = environment->swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef1{};
        colorAttachmentRef1.attachment = 0;
        colorAttachmentRef1.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef2{};
        colorAttachmentRef2.attachment = 0;
        colorAttachmentRef2.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        std::vector<VkSubpassDescription> subpasses(2);

        subpasses[0] = VkSubpassDescription{};
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].colorAttachmentCount = 1;
        subpasses[0].pColorAttachments = &colorAttachmentRef1;

        subpasses[1] = VkSubpassDescription{};
        subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[1].colorAttachmentCount = 1;
        subpasses[1].pColorAttachments = &colorAttachmentRef2;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = subpasses.size();
        renderPassInfo.pSubpasses = subpasses.data();

        std::vector<VkSubpassDependency> dependencies(2);
        dependencies[0] = VkSubpassDependency{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        dependencies[1] = VkSubpassDependency{};
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = 1;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(environment->logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void Renderer::createFramebuffers()
    {
        frameBuffers.resize(environment->swapChainImageViews.size());
        for (size_t i = 0; i < environment->swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {environment->swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = environment->swapChainExtent.width;
            framebufferInfo.height = environment->swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(environment->logicalDevice, &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void Renderer::render()
    {
        vkWaitForFences(environment->logicalDevice, 1, &environment->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(environment->logicalDevice, 1, &environment->inFlightFences[currentFrame]);
        uint32_t imageIndex;
        vkAcquireNextImageKHR(environment->logicalDevice, environment->swapChain, UINT64_MAX, environment->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        VkCommandBuffer commandBuffer = environment->commandBuffers[currentFrame];
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
        renderPassInfo.renderArea.extent = environment->swapChainExtent;
        VkClearValue clearColor = {{{1.0f, 0.5f, 0.3f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        baseRenderer->Render(commandBuffer);
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        opaqueRenderer->Render(commandBuffer);

        vkCmdEndRenderPass(commandBuffer);
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {environment->imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &environment->commandBuffers[currentFrame];
        VkSemaphore signalSemaphores[] = {environment->renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(environment->graphicsQueue, 1, &submitInfo, environment->inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {environment->swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        vkQueuePresentKHR(environment->presentQueue, &presentInfo);
    }

    void Renderer::Update()
    {
        render();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
}