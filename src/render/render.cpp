#include <render/render.hpp>
#include <render/subpass.hpp>
#include <algorithm>

namespace vke_render
{
    Renderer *Renderer::instance;

    void Renderer::initFrameGraph()
    {
        frameGraph = std::make_unique<FrameGraph>(MAX_FRAMES_IN_FLIGHT);
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

        cameraResourceNodeID = frameGraph->AllocResourceNode(false, cameraResourceID);

        vke_ds::id32_t baseInColorResourceNodeID = frameGraph->AllocResourceNode(false, colorAttachmentResourceID);
        vke_ds::id32_t baseOutColorResourceNodeID = frameGraph->AllocResourceNode(false, colorAttachmentResourceID);

        vke_ds::id32_t opaqueOutColorResourceNodeID = frameGraph->AllocResourceNode(false, colorAttachmentResourceID);
        vke_ds::id32_t opaqueInDepthResourceNodeID = frameGraph->AllocResourceNode(false, depthAttachmentResourceID);
        vke_ds::id32_t opaqueOutDepthResourceNodeID = frameGraph->AllocResourceNode(false, depthAttachmentResourceID);

        auto copyCallback = [this](TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
        {
            camInfoBuffer.ToBufferAsync(commandBuffer, 0, camInfoBuffer.bufferSize);
        };

        cameraUpdateTaskID = frameGraph->AllocTaskNode(TRANSFER_TASK, copyCallback);

        vke_ds::id32_t baseTaskNodeID = frameGraph->AllocTaskNode(RENDER_TASK,
                                                                  std::bind(&BaseRenderer::Render, static_cast<BaseRenderer *>(subPasses[subPassMap[BASE_RENDERER]].get()),
                                                                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        vke_ds::id32_t opaqueTaskNodeID = frameGraph->AllocTaskNode(RENDER_TASK,
                                                                    std::bind(&OpaqueRenderer::Render, static_cast<OpaqueRenderer *>(subPasses[subPassMap[OPAQUE_RENDERER]].get()),
                                                                              std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

        // copy
        frameGraph->AddTaskNodeResourceRef(cameraUpdateTaskID, false, 0, cameraResourceNodeID,
                                           VK_ACCESS_TRANSFER_WRITE_BIT,
                                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

        // base
        frameGraph->AddTaskNodeResourceRef(baseTaskNodeID, false, cameraResourceNodeID, 0,
                                           VK_ACCESS_SHADER_READ_BIT,
                                           VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        frameGraph->AddTaskNodeResourceRef(baseTaskNodeID, false, baseInColorResourceNodeID, baseOutColorResourceNodeID,
                                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                           VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

        // opaque
        frameGraph->AddTaskNodeResourceRef(opaqueTaskNodeID, false, cameraResourceNodeID, 0,
                                           VK_ACCESS_SHADER_READ_BIT,
                                           VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        frameGraph->AddTaskNodeResourceRef(opaqueTaskNodeID, false, baseOutColorResourceNodeID, opaqueOutColorResourceNodeID,
                                           VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                           VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
        frameGraph->AddTaskNodeResourceRef(opaqueTaskNodeID, false, opaqueInDepthResourceNodeID, opaqueOutDepthResourceNodeID,
                                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                           VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                           VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
        frameGraph->Compile();
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