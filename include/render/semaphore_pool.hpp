#ifndef SEMAPHORE_POOL_H
#define SEMAPHORE_POOL_H

#include <render/environment.hpp>

namespace vke_render
{
    class SemaphorePool
    {
    public:
        std::vector<VkSemaphore> semaphores;
        uint32_t allocatedCnt;

        SemaphorePool() : allocatedCnt(0) {}
        ~SemaphorePool()
        {
            for (auto semaphore : semaphores)
                vkDestroySemaphore(globalLogicalDevice, semaphore, nullptr);
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

        VkSemaphore AllocateSemaphore()
        {
            if (allocatedCnt == semaphores.size())
                PreAllocateSemaphore(1);
            return semaphores[allocatedCnt++];
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
                    vkCreateSemaphore(globalLogicalDevice, &semaphoreCreateInfo, nullptr, semaphores.data() + i);
            }
        }

        void Reset()
        {
            allocatedCnt = 0;
        }
    };
}

#endif