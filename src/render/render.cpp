#include <render/render.hpp>
#include <render/subpass.hpp>
#include <algorithm>

namespace vke_render
{
    Renderer *Renderer::instance;

    void Renderer::cleanup() {}

    void Renderer::recreate(RenderContext *ctx)
    {
        cleanup();
        context = *ctx;
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

        RenderEnvironment::MakeLayoutTransition(commandBuffer,
                                                VK_ACCESS_NONE,
                                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                VK_IMAGE_LAYOUT_UNDEFINED,
                                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                (*context.colorImages)[currentFrame],
                                                VK_IMAGE_ASPECT_COLOR_BIT,
                                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        RenderEnvironment::MakeLayoutTransition(commandBuffer,
                                                VK_ACCESS_NONE,
                                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                                VK_IMAGE_LAYOUT_UNDEFINED,
                                                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                                (*context.depthImages)[currentFrame],
                                                VK_IMAGE_ASPECT_DEPTH_BIT,
                                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

        for (int i = 0; i < subPasses.size(); i++)
            subPasses[i]->Render(commandBuffer, currentFrame);

        RenderEnvironment::MakeLayoutTransition(commandBuffer,
                                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                VK_ACCESS_NONE,
                                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                (*context.colorImages)[currentFrame],
                                                VK_IMAGE_ASPECT_COLOR_BIT,
                                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

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