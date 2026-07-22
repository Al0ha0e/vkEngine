#include <render/frame_graph.hpp>
#include <logger.hpp>

namespace vke_render
{
#ifdef VKE_ENABLE_FRAME_GRAPH_PROFILING
    void FrameGraph::initProfiler()
    {
        RenderEnvironment *environment = RenderEnvironment::GetInstance();
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(environment->physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(environment->physicalDevice,
                                                 &queueFamilyCount, queueFamilyProperties.data());

        VkQueryPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        createInfo.queryCount = PROFILE_QUERY_POOL_SIZE;

        for (int taskType = RENDER_TASK; taskType < CPU_TASK; ++taskType)
        {
            if (!RenderEnvironment::HasQueue(QueueType(taskType)))
                continue;

            const uint32_t queueFamily = queueFamilies[taskType];
            if (queueFamily >= queueFamilyProperties.size())
                continue;

            profileTimestampValidBits[taskType] = queueFamilyProperties[queueFamily].timestampValidBits;
            if (profileTimestampValidBits[taskType] == 0)
            {
                VKE_LOG_WARN("FrameGraph profiling disabled for queue {}: timestamps are unsupported", taskType)
                continue;
            }

            for (uint32_t frame = 0; frame < framesInFlight; ++frame)
                VKE_VK_CHECK(vkCreateQueryPool(globalLogicalDevice, &createInfo, nullptr,
                                               &profileQueryPools[taskType][frame]),
                             "failed to create FrameGraph profile query pool")
        }
    }

    void FrameGraph::cleanupProfiler()
    {
        for (int taskType = RENDER_TASK; taskType < CPU_TASK; ++taskType)
            for (uint32_t frame = 0; frame < framesInFlight; ++frame)
                if (profileQueryPools[taskType][frame] != VK_NULL_HANDLE)
                    vkDestroyQueryPool(globalLogicalDevice, profileQueryPools[taskType][frame], nullptr);
    }

    double FrameGraph::timestampDeltaMs(const TaskType taskType, const uint64_t begin, const uint64_t end) const
    {
        const uint32_t validBits = profileTimestampValidBits[taskType];
        uint64_t delta;
        if (validBits >= 64)
            delta = end - begin;
        else
            delta = (end - begin) & ((uint64_t(1) << validBits) - 1);

        return double(delta) * double(RenderEnvironment::GetInstance()->physicalDeviceProperties.limits.timestampPeriod) / 1000000.0;
    }

    void FrameGraph::LogProfile(const uint32_t currentFrame)
    {
        FrameProfile &frameProfile = frameProfiles[currentFrame];
        if (!frameProfile.pending)
            return;

        VKE_LOG_INFO("[FrameGraph Profile] frame={} task_count={}", frameProfile.frameNumber, frameProfile.tasks.size())
        for (const TaskProfile &taskProfile : frameProfile.tasks)
        {
            if (!taskProfile.hasGpuTimestamps)
            {
                VKE_LOG_WARN("[FrameGraph Profile] task={} queue={} GPU timestamps unsupported cpu_record={:.3f} ms",
                             taskProfile.name, uint32_t(taskProfile.actualTaskType), taskProfile.cpuRecordMs)
                continue;
            }

            VkQueryPool queryPool = profileQueryPools[taskProfile.actualTaskType][currentFrame];
            uint64_t queryResults[PROFILE_QUERY_COUNT_PER_TASK * 2]{};
            VkResult result = vkGetQueryPoolResults(
                globalLogicalDevice, queryPool, taskProfile.queryIndex, PROFILE_QUERY_COUNT_PER_TASK,
                sizeof(queryResults), queryResults, sizeof(uint64_t) * 2,
                VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

            bool available = result == VK_SUCCESS;
            for (uint32_t query = 0; query < PROFILE_QUERY_COUNT_PER_TASK; ++query)
                available = available && queryResults[query * 2 + 1] != 0;

            if (!available)
            {
                VKE_LOG_WARN("[FrameGraph Profile] task={} queue={} GPU timestamps unavailable cpu_record={:.3f} ms",
                             taskProfile.name, uint32_t(taskProfile.actualTaskType), taskProfile.cpuRecordMs)
                continue;
            }

            const double preBarrierMs = taskProfile.hasPreBarrier
                                            ? timestampDeltaMs(taskProfile.actualTaskType, queryResults[0], queryResults[2])
                                            : 0.0;
            const double taskMs = timestampDeltaMs(taskProfile.actualTaskType, queryResults[2], queryResults[4]);
            const double postBarrierMs = taskProfile.hasPostBarrier
                                             ? timestampDeltaMs(taskProfile.actualTaskType, queryResults[4], queryResults[6])
                                             : 0.0;
            VKE_LOG_INFO("[FrameGraph Profile] task={} queue={} gpu_task={:.3f} ms pre_barrier={:.3f} ms post_barrier={:.3f} ms cpu_record={:.3f} ms",
                         taskProfile.name, uint32_t(taskProfile.actualTaskType), taskMs,
                         preBarrierMs, postBarrierMs, taskProfile.cpuRecordMs)
        }
        frameProfile.pending = false;
    }
#endif
}
