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

        vkCreateSemaphore(RenderEnvironment::GetInstance()->logicalDevice, &createInfo, nullptr, &timelineSemaphore);

        computeQueue = RenderEnvironment::GetInstance()->computeQueue;
        graphicsQueue = RenderEnvironment::GetInstance()->graphicsQueue;
        renderQueueFamily = RenderEnvironment::GetInstance()->queueFamilyIndices.graphicsAndComputeFamily.value();
        computeQueueFamily = RenderEnvironment::GetInstance()->queueFamilyIndices.computeOnlyFamily[0];

        asyncCompute = computeQueue != graphicsQueue;

        for (int i = 0; i < framesInFlight; i++)
            renderCommandPools.push_back(std::make_unique<CommandPool>(renderQueueFamily, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT));
        if (asyncCompute)
            for (int i = 0; i < framesInFlight; i++)
                computeCommandPools.push_back(std::make_unique<CommandPool>(computeQueueFamily, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT));
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

        if (resource->lastUsedTask != nullptr && resource->lastUsedTask->taskType != taskNode.taskType)
        {
            resource->lastUsedTask->needQueueSubmit = true;
            taskNode.needQueueSubmit = true;
            resource->lastUsedRef->crossQueueRef = &ref;
        }

        resource->lastUsedTask = &taskNode;
        resource->lastUsedRef = &ref;
    }

    void FrameGraph::Compile()
    {
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
                    ref.crossQueueRef = nullptr;
                    if (targetResources.find(resourceNodes[ref.outResourceNodeID]->resourceID) != targetResources.end())
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
                if (ref.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
                {
                    ref.crossQueueRef = nullptr;
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

        visited.clear();

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
                    if (visited.find(resourceNodeID) == visited.end())
                    {
                        visited.insert(resourceNodeID);
                        ResourceNode &resourceNode = *resourceNodes[resourceNodeID];
                        for (auto dstTaskID : resourceNode.dstTaskIDs)
                        {
                            TaskNode &dstNode = *taskNodes[dstTaskID];
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

        if (asyncCompute)
        {
            for (auto taskID : orderedTasks)
            {
                TaskNode &taskNode = *taskNodes[taskID];
                for (auto &ref : taskNode.resourceRefs)
                    checkCrossQueue(ref, taskNode);
            }
            clearLastUsedInfo();

            renderSubmitCnt = 0;
            computeSubmitCnt = 0;
            for (int i = orderedTasks.size() - 1; i >= 0; --i)
            {
                TaskNode &taskNode = *taskNodes[orderedTasks[i]];
                if (taskNode.taskType == RENDER_TASK)
                {
                    if (renderSubmitCnt == 0)
                    {
                        taskNode.needQueueSubmit = true;
                        ++renderSubmitCnt;
                    }
                    else if (taskNode.needQueueSubmit)
                        ++renderSubmitCnt;
                }
                else
                {
                    if (computeSubmitCnt == 0)
                    {
                        taskNode.needQueueSubmit = true;
                        ++computeSubmitCnt;
                    }
                    else if (taskNode.needQueueSubmit)
                        ++computeSubmitCnt;
                }
            }
        }
        else
        {
            taskNodes[orderedTasks[orderedTasks.size() - 1]]->needQueueSubmit = true;
            renderSubmitCnt = 1;
            computeSubmitCnt = 0;
        }

        for (int i = 0; i < framesInFlight; i++)
            renderCommandPools[i]->PreAllocateCommandBuffer(renderSubmitCnt);
        if (asyncCompute)
            for (int i = 0; i < framesInFlight; i++)
                computeCommandPools[i]->PreAllocateCommandBuffer(computeSubmitCnt);

        std::cout << "!!!!!!!!!!!!!!! " << orderedTasks.size() << "\n";
        for (auto taskID : orderedTasks)
            std::cout << "task id " << taskID << "\n";
        // exit(1);
    }

    void FrameGraph::syncResources(VkCommandBuffer commandBuffer, ResourceRef &ref, TaskNode &taskNode,
                                   uint64_t &waitSemaphoreValue, VkPipelineStageFlags &waitDstStageMask)
    {
        ResourceNode &resourceNode = *resourceNodes[ref.inResourceNodeID == 0 ? ref.outResourceNodeID : ref.inResourceNodeID];
        auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];
        if (resource->lastUsedRef == nullptr) // first use of resource
        {
            if (!resourceNode.isTransient)
            {
                PermanentResourceState &state = permanentResourceStates[resourceNode.resourceID];
                if (resource->resourceType == IMAGE_RESOURCE &&
                    state.stImageLayout.has_value() &&
                    state.stImageLayout.value() != ref.imageLayout)
                {
                    std::cout << "!!!!!!!!GG TASK " << taskNode.taskID << " rnode " << resourceNode.resourceNodeID << " resource " << resourceNode.resourceID << "\n";
                    ImageResource *imageResource = (ImageResource *)resource.get();
                    VkImageMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.srcAccessMask = VK_ACCESS_NONE;
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
                    vkCmdPipelineBarrier(
                        commandBuffer,
                        state.stStage,
                        ref.stageMask,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);
                }
            }
        }
        else if (resource->lastUsedTask != &taskNode)
        {
            std::cout << "BB " << resource->lastUsedTask << " " << &taskNode << "\n";
            ResourceRef &lastUsedRef = *(resource->lastUsedRef);
            if (lastUsedRef.crossQueueRef == &ref) // cross queue
            {
                waitDstStageMask |= ref.stageMask;
                waitSemaphoreValue = std::max(waitSemaphoreValue, resource->lastUsedTask->order);
                if (ref.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
                {
                    if (resource->resourceType == IMAGE_RESOURCE)
                    {
                        ImageResource *imageResource = (ImageResource *)resource.get();
                        VkImageMemoryBarrier barrier{};
                        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        barrier.srcAccessMask = VK_ACCESS_NONE;
                        barrier.dstAccessMask = ref.accessMask;
                        barrier.oldLayout = lastUsedRef.imageLayout;
                        barrier.newLayout = ref.imageLayout;
                        barrier.srcQueueFamilyIndex = taskNode.taskType == RENDER_TASK ? computeQueueFamily : renderQueueFamily;
                        barrier.dstQueueFamilyIndex = taskNode.taskType == RENDER_TASK ? renderQueueFamily : computeQueueFamily;
                        barrier.image = imageResource->image;
                        barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                        barrier.subresourceRange.baseMipLevel = 0;
                        barrier.subresourceRange.levelCount = 1;
                        barrier.subresourceRange.baseArrayLayer = 0;
                        barrier.subresourceRange.layerCount = 1;

                        vkCmdPipelineBarrier(
                            commandBuffer,
                            lastUsedRef.stageMask,
                            ref.stageMask,
                            0,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);
                    }
                    else
                    {
                        BufferResource *bufferResource = (BufferResource *)resource.get();
                        VkBufferMemoryBarrier barrier{};
                        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                        barrier.srcAccessMask = VK_ACCESS_NONE;
                        barrier.dstAccessMask = ref.accessMask;
                        barrier.srcQueueFamilyIndex = taskNode.taskType == RENDER_TASK ? computeQueueFamily : renderQueueFamily;
                        barrier.dstQueueFamilyIndex = taskNode.taskType == RENDER_TASK ? renderQueueFamily : computeQueueFamily;
                        barrier.buffer = bufferResource->info.buffer;
                        barrier.offset = bufferResource->info.offset;
                        barrier.size = bufferResource->info.range;

                        vkCmdPipelineBarrier(
                            commandBuffer,
                            lastUsedRef.stageMask,
                            ref.stageMask,
                            0,
                            0, nullptr,
                            1, &barrier,
                            0, nullptr);
                    }
                }
            }
            else
            {
                std::cout << "!!!!!!!!!!!!0\n";
                if ((ref.storeOp == VK_ATTACHMENT_STORE_OP_STORE) ||
                    (resource->lastUsedRef->storeOp == VK_ATTACHMENT_STORE_OP_STORE) ||
                    ((resource->resourceType == IMAGE_RESOURCE) && resource->lastUsedRef->imageLayout != ref.imageLayout)) // TODO 先读后写不需要barrier，只需要event
                {
                    std::cout << "!!!!!!!!!!!!1\n";
                    if (resource->resourceType == IMAGE_RESOURCE)
                    {
                        std::cout << "!!!!!!!!MM TASK " << taskNode.taskID << " rnode " << resourceNode.resourceNodeID << " resource " << resourceNode.resourceID << "\n";
                        std::cout << "src acc " << lastUsedRef.accessMask << " dst acc " << ref.accessMask << " src stg " << lastUsedRef.stageMask << " dst stg " << ref.stageMask << "\n";
                        ImageResource *imageResource = (ImageResource *)resource.get();
                        VkImageMemoryBarrier barrier{};
                        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        barrier.srcAccessMask = lastUsedRef.accessMask;
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

                        vkCmdPipelineBarrier(
                            commandBuffer,
                            lastUsedRef.stageMask,
                            ref.stageMask,
                            0,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);
                    }
                    else
                    {
                        BufferResource *bufferResource = (BufferResource *)resource.get();
                        VkBufferMemoryBarrier barrier{};
                        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                        barrier.srcAccessMask = lastUsedRef.accessMask;
                        barrier.dstAccessMask = ref.accessMask;
                        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.buffer = bufferResource->info.buffer;
                        barrier.offset = bufferResource->info.offset;
                        barrier.size = bufferResource->info.range;

                        vkCmdPipelineBarrier(
                            commandBuffer,
                            lastUsedRef.stageMask,
                            ref.stageMask,
                            0,
                            0, nullptr,
                            1, &barrier,
                            0, nullptr);
                    }
                }
            }
        }
        resource->lastUsedRef = &ref;
        resource->lastUsedTask = &taskNode;
    }

    void FrameGraph::endResourcesUse(VkCommandBuffer commandBuffer, ResourceRef &ref, TaskNode &taskNode, bool &willSignal)
    {
        ResourceNode &resourceNode = *resourceNodes[ref.outResourceNodeID == 0 ? ref.inResourceNodeID : ref.outResourceNodeID]; // first use out
        auto &resource = resourceNode.isTransient ? transientResources[resourceNode.resourceID] : permanentResources[resourceNode.resourceID];
        // resource->lastUsedRef = &ref;
        // resource->lastUsedTask = &taskNode;
        if (ref.crossQueueRef != nullptr)
        {
            willSignal = true;
            if (ref.crossQueueRef->loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
            {
                if (resource->resourceType == IMAGE_RESOURCE)
                {
                    ImageResource *imageResource = (ImageResource *)resource.get();
                    VkImageMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.srcAccessMask = ref.accessMask;
                    barrier.dstAccessMask = VK_ACCESS_NONE;
                    barrier.oldLayout = ref.imageLayout;
                    barrier.newLayout = ref.imageLayout;
                    barrier.srcQueueFamilyIndex = taskNode.taskType == RENDER_TASK ? renderQueueFamily : computeQueueFamily;
                    barrier.dstQueueFamilyIndex = taskNode.taskType == RENDER_TASK ? computeQueueFamily : renderQueueFamily;
                    barrier.image = imageResource->image;
                    barrier.subresourceRange.aspectMask = imageResource->aspectMask;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;

                    vkCmdPipelineBarrier(
                        commandBuffer,
                        ref.stageMask,
                        ref.crossQueueRef->stageMask,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);
                }
                else
                {
                    BufferResource *bufferResource = (BufferResource *)resource.get();
                    VkBufferMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    barrier.srcAccessMask = ref.accessMask;
                    barrier.dstAccessMask = VK_ACCESS_NONE;
                    barrier.srcQueueFamilyIndex = taskNode.taskType == RENDER_TASK ? renderQueueFamily : computeQueueFamily;
                    barrier.dstQueueFamilyIndex = taskNode.taskType == RENDER_TASK ? computeQueueFamily : renderQueueFamily;
                    barrier.buffer = bufferResource->info.buffer;
                    barrier.offset = bufferResource->info.offset;
                    barrier.size = bufferResource->info.range;

                    vkCmdPipelineBarrier(
                        commandBuffer,
                        ref.stageMask,
                        ref.crossQueueRef->stageMask,
                        0,
                        0, nullptr,
                        1, &barrier,
                        0, nullptr);
                }
            }
        }
        else if (!resourceNode.isTransient && resourceNode.dstTaskIDs.size() == 0) // TODO final op 如何判断是最后一个？尤其是最后一个task是读的情况，此时没有任何对应的resourceNode.dstTaskIDs.size() == 0
        {
            PermanentResourceState &state = permanentResourceStates[resourceNode.resourceID];
            if (resource->resourceType == IMAGE_RESOURCE &&
                state.enImageLayout.has_value() &&
                state.enImageLayout.value() != ref.imageLayout)
            {
                ImageResource *imageResource = (ImageResource *)resource.get();
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.srcAccessMask = ref.accessMask;
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

                vkCmdPipelineBarrier(
                    commandBuffer,
                    ref.stageMask,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
            }
        }
    }

    void FrameGraph::Execute(uint32_t currentFrame, uint32_t imageIndex)
    {
        CommandPool *renderCommandPool = renderCommandPools[currentFrame].get();
        renderCommandPool->Reset();
        VkCommandBuffer renderCommandBuffer = renderCommandPool->AllocateCommandBuffer();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional
        vkBeginCommandBuffer(renderCommandBuffer, &beginInfo);

        CommandPool *computeCommandPool = nullptr;
        VkCommandBuffer computeCommandBuffer = nullptr;

        bool needAsyncCompute = asyncCompute && (computeSubmitCnt > 0);
        if (needAsyncCompute)
        {
            computeCommandPool = computeCommandPools[currentFrame].get();
            computeCommandPool->Reset();
            computeCommandBuffer = computeCommandPool->AllocateCommandBuffer();
            vkBeginCommandBuffer(computeCommandBuffer, &beginInfo);
        }

        uint32_t rSubmitCnt = 0;
        uint32_t cSubmitCnt = 0;

        std::cout << "---------------------EXE-------------------\n";

        for (auto taskID : orderedTasks)
        {
            uint64_t waitSemaphoreValue = 0;
            VkPipelineStageFlags waitDstStageMask = 0;
            TaskNode &taskNode = *taskNodes[taskID];
            needAsyncCompute = asyncCompute && taskNode.taskType == COMPUTE_TASK;
            VkCommandBuffer commandBuffer = needAsyncCompute ? computeCommandBuffer : renderCommandBuffer;

            for (auto &ref : taskNode.resourceRefs)
                syncResources(commandBuffer, ref, taskNode, waitSemaphoreValue, waitDstStageMask);

            taskNode.executeCallback(taskNode, *this, commandBuffer, currentFrame, imageIndex);

            bool willSignal = false;
            for (auto &ref : taskNode.resourceRefs)
                endResourcesUse(commandBuffer, ref, taskNode, willSignal);

            if (taskNode.needQueueSubmit)
            {
                vkEndCommandBuffer(commandBuffer);

                VkTimelineSemaphoreSubmitInfo tlsSubmitInfo{};
                tlsSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.pNext = &tlsSubmitInfo;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffer;

                if (waitSemaphoreValue > 0)
                {
                    waitSemaphoreValue += timelineSemaphoreBase;
                    tlsSubmitInfo.waitSemaphoreValueCount = 1;
                    tlsSubmitInfo.pWaitSemaphoreValues = &waitSemaphoreValue;
                    submitInfo.waitSemaphoreCount = 1;
                    submitInfo.pWaitSemaphores = &timelineSemaphore;
                    submitInfo.pWaitDstStageMask = &waitDstStageMask;
                }
                else
                {
                    tlsSubmitInfo.waitSemaphoreValueCount = 0;
                    submitInfo.waitSemaphoreCount = 0;
                }

                uint64_t signalSemaphoreValue = taskNode.order + timelineSemaphoreBase;
                if (willSignal)
                {
                    tlsSubmitInfo.signalSemaphoreValueCount = 1;
                    tlsSubmitInfo.pSignalSemaphoreValues = &signalSemaphoreValue;
                    submitInfo.signalSemaphoreCount = 1;
                    submitInfo.pSignalSemaphores = &timelineSemaphore;
                }
                else
                {
                    tlsSubmitInfo.signalSemaphoreValueCount = 0;
                    submitInfo.signalSemaphoreCount = 0;
                }

                vkQueueSubmit(needAsyncCompute ? computeQueue : graphicsQueue, 1, &submitInfo, nullptr);

                if (needAsyncCompute)
                {
                    ++cSubmitCnt;
                    if (cSubmitCnt < computeSubmitCnt)
                    {
                        computeCommandBuffer = computeCommandPool->AllocateCommandBuffer();
                        vkBeginCommandBuffer(computeCommandBuffer, &beginInfo);
                    }
                }
                else
                {
                    ++rSubmitCnt;
                    if (rSubmitCnt < renderSubmitCnt)
                    {
                        renderCommandBuffer = renderCommandPool->AllocateCommandBuffer();
                        vkBeginCommandBuffer(renderCommandBuffer, &beginInfo);
                    }
                }
            }

            timelineSemaphoreBase += orderedTasks.size();
        }
        clearLastUsedInfo();
    }
}