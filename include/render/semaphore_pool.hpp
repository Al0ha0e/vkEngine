#ifndef SEMAPHORE_POOL_H
#define SEMAPHORE_POOL_H

#include <render/environment.hpp>
#include <algorithm>

namespace vke_render
{
    class SemaphorePool
    {
    public:
        struct SemaphoreState
        {
            VkSemaphore semaphore;
            uint64_t value;

            SemaphoreState() : semaphore(nullptr), value(0) {}
        };

        std::vector<SemaphoreState> semaphores;
        uint32_t allocatedCnt;

        SemaphorePool() : allocatedCnt(0) {}
        ~SemaphorePool()
        {
            for (auto &semaphore : semaphores)
                vkDestroySemaphore(globalLogicalDevice, semaphore.semaphore, nullptr);
        }
        SemaphorePool &operator=(const SemaphorePool &) = delete;
        SemaphorePool(const SemaphorePool &) = delete;

        SemaphorePool &operator=(SemaphorePool &&ano)
        {
            if (this != &ano)
            {
                semaphores = std::move(ano.semaphores);
                allocatedCnt = ano.allocatedCnt;
                ano.allocatedCnt = 0;
            }
            return *this;
        }

        SemaphorePool(SemaphorePool &&ano) : allocatedCnt(ano.allocatedCnt), semaphores(std::move(ano.semaphores))
        {
            ano.allocatedCnt = 0;
        }

        VkSemaphore AllocateSemaphore(uint64_t &value)
        {
            if (allocatedCnt == semaphores.size())
                PreAllocateSemaphore(1);
            SemaphoreState &state = semaphores[allocatedCnt++];
            state.value += 1;
            value = state.value;
            return state.semaphore;
        }

        void SetValue(const VkSemaphore semaphore, const uint64_t value)
        {
            for (SemaphoreState &state : semaphores)
            {
                if (state.semaphore == semaphore)
                {
                    state.value = std::max(state.value, value);
                    return;
                }
            }
        }

        void PreAllocateSemaphore(uint32_t cnt)
        {
            VkSemaphoreTypeCreateInfo timelineInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
                .initialValue = 0};
            VkSemaphoreCreateInfo semaphoreCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = &timelineInfo};
            if (cnt + allocatedCnt > semaphores.size())
            {
                uint32_t prevSize = semaphores.size();
                semaphores.resize(cnt + allocatedCnt);
                for (int i = prevSize; i < semaphores.size(); i++)
                    vkCreateSemaphore(globalLogicalDevice, &semaphoreCreateInfo, nullptr, &semaphores[i].semaphore);
            }
        }

        void Reset()
        {
            allocatedCnt = 0;
        }
    };
}

#endif