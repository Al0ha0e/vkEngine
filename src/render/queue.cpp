#include <render/queue.hpp>
#include <logger.hpp>

namespace vke_render
{

    void CPUCommandQueue::Submit(uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence)
    {
        std::shared_ptr<uint32_t> submitCnt = std::make_shared<uint32_t>(submitCount);

        std::lock_guard lock(queueMutex);
        for (int i = 0; i < submitCount; i++)
        {
            const VkSubmitInfo2 &submitInfo = pSubmits[i];

            std::shared_ptr<SubmitInfo> localSubmitInfo = std::make_shared<SubmitInfo>(submitInfo.signalSemaphoreInfoCount,
                                                                                       (std::function<void()> *)(submitInfo.pCommandBufferInfos[0].commandBuffer));

            for (int j = 0; j < submitInfo.signalSemaphoreInfoCount; j++)
                localSubmitInfo->signalSemaphoreInfos[j] = submitInfo.pSignalSemaphoreInfos[j];

            auto callback = [this, localSubmitInfo, fence, submitCnt]()
            {
                (*(localSubmitInfo->callback))();

                for (auto &signalSemInfo : localSubmitInfo->signalSemaphoreInfos)
                {
                    VkSemaphoreSignalInfo signalInfo = {};
                    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
                    signalInfo.semaphore = signalSemInfo.semaphore;
                    signalInfo.value = signalSemInfo.value;
                    VKE_LOG_INFO("submitCnt {} CPU SIGNAL {} {}", (*submitCnt), (void *)(signalSemInfo.semaphore), signalSemInfo.value)
                    vkSignalSemaphore(device, &signalInfo);
                }

                if (fence != nullptr && --(*submitCnt) == 0)
                {
                    VKE_LOG_INFO("RESET FENCE {}", (void *)fence)
                    ((std::atomic<bool> *)fence)->store(true);
                }
            };
            std::shared_ptr<uint32_t> counter = std::make_unique<uint32_t>(submitInfo.waitSemaphoreInfoCount);
            for (int j = 0; j < submitInfo.waitSemaphoreInfoCount; j++)
            {
                const VkSemaphoreSubmitInfo &waitInfo = submitInfo.pWaitSemaphoreInfos[j];
                waitInfoMap.try_emplace(waitInfo.semaphore).first->second.emplace(waitInfo.value, counter, callback);
                // 多个等待同一组callback
            }
        }
        idle.store(false);
        idle.notify_one();
        queueCV.notify_one();
    }

    void CPUCommandQueue::process()
    {
        VkSemaphoreWaitInfo semWaitInfo{};
        semWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        semWaitInfo.flags = VK_SEMAPHORE_WAIT_ANY_BIT;

        std::vector<VkSemaphore> sems;
        std::vector<uint64_t> vals;
        std::vector<std::function<void()>> callbacks;

        std::unique_lock lock(queueMutex);
        while (running.load())
        {
            bool empty = true;
            for (auto &[semaphore, infoQueue] : waitInfoMap)
            {
                if (infoQueue.size() > 0)
                {
                    empty = false;
                    break;
                }
            }

            if (empty)
            {
                idle.store(true);
                idle.notify_one();
                queueCV.wait(lock);
                continue;
            }
            for (auto &[semaphore, infoQueue] : waitInfoMap)
            {
                for (auto &waitInfo : infoQueue)
                {
                    sems.push_back(semaphore);
                    vals.push_back(waitInfo.value);
                }
            }
            semWaitInfo.semaphoreCount = sems.size();
            semWaitInfo.pSemaphores = sems.data();
            semWaitInfo.pValues = vals.data();
            lock.unlock();

            if (vkWaitSemaphores(device, &semWaitInfo, CPU_QUEUE_WAIT_TIMEOUT) == VK_SUCCESS)
            {
                lock.lock();
                for (auto &[semaphore, infoQueue] : waitInfoMap)
                {
                    uint64_t currentVal;
                    if (vkGetSemaphoreCounterValue(device, semaphore, &currentVal) == VK_SUCCESS)
                    {
                        auto ub = infoQueue.upper_bound(currentVal);
                        for (auto it = infoQueue.begin(); it != ub; ++it)
                            if (--(*(it->counter)) == 0)
                                callbacks.push_back(it->callback);

                        infoQueue.erase(infoQueue.begin(), ub);
                    }
                }
                lock.unlock();

                for (auto &callback : callbacks)
                    callback();
            }

            callbacks.clear();
            sems.clear();
            vals.clear();
            lock.lock();
        }
    }
}