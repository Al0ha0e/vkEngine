#ifndef QUEUE_H
#define QUEUE_H

#include <common.hpp>
#include <optional>
#include <vector>
#include <set>
#include <stdexcept>

namespace vke_render
{
    enum QueueType
    {
        GRAPHICS_QUEUE = 0,
        COMPUTE_QUEUE,
        TRANSFER_QUEUE,
        CPU_QUEUE
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsAndComputeFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeOnlyFamily;
        std::optional<uint32_t> transferOnlyFamily;
        std::vector<uint32_t> uniqueQueueFamilies;

        void clear()
        {
            graphicsAndComputeFamily.reset();
            presentFamily.reset();
            computeOnlyFamily.reset();
            transferOnlyFamily.reset();
            uniqueQueueFamilies.clear();
        }

        bool isComplete()
        {
            return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
        }

        void getUniqueQueueFamilies()
        {
            std::set<uint32_t> queueFamilySet = {graphicsAndComputeFamily.value(),
                                                 presentFamily.value()};
            if (computeOnlyFamily.has_value())
                queueFamilySet.insert(computeOnlyFamily.value());
            if (transferOnlyFamily.has_value())
                queueFamilySet.insert(transferOnlyFamily.value());
            for (auto &val : queueFamilySet)
                uniqueQueueFamilies.push_back(val);
        }
    };

    class CommandQueue
    {
    public:
        CommandQueue() {}
        virtual void Submit(uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence) = 0;
    };

    class GPUCommandQueue : public CommandQueue
    {
    public:
        VkQueue queue;
        GPUCommandQueue() {}
        GPUCommandQueue(VkQueue queue) : queue(queue) {}

        virtual void Submit(uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence) override
        {
            if (vkQueueSubmit2(queue, submitCount, pSubmits, fence) != VK_SUCCESS)
                throw std::runtime_error("vkQueueSubmit2 FAIL");
        }
    };

    class CPUCommandQueue : public CommandQueue
    {
    public:
        virtual void Submit(uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence) override
        {
        }

        void Start() {}
        void Stop() {}
    };
}

#endif