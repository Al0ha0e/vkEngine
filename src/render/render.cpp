#include <render/render.hpp>
#include <render/subpass.hpp>
#include <algorithm>

namespace vke_render
{
    Renderer *Renderer::instance;

    void Renderer::initFrameGraph(std::map<std::string, vke_ds::id32_t> &blackboard,
                                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
    {
        instance->frameGraph = std::make_unique<FrameGraph>(MAX_FRAMES_IN_FLIGHT);
        VkDescriptorImageInfo info{};
        info.sampler = nullptr;
        info.imageView = (*context.colorImageViews)[0];
        colorAttachmentResourceID = frameGraph->AddPermanentImageResource((*context.colorImages)[0], VK_IMAGE_ASPECT_COLOR_BIT, info,
                                                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        info.imageView = context.depthImageView;
        vke_ds::id32_t depthAttachmentResourceID = frameGraph->AddPermanentImageResource(context.depthImage, VK_IMAGE_ASPECT_DEPTH_BIT, info,
                                                                                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, std::nullopt, std::nullopt);
        VkDescriptorBufferInfo binfo{};
        binfo.buffer = camInfoBuffer.buffer;
        binfo.offset = 0;
        binfo.range = camInfoBuffer.bufferSize;
        vke_ds::id32_t cameraResourceID = frameGraph->AddPermanentBufferResource(binfo, VK_PIPELINE_STAGE_TRANSFER_BIT);

        std::cout << "resource ids " << colorAttachmentResourceID << " " << depthAttachmentResourceID << " " << cameraResourceID << "\n";

        frameGraph->AddTargetResource(colorAttachmentResourceID);
        frameGraph->AddTargetResource(depthAttachmentResourceID);

        blackboard["colorAttachment"] = colorAttachmentResourceID;
        blackboard["depthAttachment"] = depthAttachmentResourceID;
        blackboard["cameraInfo"] = cameraResourceID;

        vke_ds::id32_t oriColorResourceNodeID = frameGraph->AllocResourceNode(false, colorAttachmentResourceID);
        vke_ds::id32_t oriDepthResourceNodeID = frameGraph->AllocResourceNode(false, depthAttachmentResourceID);
        cameraResourceNodeID = frameGraph->AllocResourceNode(false, cameraResourceID);

        currentResourceNodeID[colorAttachmentResourceID] = oriColorResourceNodeID;
        currentResourceNodeID[depthAttachmentResourceID] = oriDepthResourceNodeID;
        currentResourceNodeID[cameraResourceID] = cameraResourceNodeID;

        auto copyCallback = [this](TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
        {
            camInfoBuffer.ToBufferAsync(commandBuffer, 0, camInfoBuffer.bufferSize);
        };

        cameraUpdateTaskID = frameGraph->AllocTaskNode(TRANSFER_TASK, copyCallback);

        // copy
        frameGraph->AddTaskNodeResourceRef(cameraUpdateTaskID, false, 0, cameraResourceNodeID,
                                           VK_ACCESS_TRANSFER_WRITE_BIT,
                                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
    }

    void Renderer::cleanup() {}

    void Renderer::recreate(RenderContext *ctx)
    {
        cleanup();
        context = *ctx;
    }

    void Renderer::render()
    {
        uint32_t imageIndex = context.AcquireNextImage(currentFrame);
        ((ImageResource *)frameGraph->permanentResources[colorAttachmentResourceID].get())->image = (*context.colorImages)[imageIndex];

        frameGraph->Execute(currentFrame, imageIndex);
        context.Present(currentFrame, imageIndex);
    }

    void Renderer::Update()
    {
        if (cameraInfoUpdated)
        {
            cameraInfoUpdated = false;
            std::cout << "Cam updated\n";
            frameGraph->resourceNodes[cameraResourceNodeID]->srcTaskID = cameraUpdateTaskID;
            frameGraph->Compile();

            frameGraph->resourceNodes[cameraResourceNodeID]->srcTaskID = 0;
            frameGraphUpdated = true;
        }
        else if (frameGraphUpdated)
        {
            frameGraph->Compile();
            frameGraphUpdated = false;
        }

        render();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
}