#include <render/frame_graph.hpp>
#include <queue>
#include <algorithm>

namespace vke_render
{

    void FrameGraph::init()
    {
        VkSemaphoreTypeCreateInfo timelineInfo{};

        timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineInfo.initialValue = 0;

        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = &timelineInfo;
        VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
        vkCreateSemaphore(logicalDevice, &createInfo, nullptr, &timelineSemaphore);

        gpuQueues[COMPUTE_TASK] = RenderEnvironment::GetInstance()->computeQueue;
        gpuQueues[RENDER_TASK] = RenderEnvironment::GetInstance()->graphicsQueue;
        gpuQueues[TRANSFER_TASK] = RenderEnvironment::GetInstance()->transferQueue;
        queueFamilies[RENDER_TASK] = RenderEnvironment::GetInstance()->queueFamilyIndices.graphicsAndComputeFamily.value();
        queueFamilies[COMPUTE_TASK] = RenderEnvironment::GetInstance()->queueFamilyIndices.computeOnlyFamily.value();
        queueFamilies[TRANSFER_TASK] = RenderEnvironment::GetInstance()->queueFamilyIndices.transferOnlyFamily.value();

        haveQueue[RENDER_TASK] = true;
        haveQueue[COMPUTE_TASK] = gpuQueues[COMPUTE_TASK] != gpuQueues[RENDER_TASK];
        haveQueue[TRANSFER_TASK] = gpuQueues[TRANSFER_TASK] != gpuQueues[RENDER_TASK];

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < 3; i++)
            if (haveQueue[i])
                for (int j = 0; j < framesInFlight; j++)
                {
                    commandPools[i].push_back(std::make_unique<CommandPool>(queueFamilies[i], VK_COMMAND_POOL_CREATE_TRANSIENT_BIT));
                    if (i > 0)
                        vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fences[i - 1][j]);
                }
    }

    void FrameGraph::clearLastUsedInfo()
    {
        for (auto &resource : permanentResources)
        {
            resource->lastUsedTask = nullptr;
            resource->lastUsedRef = nullptr;
        }

        for (auto &kv : transientResources)
        {
            kv.second->lastUsedTask = nullptr;
            kv.second->lastUsedRef = nullptr;
        }
    }

    void FrameGraph::checkCrossQueue(ResourceRef &ref, TaskNode &taskNode)
    {
        ResourceNode &resourceNode = *resourceNodes[ref.inResourceNodeID == 0 ? ref.outResourceNodeID : ref.inResourceNodeID];
        auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];

        if (resource->lastUsedTask != nullptr && resource->lastUsedTask->actualTaskType != taskNode.actualTaskType)
        {
            resource->lastUsedTask->needQueueSubmit = true;
            taskNode.needQueueSubmit = true;
            resource->lastUsedRef->crossQueueRef = &ref;
            resource->lastUsedRef->crossQueueTask = &taskNode;
        }

        resource->lastUsedTask = &taskNode;
        resource->lastUsedRef = &ref;
    }

    void FrameGraph::Compile()
    {
        std::cout << "-------------------------- BEGIN COMPILE-----------------------\n";
        orderedTasks.clear();
        std::set<vke_ds::id32_t> visited;
        std::queue<vke_ds::id32_t> taskQueue;

        // find valid tasks

        std::cout << "task node cnt " << taskNodes.size() << "\n";

        for (auto &kv : taskNodes)
        {
            TaskNode &node = *kv.second;
            node.valid = false;
            for (auto &ref : node.resourceRefs)
            {
                if (ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE)
                {
                    ResourceNode &resourceNode = *resourceNodes[ref.outResourceNodeID];
                    // TODO final op 如何判断是最后一个？尤其是最后一个task是读的情况，此时没有任何对应的resourceNode.dstTaskIDs.size() == 0
                    if (resourceNode.dstTaskIDs.size() == 0 && targetResources.find(resourceNode.resourceID) != targetResources.end())
                    {
                        taskQueue.push(kv.first);
                        visited.insert(kv.first);
                        break;
                    }
                }
            }
        }
        std::cout << "final out cnt " << taskQueue.size() << "\n";

        while (!taskQueue.empty())
        {
            TaskNode &node = *taskNodes[taskQueue.front()];
            taskQueue.pop();
            node.valid = true;
            node.indeg = 0;
            node.needQueueSubmit = false;

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
            node.order = orderedTasks.size();

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
                            dstNode.indeg--;
                            if (dstNode.indeg == 0)
                                taskQueue.push(dstTaskID);
                        }
                    }
                }
            }
        }

        // TODO handle transient resources

        clearLastUsedInfo();
        submitCnts[0] = submitCnts[1] = submitCnts[2] = 0;

        if (haveQueue[COMPUTE_TASK] || haveQueue[TRANSFER_TASK])
        {
            for (auto taskID : orderedTasks)
            {
                TaskNode &taskNode = *taskNodes[taskID];
                for (auto &ref : taskNode.resourceRefs)
                    checkCrossQueue(ref, taskNode);
            }
            clearLastUsedInfo();
        }

        for (int i = orderedTasks.size() - 1; i >= 0; --i)
        {
            TaskNode &taskNode = *taskNodes[orderedTasks[i]];
            uint32_t &cnt = submitCnts[taskNode.actualTaskType];
            if (cnt == 0)
            {
                taskNode.needQueueSubmit = true;
                ++cnt;
            }
            else if (taskNode.needQueueSubmit)
                ++cnt;
        }

        for (int i = 0; i < 3; i++)
            if (haveQueue[i])
                for (int j = 0; j < framesInFlight; j++)
                    commandPools[i][j]->PreAllocateCommandBuffer(submitCnts[i]);

        std::cout << "orderedTask cnt " << orderedTasks.size() << "\n";
        for (auto taskID : orderedTasks)
            std::cout << "task id " << taskID << "\n";
        std::cout << "-------------------------- END COMPILE-----------------------\n";
    }

    void FrameGraph::syncResources(VkCommandBuffer commandBuffer, ResourceRef &ref, TaskNode &taskNode,
                                   std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                                   std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers,
                                   uint64_t &waitSemaphoreValue, VkPipelineStageFlags &waitDstStageMask)
    {
        ResourceNode &resourceNode = *resourceNodes[ref.inResourceNodeID == 0 ? ref.outResourceNodeID : ref.inResourceNodeID];
        auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];
        // std::cout << "----SYNC RESOURCE " << "TASKID " << taskNode.taskID << " RESOURCE_NODE_ID " << resourceNode.resourceNodeID << " RESOURCEID " << resourceNode.resourceID << "\n";
        if (resource->lastUsedRef == nullptr) // first use of resource
        {
            if (!resourceNode.isTransient)
            {
                PermanentResourceState &state = permanentResourceStates[resourceNode.resourceID];
                if (resource->resourceType == IMAGE_RESOURCE &&
                    state.stImageLayout.has_value() &&
                    state.stImageLayout.value() != ref.imageLayout)
                {
                    //  std::cout << "  FIRST IMAGE USE\n";
                    ImageResource *imageResource = (ImageResource *)resource.get();
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
                    barrier.image = imageResource->image;
                    barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                }
            }
        }
        else if (resource->lastUsedTask != &taskNode)
        {
            ResourceRef &lastUsedRef = *(resource->lastUsedRef);
            if (lastUsedRef.crossQueueRef == &ref) // cross queue
            {
                // std::cout << "  CROSS QUEUE\n";
                waitDstStageMask |= ref.stageMask;
                waitSemaphoreValue = std::max(waitSemaphoreValue, resource->lastUsedTask->order);
                if (ref.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
                {
                    if (resource->resourceType == IMAGE_RESOURCE)
                    {
                        //  std::cout << "  CROSS QUEUE IMAGE RECEIVE\n";
                        ImageResource *imageResource = (ImageResource *)resource.get();
                        imageMemoryBarriers.emplace_back();
                        VkImageMemoryBarrier2 &barrier = imageMemoryBarriers.back();
                        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                        barrier.pNext = nullptr;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        barrier.srcAccessMask = VK_ACCESS_NONE;
                        barrier.dstStageMask = ref.stageMask;
                        barrier.dstAccessMask = ref.accessMask;
                        barrier.oldLayout = lastUsedRef.imageLayout;
                        barrier.newLayout = ref.imageLayout;
                        barrier.srcQueueFamilyIndex = queueFamilies[resource->lastUsedTask->actualTaskType];
                        barrier.dstQueueFamilyIndex = queueFamilies[taskNode.actualTaskType];
                        barrier.image = imageResource->image;
                        barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                        barrier.subresourceRange.baseMipLevel = 0;
                        barrier.subresourceRange.levelCount = 1;
                        barrier.subresourceRange.baseArrayLayer = 0;
                        barrier.subresourceRange.layerCount = 1;
                    }
                    // else
                    // {
                    //     //  std::cout << "  CROSS QUEUE BUFFER RECEIVE\n";
                    //     BufferResource *bufferResource = (BufferResource *)resource.get();
                    //     VkBufferMemoryBarrier barrier{};
                    //     barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    //     barrier.srcAccessMask = VK_ACCESS_NONE;
                    //     barrier.dstAccessMask = ref.accessMask;
                    //     barrier.srcQueueFamilyIndex = queueFamilies[resource->lastUsedTask->actualTaskType];
                    //     barrier.dstQueueFamilyIndex = queueFamilies[taskNode.actualTaskType];
                    //     barrier.buffer = bufferResource->info.buffer;
                    //     barrier.offset = bufferResource->info.offset;
                    //     barrier.size = bufferResource->info.range;

                    //     vkCmdPipelineBarrier(
                    //         commandBuffer,
                    //         // lastUsedRef.stageMask,
                    //         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    //         ref.stageMask,
                    //         0,
                    //         0, nullptr,
                    //         1, &barrier,
                    //         0, nullptr);
                    // }
                }
            }
            else
            {
                //  std::cout << "  SAME QUEUE\n";
                if ((ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE) ||
                    (resource->lastUsedRef->storeOp == VK_ATTACHMENT_STORE_OP_STORE) ||
                    ((resource->resourceType == IMAGE_RESOURCE) && resource->lastUsedRef->imageLayout != ref.imageLayout)) // TODO 先读后写不需要barrier，只需要event
                {
                    //  std::cout << "  NEED SYNC\n";
                    if (resource->resourceType == IMAGE_RESOURCE)
                    {
                        //  std::cout << "  IMAGE SYNC\n";
                        ImageResource *imageResource = (ImageResource *)resource.get();
                        imageMemoryBarriers.emplace_back();
                        VkImageMemoryBarrier2 &barrier = imageMemoryBarriers.back();
                        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                        barrier.pNext = nullptr;
                        barrier.srcStageMask = lastUsedRef.stageMask;
                        barrier.srcAccessMask = lastUsedRef.accessMask;
                        barrier.dstStageMask = ref.stageMask;
                        barrier.dstAccessMask = ref.accessMask;
                        barrier.oldLayout = lastUsedRef.imageLayout;
                        barrier.newLayout = ref.imageLayout;
                        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.image = imageResource->image;
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
                        barrier.srcStageMask = lastUsedRef.stageMask;
                        barrier.srcAccessMask = lastUsedRef.accessMask;
                        barrier.dstStageMask = ref.stageMask;
                        barrier.dstAccessMask = ref.accessMask;
                        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.buffer = bufferResource->info.buffer;
                        barrier.offset = bufferResource->info.offset;
                        barrier.size = bufferResource->info.range;
                    }
                }
            }
        }
        resource->lastUsedRef = &ref;
        resource->lastUsedTask = &taskNode;
    }

    void FrameGraph::endResourcesUse(VkCommandBuffer commandBuffer, ResourceRef &ref, TaskNode &taskNode,
                                     std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                                     std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers)
    {
        ResourceNode &resourceNode = *resourceNodes[ref.outResourceNodeID == 0 ? ref.inResourceNodeID : ref.outResourceNodeID]; // first use out
        auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];
        //  std::cout << "----END RESOURCE " << "TASKID " << taskNode.taskID << " RESOURCE_NODE_ID " << resourceNode.resourceNodeID << " RESOURCEID " << resourceNode.resourceID << "\n";
        if (ref.crossQueueRef != nullptr)
        {
            // std::cout << "  CROSS QUEUE\n";
            if (ref.crossQueueRef->loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
            {
                if (resource->resourceType == IMAGE_RESOURCE)
                {
                    // std::cout << "  CROSS QUEUE IMAGE RELEASE\n";
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
                    barrier.newLayout = ref.imageLayout;
                    barrier.srcQueueFamilyIndex = queueFamilies[taskNode.actualTaskType];
                    barrier.dstQueueFamilyIndex = queueFamilies[ref.crossQueueTask->actualTaskType];
                    barrier.image = imageResource->image;
                    barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                }
                // else
                // {
                //     // std::cout << "  CROSS QUEUE BUFFER RELEASE\n";
                //     BufferResource *bufferResource = (BufferResource *)resource.get();
                //     VkBufferMemoryBarrier barrier{};
                //     barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                //     barrier.srcAccessMask = ref.accessMask;
                //     barrier.dstAccessMask = VK_ACCESS_NONE;
                //     barrier.srcQueueFamilyIndex = queueFamilies[taskNode.actualTaskType];
                //     barrier.dstQueueFamilyIndex = queueFamilies[ref.crossQueueTask->actualTaskType];
                //     barrier.buffer = bufferResource->info.buffer;
                //     barrier.offset = bufferResource->info.offset;
                //     barrier.size = bufferResource->info.range;
                //     vkCmdPipelineBarrier(
                //         commandBuffer,
                //         ref.stageMask,
                //         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                //         // ref.crossQueueRef->stageMask,
                //         0,
                //         0, nullptr,
                //         1, &barrier,
                //         0, nullptr);
                // }
            }
        }
        else if (!resourceNode.isTransient && resourceNode.dstTaskIDs.size() == 0) // TODO final op 如何判断是最后一个？尤其是最后一个task是读的情况，此时没有任何对应的resourceNode.dstTaskIDs.size() == 0
        {
            PermanentResourceState &state = permanentResourceStates[resourceNode.resourceID];
            if (resource->resourceType == IMAGE_RESOURCE &&
                state.enImageLayout.has_value() &&
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
                barrier.image = imageResource->image;
                barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
            }
        }
    }

    void FrameGraph::Execute(uint32_t currentFrame, uint32_t imageIndex)
    {
        VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
        VkCommandBuffer commandBuffers[3] = {nullptr, nullptr, nullptr};

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional
        for (int i = 0; i < 3; i++)
        {
            if (haveQueue[i] && (submitCnts[i] > 0))
            {
                if (i > 0)
                {
                    vkWaitForFences(logicalDevice, 1, &(fences[i - 1][currentFrame]), VK_TRUE, UINT64_MAX);
                    vkResetFences(logicalDevice, 1, &(fences[i - 1][currentFrame]));
                }
                CommandPool *commandPool = commandPools[i][currentFrame].get();
                commandPool->Reset();
                VkCommandBuffer commandBuffer = commandPool->AllocateCommandBuffer();
                commandBuffers[i] = commandBuffer;
                vkBeginCommandBuffer(commandBuffer, &beginInfo);
            }
        }

        // std::cout << "---------------------EXE-------------------\n";
        uint32_t currentSubmitCnts[3] = {0, 0, 0};
        uint64_t waitSemaphoreValue = 0;
        VkPipelineStageFlags waitDstStageMask = 0;
        std::vector<VkBufferMemoryBarrier2> bufferMemoryBarriers;
        std::vector<VkImageMemoryBarrier2> imageMemoryBarriers;
        VkDependencyInfo dependencyInfo{};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.pNext = nullptr;

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.pNext = nullptr;
        commandBufferSubmitInfo.deviceMask = 0;

        VkSemaphoreSubmitInfo waitSemaphoreInfo{};
        waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        waitSemaphoreInfo.pNext = nullptr;
        waitSemaphoreInfo.semaphore = timelineSemaphore;
        waitSemaphoreInfo.deviceIndex = 0;

        VkSemaphoreSubmitInfo signalSemaphoreInfo{};
        signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalSemaphoreInfo.pNext = nullptr;
        signalSemaphoreInfo.semaphore = timelineSemaphore;
        signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        signalSemaphoreInfo.deviceIndex = 0;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.pNext = nullptr; //&tlsSubmitInfo;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

        for (auto taskID : orderedTasks)
        {
            TaskNode &taskNode = *taskNodes[taskID];
            TaskType actualTaskType = taskNode.actualTaskType;
            VkCommandBuffer commandBuffer = commandBuffers[actualTaskType];
            // std::cout << "-----------TASK " << taskID << " TYPE " << taskNode.taskType << " ACTUAL " << actualTaskType << "\n";
            bufferMemoryBarriers.clear();
            imageMemoryBarriers.clear();
            for (auto &ref : taskNode.resourceRefs)
                syncResources(commandBuffer, ref, taskNode, bufferMemoryBarriers, imageMemoryBarriers,
                              waitSemaphoreValue, waitDstStageMask);
            dependencyInfo.bufferMemoryBarrierCount = bufferMemoryBarriers.size();
            dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();
            dependencyInfo.imageMemoryBarrierCount = imageMemoryBarriers.size();
            dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();
            vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

            taskNode.executeCallback(taskNode, *this, commandBuffer, currentFrame, imageIndex);

            bufferMemoryBarriers.clear();
            imageMemoryBarriers.clear();
            for (auto &ref : taskNode.resourceRefs)
                endResourcesUse(commandBuffer, ref, taskNode, bufferMemoryBarriers, imageMemoryBarriers);
            dependencyInfo.bufferMemoryBarrierCount = bufferMemoryBarriers.size();
            dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();
            dependencyInfo.imageMemoryBarrierCount = imageMemoryBarriers.size();
            dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();
            vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

            if (taskNode.needQueueSubmit)
            {
                vkEndCommandBuffer(commandBuffer);

                commandBufferSubmitInfo.commandBuffer = commandBuffer;

                if (waitSemaphoreValue > 0)
                    waitSemaphoreValue += timelineSemaphoreBase;
                else if (currentSubmitCnts[actualTaskType] == 0)
                {
                    waitSemaphoreValue = lastTimelineValue;
                    waitDstStageMask = actualTaskType == RENDER_TASK ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                                                     : (actualTaskType == COMPUTE_TASK ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT);
                }

                if (waitSemaphoreValue > 0)
                {
                    waitSemaphoreInfo.value = waitSemaphoreValue;
                    waitSemaphoreInfo.stageMask = waitDstStageMask;
                    submitInfo.waitSemaphoreInfoCount = 1;
                    // std::cout << "------WILL WAIT " << waitSemaphoreValue << "\n";
                }
                else
                {
                    submitInfo.waitSemaphoreInfoCount = 0;
                }

                uint64_t signalSemaphoreValue = taskNode.order + timelineSemaphoreBase;

                signalSemaphoreInfo.value = signalSemaphoreValue;

                // std::cout << "------TASK " << taskID << " SUBMIT TO QUEUE " << gpuQueues[actualTaskType] << " SIGNAL " << signalSemaphoreValue << "\n";
                lastTimelineValue = signalSemaphoreValue;
                ++currentSubmitCnts[actualTaskType];

                VkFence fence = (actualTaskType != RENDER_TASK && currentSubmitCnts[actualTaskType] == submitCnts[actualTaskType])
                                    ? fences[actualTaskType - 1][currentFrame]
                                    : VK_NULL_HANDLE;
                vkQueueSubmit2(gpuQueues[actualTaskType], 1, &submitInfo, fence);

                if (currentSubmitCnts[actualTaskType] < submitCnts[actualTaskType])
                {
                    commandBuffers[actualTaskType] = commandPools[actualTaskType][currentFrame]->AllocateCommandBuffer();
                    vkBeginCommandBuffer(commandBuffers[actualTaskType], &beginInfo);
                }

                waitSemaphoreValue = 0;
                waitDstStageMask = 0;
            }
        }
        timelineSemaphoreBase += orderedTasks.size();
        clearLastUsedInfo();
    }
}