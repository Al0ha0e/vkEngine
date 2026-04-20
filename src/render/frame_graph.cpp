#include <render/frame_graph.hpp>
#include <logger.hpp>
#include <algorithm>

namespace vke_render
{

    void FrameGraph::init()
    {
        queueFamilies[RENDER_TASK] = RenderEnvironment::GetInstance()->queueFamilyIndices.graphicsAndComputeFamily.value();
        queueFamilies[COMPUTE_TASK] = RenderEnvironment::GetInstance()->queueFamilyIndices.computeOnlyFamily.value();
        queueFamilies[TRANSFER_TASK] = RenderEnvironment::GetInstance()->queueFamilyIndices.transferOnlyFamily.value();

        for (int i = 0; i < framesInFlight; i++)
            cpuSemaphores.push_back(std::make_unique<std::atomic<bool>>(true));

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 1; i < TASK_TYPE_CNT - 1; i++)
            if (RenderEnvironment::HasQueue(QueueType(i)))
                for (int j = 0; j < framesInFlight; j++)
                    vkCreateFence(globalLogicalDevice, &fenceInfo, nullptr, &fences[i - 1][j]);

        for (int i = 0; i < TASK_TYPE_CNT - 1; i++)
            if (RenderEnvironment::HasQueue(QueueType(i)))
                for (int j = 0; j < framesInFlight; j++)
                    commandPools[i].push_back(std::make_unique<GPUCommandPool>(queueFamilies[i], VK_COMMAND_POOL_CREATE_TRANSIENT_BIT));
        for (int j = 0; j < framesInFlight; j++)
            commandPools[CPU_TASK].push_back(std::make_unique<CPUCommandPool>());

        semaphorePool = std::make_unique<SemaphorePool>();
    }

    void FrameGraph::checkCrossQueue(ResourceRef &ref, TaskNode &taskNode)
    {
        ResourceNode &resourceNode = *resourceNodes[ref.GetResourceNodeID()];
        auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];

        if (resource->tmpPrevUsedTask != nullptr && resource->tmpPrevUsedTask->actualTaskType != taskNode.actualTaskType)
        {
            resource->tmpPrevUsedTask->needQueueSubmit = true;
            taskNode.needQueueSubmit = true;
            resource->tmpPrevUsedRef->crossQueueRef = &ref;
            resource->tmpPrevUsedRef->crossQueueTask = &taskNode;
        }

        resource->tmpPrevUsedTask = &taskNode;
        resource->tmpPrevUsedRef = &ref;
    }

    void FrameGraph::Compile()
    {
        VKE_LOG_INFO("-------------------------- BEGIN COMPILE-----------------------")
        orderedTasks.clear();
        semaphorePool->Reset();
        std::set<vke_ds::id32_t> visited;
        std::queue<vke_ds::id32_t> taskQueue;

        // find valid tasks

        VKE_LOG_INFO("task node cnt {}", taskNodes.size())

        for (auto &kv : taskNodes)
        {
            TaskNode &node = *kv.second;
            node.Reset();

            uint32_t odeg = 0;
            for (auto &ref : node.resourceRefs)
                if (ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE)
                    ++odeg;

            for (auto &ref : node.resourceRefs)
            {
                if (ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE)
                {
                    ResourceNode &resourceNode = *resourceNodes[ref.outResourceNodeID];
                    if (resourceNode.dstTaskIDs.size() == 0 && targetResources.find(resourceNode.resourceID) != targetResources.end())
                    {
                        taskQueue.push(kv.first);
                        visited.insert(kv.first);
                        break;
                    }
                }
                else if (ref.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD && odeg == 0)
                {
                    ResourceNode &resourceNode = *resourceNodes[ref.inResourceNodeID];
                    if (targetResources.find(resourceNode.resourceID) != targetResources.end())
                    {
                        taskQueue.push(kv.first);
                        visited.insert(kv.first);
                        break;
                    }
                }
            }
        }
        VKE_LOG_INFO("final out cnt {}", taskQueue.size())

        while (!taskQueue.empty())
        {
            TaskNode &node = *taskNodes[taskQueue.front()];
            taskQueue.pop();
            node.valid = true;

            for (auto &ref : node.resourceRefs)
            {
                ref.crossQueueRef = nullptr;
                ref.crossQueueTask = nullptr;
                if (ref.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
                {
                    ResourceNode &resourceNode = *resourceNodes[ref.inResourceNodeID];
                    vke_ds::id32_t srcTaskID = resourceNode.srcTaskID;
                    if (srcTaskID != 0)
                    {
                        ++node.indeg;
                        if (visited.find(srcTaskID) == visited.end())
                        {
                            taskQueue.push(srcTaskID);
                            visited.insert(srcTaskID);
                        }
                    }
                }
            }
        }

        // sort tasks
        for (auto &kv : taskNodes)
        {
            TaskNode &node = *kv.second;
            if (node.valid && node.indeg == 0)
                taskQueue.push(kv.first);
        }

        while (!taskQueue.empty())
        {
            TaskNode &node = *taskNodes[taskQueue.front()];
            taskQueue.pop();
            orderedTasks.push_back(node.taskID);

            if (node.currentSemaphore == nullptr)
            {
                node.currentSemaphore = semaphorePool->AllocateSemaphore();
                node.semaphoreValueOffset = 1;
            }

            bool allocateNext = true;

            for (auto &ref : node.resourceRefs)
            {
                if (ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE)
                {
                    vke_ds::id32_t resourceNodeID = ref.outResourceNodeID;
                    ResourceNode &resourceNode = *resourceNodes[resourceNodeID];
                    for (auto dstTaskID : resourceNode.dstTaskIDs)
                    {
                        TaskNode &dstNode = *taskNodes[dstTaskID];
                        if (dstNode.valid)
                        {
                            if (allocateNext && dstNode.currentSemaphore == nullptr)
                            {
                                allocateNext = false;
                                dstNode.currentSemaphore = node.currentSemaphore;
                                dstNode.semaphoreValueOffset = node.semaphoreValueOffset + 1;
                            }

                            dstNode.indeg--;
                            if (dstNode.indeg == 0)
                                taskQueue.push(dstTaskID);
                        }
                    }
                }
            }
        }

        for (auto &resource : permanentResources)
            resource->ResetTmpValues();

        for (auto &kv : transientResources)
            kv.second->ResetTmpValues();

        for (int i = 0; i < TASK_TYPE_CNT; ++i)
            submitCntEstimates[i] = 0;

        for (auto taskID : orderedTasks)
        {
            TaskNode &taskNode = *taskNodes[taskID];
            for (auto &ref : taskNode.resourceRefs)
                checkCrossQueue(ref, taskNode);
        }

        // construct first access
        for (auto taskID : orderedTasks)
        {
            TaskNode &taskNode = *taskNodes[taskID];
            for (auto &ref : taskNode.resourceRefs)
            {
                ResourceNode &resourceNode = *resourceNodes[ref.outResourceNodeID == 0 ? ref.inResourceNodeID : ref.outResourceNodeID]; // first use out
                auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];
                if (resource->firstAccessTaskID == 0)
                    resource->firstAccessTaskID = taskID;
            }
        }
        // construct last access
        for (int i = orderedTasks.size() - 1; i >= 0; --i)
        {
            vke_ds::id32_t taskID = orderedTasks[i];
            TaskNode &taskNode = *taskNodes[taskID];
            for (auto &ref : taskNode.resourceRefs)
            {
                ResourceNode &resourceNode = *resourceNodes[ref.outResourceNodeID == 0 ? ref.inResourceNodeID : ref.outResourceNodeID]; // last use out
                auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];
                if (resource->lastAccessTaskID == 0)
                    resource->lastAccessTaskID = taskID;
            }

            uint32_t &cnt = submitCntEstimates[taskNode.actualTaskType];
            if (cnt == 0)
            {
                taskNode.isFinalTask = true;
                ++cnt;
            }
            else if (taskNode.needQueueSubmit)
                ++cnt;
        }

        // calculate transient memory usage
        {
            transientMemoryAllocationMap.clear();
            transientMemorySimulator.Reset();

            VkMemoryRequirements memoryReq;
            for (auto taskID : orderedTasks)
            {
                TaskNode &taskNode = *taskNodes[taskID];
                for (auto &ref : taskNode.resourceRefs)
                {
                    if (!ref.isTransient)
                        continue;
                    ResourceNode &resourceNode = *resourceNodes[ref.GetResourceNodeID()];
                    auto &resource = transientResources[resourceNode.resourceID];
                    if (resource->firstAccessTaskID == taskID) // transient first access must be RENDER/COMPUTE TASK
                    {
                        VKE_FATAL_IF((taskNode.actualTaskType != RENDER_TASK) && (taskNode.actualTaskType != COMPUTE_TASK), "transient first access must be RENDER/COMPUTE TASK")
                        if (resource->resourceType == IMAGE_RESOURCE)
                        {
                            ImageResource *imageResource = (ImageResource *)resource.get();
                            vkGetImageMemoryRequirements(globalLogicalDevice, imageResource->images[0], &memoryReq);
                        }
                        else
                        {
                            BufferResource *bufferResource = (BufferResource *)resource.get();
                            vkGetBufferMemoryRequirements(globalLogicalDevice, bufferResource->buffers[0], &memoryReq);
                        }
                        transientMemoryAllocationMap[resourceNode.resourceID] = transientMemorySimulator.PreAllocMemory(taskNode.actualTaskType, memoryReq);
                    }
                }

                for (auto &ref : taskNode.resourceRefs)
                {
                    if (!ref.isTransient)
                        continue;
                    ResourceNode &resourceNode = *resourceNodes[ref.GetResourceNodeID()];
                    auto &resource = transientResources[resourceNode.resourceID];
                    if (resource->lastAccessTaskID == taskID)
                    {
                        TransientMemoryAllocation &allocation = transientMemoryAllocationMap[resourceNode.resourceID];
                        VKE_FATAL_IF(allocation.type != taskNode.actualTaskType, "transient resource must be de/allocated from the same queue")
                        transientMemorySimulator.PreDeallocMemory(allocation);
                    }
                }
            }
            transientMemoryUpdateCnt = framesInFlight;
        }

        VKE_LOG_INFO("orderedTask cnt {}", orderedTasks.size())
        for (auto taskID : orderedTasks)
            VKE_LOG_INFO("task id {}", taskID)
        VKE_LOG_INFO("-------------------------- END COMPILE-----------------------")
    }

    void FrameGraph::syncResources(const uint32_t currentFrame, const uint32_t imageIndex,
                                   ResourceRef &ref, TaskNode &taskNode,
                                   bool &needQueueSubmit,
                                   std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                                   std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers,
                                   std::map<VkSemaphore, uint64_t> &waitSemaphoreMap,
                                   VkPipelineStageFlags2 &waitDstStageMask)
    {
        ResourceNode &resourceNode = *resourceNodes[ref.GetResourceNodeID()];
        auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];
        // std::cout << "----SYNC RESOURCE " << "TASK " << taskNode.name << " RESOURCE_NODE " << resourceNode.name << " RESOURCE " << resource->name << "\n";
        // std::cout << "resource prevUsedTask " << resource->prevUsedTask << " " << resource->prevUsedRef << "\n";
        bool layoutChanged = false;
        if (resource->firstAccessTaskID == taskNode.taskID) // first use of resource
        {
            if (resourceNode.isTransient)
            {
                layoutChanged = true;
                if (resource->resourceType == IMAGE_RESOURCE)
                {
                    ImageResource *imageResource = (ImageResource *)resource.get();
                    imageMemoryBarriers.emplace_back();
                    VkImageMemoryBarrier2 &barrier = imageMemoryBarriers.back();
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                    barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                    barrier.srcAccessMask = VK_ACCESS_2_NONE;
                    barrier.dstStageMask = ref.stageMask;
                    barrier.dstAccessMask = ref.accessMask;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    barrier.newLayout = ref.imageLayout;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = imageResource->GetCurrentImage(currentFrame, imageIndex);
                    barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                }
                else // BUFFER_RESOURCE
                {
                    BufferResource *bufferResource = (BufferResource *)resource.get();
                    bufferMemoryBarriers.emplace_back();
                    VkBufferMemoryBarrier2 &barrier = bufferMemoryBarriers.back();
                    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
                    barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                    barrier.srcAccessMask = VK_ACCESS_NONE;
                    barrier.dstStageMask = ref.stageMask;
                    barrier.dstAccessMask = ref.accessMask;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.buffer = bufferResource->GetCurrentBuffer(currentFrame);
                    barrier.offset = bufferResource->offset;
                    barrier.size = bufferResource->size;
                }
            }
            else if (resource->resourceType == IMAGE_RESOURCE &&
                     permanentResourceStates[resourceNode.resourceID].stImageLayout.has_value())
            {
                PermanentResourceState &state = permanentResourceStates[resourceNode.resourceID];
                if (state.stImageLayout.has_value() &&
                    state.stImageLayout.value() != ref.imageLayout)
                {
                    layoutChanged = true;
                    // std::cout << "  FIRST IMAGE USE\n";
                    ImageResource *imageResource = (ImageResource *)resource.get();
                    VKE_LOG_DEBUG("FIRST IMAGE USE {} {}", imageResource->name, (void *)(imageResource->GetCurrentImage(currentFrame, imageIndex)))
                    imageMemoryBarriers.emplace_back();
                    VkImageMemoryBarrier2 &barrier = imageMemoryBarriers.back();
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                    barrier.pNext = nullptr;
                    barrier.srcStageMask = state.stStage;
                    barrier.srcAccessMask = VK_ACCESS_NONE;
                    barrier.dstStageMask = ref.stageMask;
                    barrier.dstAccessMask = ref.accessMask;
                    barrier.oldLayout = state.stImageLayout.value();
                    barrier.newLayout = ref.imageLayout;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = imageResource->GetCurrentImage(currentFrame, imageIndex);
                    barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                }
            }
        }
        else if (resource->prevUsedTask != nullptr)
        {
            ResourceRef &prevUsedRef = *(resource->prevUsedRef);
            TaskNode &prevUsedTask = *(resource->prevUsedTask);
            // std::cout << "PREVUSED TASK " << prevUsedTask.name << "\n";
            if (prevUsedTask.actualTaskType != taskNode.actualTaskType) // cross queue
            {
                // VKE_LOG_DEBUG("CROSS QUEUE {} {}", (uint32_t)prevUsedRef.imageLayout, (uint32_t)ref.imageLayout)
                needQueueSubmit = true;
                waitDstStageMask |= ref.stageMask;

                auto it = waitSemaphoreMap.find(prevUsedTask.lastSemaphore);
                if (it == waitSemaphoreMap.end())
                    waitSemaphoreMap[prevUsedTask.lastSemaphore] = prevUsedTask.lastSemaphoreValue;
                else
                    waitSemaphoreMap[prevUsedTask.lastSemaphore] = std::max(it->second, prevUsedTask.lastSemaphoreValue);

                if (resource->resourceType == IMAGE_RESOURCE &&
                    ((resource->prevWrite && ref.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD) ||
                     (prevUsedRef.imageLayout != ref.imageLayout)))
                {
                    // std::cout << "  CROSS QUEUE IMAGE RECEIVE\n";
                    layoutChanged = prevUsedRef.imageLayout != ref.imageLayout;
                    if (layoutChanged)
                        waitDstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // prevent layout transfer conflict with prev write op in another queue
                    ImageResource *imageResource = (ImageResource *)resource.get();
                    VKE_LOG_DEBUG("CROSS QUEUE IMAGE RECEIVE {} {}", imageResource->name, (void *)(imageResource->GetCurrentImage(currentFrame, imageIndex)))
                    imageMemoryBarriers.emplace_back();
                    VkImageMemoryBarrier2 &barrier = imageMemoryBarriers.back();
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                    barrier.pNext = nullptr;
                    barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    barrier.srcAccessMask = VK_ACCESS_NONE;
                    barrier.dstStageMask = ref.stageMask;
                    barrier.dstAccessMask = ref.accessMask;
                    barrier.oldLayout = prevUsedRef.imageLayout;
                    barrier.newLayout = ref.imageLayout;
                    if (prevUsedTask.actualTaskType != CPU_TASK && taskNode.actualTaskType != CPU_TASK &&
                        resource->prevWrite && ref.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
                    {
                        VKE_LOG_DEBUG("REAL CROSS QUEUE IMAGE RECEIVE")
                        barrier.srcQueueFamilyIndex = queueFamilies[resource->prevUsedTask->actualTaskType];
                        barrier.dstQueueFamilyIndex = queueFamilies[taskNode.actualTaskType];
                    }
                    else
                    {
                        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    }
                    barrier.image = imageResource->GetCurrentImage(currentFrame, imageIndex);
                    barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                }
            }
            else
            {
                // std::cout << "  SAME QUEUE\n";
                if ((ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE) ||
                    resource->prevWrite ||
                    ((resource->resourceType == IMAGE_RESOURCE) && (prevUsedRef.imageLayout != ref.imageLayout)))
                {
                    bool war = (!resource->prevWrite &&
                                prevUsedRef.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD &&
                                ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE);
                    // std::cout << "  NEED SYNC\n";
                    if (resource->resourceType == IMAGE_RESOURCE)
                    {
                        layoutChanged = prevUsedRef.imageLayout != ref.imageLayout;
                        war = war && prevUsedRef.imageLayout == ref.imageLayout;
                        // std::cout << "  IMAGE SYNC\n";
                        ImageResource *imageResource = (ImageResource *)resource.get();
                        imageMemoryBarriers.emplace_back();
                        VkImageMemoryBarrier2 &barrier = imageMemoryBarriers.back();
                        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                        barrier.pNext = nullptr;
                        barrier.srcStageMask = prevUsedRef.stageMask;
                        barrier.srcAccessMask = war ? VK_ACCESS_NONE : prevUsedRef.accessMask;
                        barrier.dstStageMask = ref.stageMask;
                        barrier.dstAccessMask = war ? VK_ACCESS_NONE : ref.accessMask;
                        barrier.oldLayout = prevUsedRef.imageLayout;
                        barrier.newLayout = ref.imageLayout;
                        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.image = imageResource->GetCurrentImage(currentFrame, imageIndex);
                        barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                        barrier.subresourceRange.baseMipLevel = 0;
                        barrier.subresourceRange.levelCount = 1;
                        barrier.subresourceRange.baseArrayLayer = 0;
                        barrier.subresourceRange.layerCount = 1;
                    }
                    else
                    {
                        // std::cout << "  BUFFER SYNC\n";
                        BufferResource *bufferResource = (BufferResource *)resource.get();
                        bufferMemoryBarriers.emplace_back();
                        VkBufferMemoryBarrier2 &barrier = bufferMemoryBarriers.back();
                        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
                        barrier.pNext = nullptr;
                        barrier.srcStageMask = prevUsedRef.stageMask;
                        barrier.srcAccessMask = war ? VK_ACCESS_NONE : prevUsedRef.accessMask;
                        barrier.dstStageMask = ref.stageMask;
                        barrier.dstAccessMask = war ? VK_ACCESS_NONE : ref.accessMask;
                        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.buffer = bufferResource->GetCurrentBuffer(currentFrame);
                        barrier.offset = bufferResource->offset;
                        barrier.size = bufferResource->size;
                    }
                }
            }
        }
        resource->prevWrite = (ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE) || layoutChanged;
        resource->prevUsedRef = &ref;
        resource->prevUsedTask = &taskNode;
    }

    void FrameGraph::endResourcesUse(const uint32_t currentFrame, const uint32_t imageIndex,
                                     ResourceRef &ref, TaskNode &taskNode,
                                     bool &needQueueSubmit,
                                     std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                                     std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers)
    {
        ResourceNode &resourceNode = *resourceNodes[ref.outResourceNodeID == 0 ? ref.inResourceNodeID : ref.outResourceNodeID]; // first use out
        auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];
        // std::cout << "----END RESOURCE " << "TASK " << taskNode.name << " RESOURCE_NODE " << resourceNode.name << " RESOURCE " << resourceNode.name << "\n";
        if (ref.crossQueueRef != nullptr)
        {
            // std::cout << "  CROSS QUEUE\n";
            ResourceRef &crossQueueRef = *(ref.crossQueueRef);
            needQueueSubmit = true;
            if (resource->resourceType == IMAGE_RESOURCE &&
                ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE &&
                crossQueueRef.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
            {
                ImageResource *imageResource = (ImageResource *)resource.get();
                VKE_LOG_DEBUG("CROSS QUEUE IMAGE RELEASE {} {}", imageResource->name, (void *)(imageResource->GetCurrentImage(currentFrame, imageIndex)))
                imageMemoryBarriers.emplace_back();
                VkImageMemoryBarrier2 &barrier = imageMemoryBarriers.back();
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                barrier.pNext = nullptr;
                barrier.srcStageMask = ref.stageMask;
                barrier.srcAccessMask = ref.accessMask;
                barrier.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                barrier.dstAccessMask = VK_ACCESS_NONE;
                barrier.oldLayout = ref.imageLayout;
                barrier.newLayout = ref.imageLayout;
                if (taskNode.actualTaskType != CPU_TASK &&
                    ref.crossQueueTask->actualTaskType != CPU_TASK &&
                    ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE &&
                    crossQueueRef.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
                {
                    VKE_LOG_DEBUG("REAL CROSS QUEUE IMAGE RELEASE")
                    barrier.srcQueueFamilyIndex = queueFamilies[taskNode.actualTaskType];
                    barrier.dstQueueFamilyIndex = queueFamilies[ref.crossQueueTask->actualTaskType];
                }
                else
                {
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                }

                barrier.image = imageResource->GetCurrentImage(currentFrame, imageIndex);
                barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
            }
        }
        else if (!resourceNode.isTransient && resource->lastAccessTaskID == taskNode.taskID && resource->resourceType == IMAGE_RESOURCE) // TODO final op 如何判断是最后一个？尤其是最后一个task是读的情况，此时没有任何对应的resourceNode.dstTaskIDs.size() == 0
        {
            PermanentResourceState &state = permanentResourceStates[resourceNode.resourceID];
            if (state.enImageLayout.has_value() &&
                state.enImageLayout.value() != ref.imageLayout)
            {
                // std::cout << "  FINAL IMAGE USE\n";
                ImageResource *imageResource = (ImageResource *)resource.get();
                imageMemoryBarriers.emplace_back();
                VkImageMemoryBarrier2 &barrier = imageMemoryBarriers.back();
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                barrier.pNext = nullptr;
                barrier.srcStageMask = ref.stageMask;
                barrier.srcAccessMask = ref.accessMask;
                barrier.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                barrier.dstAccessMask = VK_ACCESS_NONE;
                barrier.oldLayout = ref.imageLayout;
                barrier.newLayout = state.enImageLayout.value();
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = imageResource->GetCurrentImage(currentFrame, imageIndex);
                barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
            }
        }
    }

    void FrameGraph::Sync(const uint32_t currentFrame)
    {
        for (int i = 1; i < TASK_TYPE_CNT - 1; ++i)
            if (RenderEnvironment::HasQueue(QueueType(i)) && vkGetFenceStatus(globalLogicalDevice, fences[i - 1][currentFrame]) == VK_NOT_READY)
            {
                VKE_LOG_DEBUG("WAIT FOR FENCE0 {} {}", i, (void *)(fences[i - 1][currentFrame]))
                vkWaitForFences(globalLogicalDevice, 1, &(fences[i - 1][currentFrame]), VK_TRUE, UINT64_MAX);
                VKE_LOG_DEBUG("WAIT FOR FENCE1 {} {}", i, (void *)(fences[i - 1][currentFrame]))
            }

        if (!cpuSemaphores[currentFrame]->load())
        {
            VKE_LOG_DEBUG("WAIT FOR CPU0 {}", (void *)(cpuSemaphores[currentFrame].get()))
            cpuSemaphores[currentFrame]->wait(false);
            VKE_LOG_DEBUG("WAIT FOR CPU1 {}", (void *)(cpuSemaphores[currentFrame].get()))
        }
    }

    void FrameGraph::UpdateTransientMemory(const uint32_t currentFrame)
    {
        if (transientMemoryUpdateCnt == 0)
            return;
        --transientMemoryUpdateCnt;

        RenderEnvironment *env = RenderEnvironment::GetInstance();

        transientMemoryManager.Realloc(RENDER_TASK, currentFrame, transientMemorySimulator);
        transientMemoryManager.Realloc(COMPUTE_TASK, currentFrame, transientMemorySimulator);

        for (auto &kv : transientResources)
        {
            vke_ds::id32_t resourceID = kv.first;
            RenderResource *resource = kv.second.get();

            auto allocationIt = transientMemoryAllocationMap.find(resourceID);
            if (allocationIt == transientMemoryAllocationMap.end())
                continue;

            const TransientMemoryAllocation &transientAllocation = allocationIt->second;
            VmaAllocation poolAllocation = transientMemoryManager.GetPoolVmaAllocation(transientAllocation.type, currentFrame, transientAllocation.idx);
            VkDeviceSize bindOffset = static_cast<VkDeviceSize>(transientAllocation.offset);

            if (resource->resourceType == IMAGE_RESOURCE)
            {
                ImageResource *imageResource = (ImageResource *)resource;
                VKE_VK_CHECK(vmaBindImageMemory2(env->vmaAllocator, poolAllocation, bindOffset, imageResource->images[currentFrame], nullptr),
                             "failed to bind transient image memory!")
            }
            else
            {
                BufferResource *bufferResource = (BufferResource *)resource;
                VKE_VK_CHECK(vmaBindBufferMemory2(env->vmaAllocator, poolAllocation, bindOffset, bufferResource->buffers[currentFrame], nullptr),
                             "failed to bind transient buffer memory!")
            }
        }

        for (auto taskID : orderedTasks)
        {
            TaskNode &taskNode = *taskNodes[taskID];
            if (taskNode.transientReadyCallback)
                taskNode.transientReadyCallback(taskNode, *this, currentFrame);
        }
    }

    void FrameGraph::Execute(const uint32_t currentFrame, const uint32_t imageIndex)
    {
        VkCommandBuffer commandBuffers[TASK_TYPE_CNT] = {nullptr, nullptr, nullptr, nullptr};
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        for (int i = 0; i < TASK_TYPE_CNT; ++i)
            if (RenderEnvironment::HasQueue(QueueType(i)) && (submitCntEstimates[i] > 0))
            {
                CommandPool *commandPool = commandPools[i][currentFrame].get();
                commandPool->Reset();
                commandPool->PreAllocateCommandBuffer(submitCntEstimates[i]);
                commandBuffers[i] = commandPool->AllocateAndBegin(&beginInfo);
            }
        VKE_LOG_DEBUG("---------------------EXE-------------------")
        uint32_t actualSubmitCnts[TASK_TYPE_CNT] = {0, 0, 0, 0};
        VkPipelineStageFlags2 waitDstStageMask = 0;
        std::vector<VkBufferMemoryBarrier2> bufferMemoryBarriers;
        std::vector<VkImageMemoryBarrier2> imageMemoryBarriers;
        std::map<VkSemaphore, uint64_t> waitSemaphoreMap;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreInfos;
        VkDependencyInfo dependencyInfo{};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.pNext = nullptr;

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.pNext = nullptr;
        commandBufferSubmitInfo.deviceMask = 0;

        VkSemaphoreSubmitInfo signalSemaphoreInfo{};
        signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalSemaphoreInfo.pNext = nullptr;
        signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        signalSemaphoreInfo.deviceIndex = 0;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.pNext = nullptr; //&tlsSubmitInfo;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

        for (auto taskID : orderedTasks)
        {
            TaskNode &taskNode = *taskNodes[taskID];
            TaskType actualTaskType = taskNode.actualTaskType;
            bool needQueueSubmit = taskNode.isFinalTask;
            VkCommandBuffer commandBuffer = commandBuffers[actualTaskType];
            VKE_LOG_DEBUG("-----------TASK <{}> TYPE {} ACTUAL {}", taskNode.name, (uint32_t)taskNode.taskType, (uint32_t)actualTaskType)
            bufferMemoryBarriers.clear();
            imageMemoryBarriers.clear();
            for (auto &ref : taskNode.resourceRefs)
                syncResources(currentFrame, imageIndex, ref, taskNode, needQueueSubmit,
                              bufferMemoryBarriers, imageMemoryBarriers,
                              waitSemaphoreMap, waitDstStageMask);
            dependencyInfo.bufferMemoryBarrierCount = bufferMemoryBarriers.size();
            dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();
            dependencyInfo.imageMemoryBarrierCount = imageMemoryBarriers.size();
            dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();
            if (dependencyInfo.bufferMemoryBarrierCount > 0 || dependencyInfo.imageMemoryBarrierCount > 0)
                vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

            taskNode.executeCallback(taskNode, *this, commandBuffer, currentFrame, imageIndex);

            bufferMemoryBarriers.clear();
            imageMemoryBarriers.clear();
            for (auto &ref : taskNode.resourceRefs)
                endResourcesUse(currentFrame, imageIndex, ref, taskNode, needQueueSubmit,
                                bufferMemoryBarriers, imageMemoryBarriers);
            dependencyInfo.bufferMemoryBarrierCount = bufferMemoryBarriers.size();
            dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();
            dependencyInfo.imageMemoryBarrierCount = imageMemoryBarriers.size();
            dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();
            if (dependencyInfo.bufferMemoryBarrierCount > 0 || dependencyInfo.imageMemoryBarrierCount > 0)
                vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

            if (needQueueSubmit)
            {
                if (actualTaskType != CPU_TASK)
                    vkEndCommandBuffer(commandBuffer);

                commandBufferSubmitInfo.commandBuffer = commandBuffer;

                for (auto [k, v] : waitSemaphoreMap)
                {
                    waitSemaphoreInfos.push_back(VkSemaphoreSubmitInfo{
                        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                        .semaphore = k,
                        .value = v,
                        .stageMask = waitDstStageMask,
                        .deviceIndex = 0});
                    VKE_LOG_DEBUG("------TASK <{}> WAIT FOR {} {}", taskNode.name, (void *)(k), v)
                }

                submitInfo.waitSemaphoreInfoCount = waitSemaphoreMap.size();
                submitInfo.pWaitSemaphoreInfos = waitSemaphoreInfos.data();

                uint64_t signalSemaphoreValue = taskNode.semaphoreValueOffset + semaphoreValueBase;
                signalSemaphoreInfo.semaphore = taskNode.currentSemaphore;
                signalSemaphoreInfo.value = signalSemaphoreValue;

                CommandQueue *queue = RenderEnvironment::GetQueue(QueueType(actualTaskType));

                VKE_LOG_DEBUG("------TASK <{}> SUBMIT TO QUEUE {} COMMAND BUFFER {} SIGNAL {} {}", taskNode.name, (void *)queue, (void *)commandBuffer, (void *)(signalSemaphoreInfo.semaphore), signalSemaphoreValue)
                ++actualSubmitCnts[actualTaskType];

                if (taskNode.isFinalTask)
                {
                    VkFence fence = nullptr;
                    if (actualTaskType == CPU_TASK)
                    {
                        VKE_LOG_DEBUG("RESET CPU FENCE {}", (void *)(cpuSemaphores[currentFrame].get()))
                        cpuSemaphores[currentFrame]->store(false);
                        fence = (VkFence)(cpuSemaphores[currentFrame].get());
                    }
                    else if (actualTaskType != RENDER_TASK)
                    {
                        fence = fences[actualTaskType - 1][currentFrame];
                        VKE_LOG_DEBUG("RESET FENCE {} {}", (uint32_t)actualTaskType, (void *)fence)
                        vkResetFences(globalLogicalDevice, 1, &fence);
                        VKE_LOG_DEBUG("RESET FENCE {} {}", (uint32_t)actualTaskType, (void *)fence)
                    }
                    queue->Submit(1, &submitInfo, fence);
                }
                else
                {
                    queue->Submit(1, &submitInfo, VK_NULL_HANDLE);
                    commandBuffers[actualTaskType] = commandPools[actualTaskType][currentFrame]->AllocateAndBegin(&beginInfo);
                }

                waitDstStageMask = 0;
                waitSemaphoreMap.clear();
                waitSemaphoreInfos.clear();
                taskNode.lastSemaphore = taskNode.currentSemaphore;
                taskNode.lastSemaphoreValue = taskNode.semaphoreValueOffset + semaphoreValueBase;
            }
        }
        for (int i = 0; i < TASK_TYPE_CNT; i++)
            submitCntEstimates[i] = actualSubmitCnts[i];

        for (auto &resource : permanentResources)
            if (resource->framesInFlight)
                resource->ResetPrev();
        for (auto &[id, resource] : transientResources)
            resource->ResetPrev();

        semaphoreValueBase += orderedTasks.size();
    }
}