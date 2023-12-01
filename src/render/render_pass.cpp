#include <render/render_pass.hpp>

namespace vke_render
{
    RenderPasses *RenderPasses::instance;

    void RenderPasses::createRenderPass(std::vector<RenderPassInfo> &passInfo)
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

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        std::vector<VkSubpassDescription> subpasses(passcnt);
        for (int i = 0; i < passcnt; i++)
        {
            VkSubpassDescription &subpass = subpasses[i];
            subpass = VkSubpassDescription{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
        }

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = passcnt;
        renderPassInfo.pSubpasses = subpasses.data();

        std::vector<VkSubpassDependency> dependencies(passcnt);

        dependencies[0] = VkSubpassDependency{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstStageMask = passInfo[0].stageMask;   // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = passInfo[0].accessMask; // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        for (int i = 1; i < passcnt; i++)
        {
            VkSubpassDependency &dependency = dependencies[i];
            dependency = VkSubpassDependency{};
            dependencies[0].srcSubpass = i - 1;
            dependencies[0].dstSubpass = i;
            dependencies[0].srcStageMask = passInfo[i - 1].stageMask;
            dependencies[0].srcAccessMask = passInfo[i - 1].accessMask;
            dependencies[0].dstStageMask = passInfo[i].stageMask;   // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].dstAccessMask = passInfo[i].accessMask; // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }

        renderPassInfo.dependencyCount = passcnt;
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(environment->logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }
}