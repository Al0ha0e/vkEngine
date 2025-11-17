#include <render/render.hpp>
#include <logger.hpp>
#include <algorithm>

namespace vke_render
{
    Renderer *Renderer::instance;

    void Renderer::initDescriptorSet()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindingInfos;
        VkDescriptorSetLayoutBinding camInfoLayoutBinding{};
        camInfoLayoutBinding.binding = 0;
        camInfoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        camInfoLayoutBinding.descriptorCount = 1;
        camInfoLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
        bindingInfos.push_back(camInfoLayoutBinding);

        globalDescriptorSetInfos[GLOBAL_DESCRIPTOR_SET_NO_LIGHT].AddCnt(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
        globalDescriptorSetInfos[GLOBAL_DESCRIPTOR_SET_LIGHT].AddCnt(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
        LightManager::GetBindingInfo(bindingInfos, globalDescriptorSetInfos[GLOBAL_DESCRIPTOR_SET_LIGHT]);

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layoutInfo.pBindings = bindingInfos.data();
        vkCreateDescriptorSetLayout(globalLogicalDevice, &layoutInfo,
                                    nullptr, &(globalDescriptorSetInfos[GLOBAL_DESCRIPTOR_SET_NO_LIGHT].layout));
        layoutInfo.bindingCount = 2;
        vkCreateDescriptorSetLayout(globalLogicalDevice, &layoutInfo,
                                    nullptr, &(globalDescriptorSetInfos[GLOBAL_DESCRIPTOR_SET_LIGHT].layout));

        globalDescriptorSets[GLOBAL_DESCRIPTOR_SET_NO_LIGHT] = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(globalDescriptorSetInfos[GLOBAL_DESCRIPTOR_SET_NO_LIGHT]);
        globalDescriptorSets[GLOBAL_DESCRIPTOR_SET_LIGHT] = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(globalDescriptorSetInfos[GLOBAL_DESCRIPTOR_SET_LIGHT]);

        // write descriptor set
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        VkDescriptorBufferInfo bufferInfo = camInfoBuffer.GetDescriptorBufferInfo();
        descriptorWrites.push_back(VkWriteDescriptorSet{});
        ConstructDescriptorSetWrite(descriptorWrites[0], globalDescriptorSets[GLOBAL_DESCRIPTOR_SET_NO_LIGHT], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        vkUpdateDescriptorSets(globalLogicalDevice, 1, descriptorWrites.data(), 0, nullptr);

        descriptorWrites[0].dstSet = globalDescriptorSets[GLOBAL_DESCRIPTOR_SET_LIGHT];
        LightManager::GetDescriptorSetWrite(descriptorWrites, globalDescriptorSets[GLOBAL_DESCRIPTOR_SET_LIGHT]);
        vkUpdateDescriptorSets(globalLogicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }

    void Renderer::initFrameGraph(std::map<std::string, vke_ds::id32_t> &blackboard,
                                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
    {
        instance->frameGraph = std::make_unique<FrameGraph>(MAX_FRAMES_IN_FLIGHT);
        VkDescriptorImageInfo info{};
        info.sampler = nullptr;
        info.imageView = (*context.colorImageViews)[0];
        colorAttachmentResourceID = frameGraph->AddPermanentImageResource("colorAttachment", (*context.colorImages)[0], VK_IMAGE_ASPECT_COLOR_BIT, info,
                                                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        info.imageView = context.depthImageView;
        vke_ds::id32_t depthAttachmentResourceID = frameGraph->AddPermanentImageResource("depthAttachment", context.depthImage, VK_IMAGE_ASPECT_DEPTH_BIT, info,
                                                                                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, std::nullopt, std::nullopt);
        VkDescriptorBufferInfo binfo{};
        binfo.buffer = camInfoBuffer.buffer;
        binfo.offset = 0;
        binfo.range = camInfoBuffer.bufferSize;
        vke_ds::id32_t cameraResourceID = frameGraph->AddPermanentBufferResource("camInfoBuffer", binfo, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VKE_LOG_INFO("resource ids {} {} {}", colorAttachmentResourceID, depthAttachmentResourceID, cameraResourceID)

        frameGraph->AddTargetResource(colorAttachmentResourceID);
        frameGraph->AddTargetResource(depthAttachmentResourceID);

        blackboard["colorAttachment"] = colorAttachmentResourceID;
        blackboard["depthAttachment"] = depthAttachmentResourceID;
        blackboard["cameraInfo"] = cameraResourceID;

        vke_ds::id32_t oriColorResourceNodeID = frameGraph->AllocResourceNode("oriColor", false, colorAttachmentResourceID);
        vke_ds::id32_t oriDepthResourceNodeID = frameGraph->AllocResourceNode("oriDepth", false, depthAttachmentResourceID);
        cameraResourceNodeID = frameGraph->AllocResourceNode("oriCamera", false, cameraResourceID);

        currentResourceNodeID[colorAttachmentResourceID] = oriColorResourceNodeID;
        currentResourceNodeID[depthAttachmentResourceID] = oriDepthResourceNodeID;
        currentResourceNodeID[cameraResourceID] = cameraResourceNodeID;

        auto copyCallback = [this](TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
        {
            camInfoBuffer.ToBufferAsync(commandBuffer, 0, camInfoBuffer.bufferSize);
        };

        cameraUpdateTaskID = frameGraph->AllocTaskNode("camera update", TRANSFER_TASK, copyCallback);

        // copy
        frameGraph->AddTaskNodeResourceRef(cameraUpdateTaskID, false, 0, cameraResourceNodeID,
                                           VK_ACCESS_TRANSFER_WRITE_BIT,
                                           VK_PIPELINE_STAGE_TRANSFER_BIT,
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
            VKE_LOG_INFO("Cam updated")
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