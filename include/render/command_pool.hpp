#ifndef COMMAND_POOL_H
#define COMMAND_POOL_H
#include <render/environment.hpp>

namespace vke_render
{
    class CommandPool
    {
    public:
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
        uint32_t allocatedCnt;

        CommandPool() : commandPool(nullptr), allocatedCnt(0) {}

        CommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) : allocatedCnt(0)
        {
            VkCommandPoolCreateInfo commandPoolCreateInfo{};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCreateInfo.flags = flags;
            commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
            vkCreateCommandPool(RenderEnvironment::GetInstance()->logicalDevice,
                                &commandPoolCreateInfo, nullptr, &commandPool);
        }

        ~CommandPool()
        {
            if (commandPool != nullptr)
            {
                VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
                if (commandBuffers.size() > 0)
                    vkFreeCommandBuffers(logicalDevice, commandPool, commandBuffers.size(), commandBuffers.data());
                vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
            }
        }

        CommandPool &operator=(const CommandPool &) = delete;
        CommandPool(const CommandPool &) = delete;

        CommandPool &operator=(CommandPool &&ano)
        {
            if (this != &ano)
            {
                if (commandPool != nullptr)
                    vkDestroyCommandPool(RenderEnvironment::GetInstance()->logicalDevice, commandPool, nullptr);
                commandPool = ano.commandPool;
                commandBuffers = std::move(ano.commandBuffers);
                ano.commandPool = nullptr;
            }
            return *this;
        }

        CommandPool(CommandPool &&ano) : commandPool(ano.commandPool), commandBuffers(std::move(ano.commandBuffers))
        {
            ano.commandPool = nullptr;
        }

        void PreAllocateCommandBuffer(uint32_t cnt)
        {
            if (cnt + allocatedCnt > commandBuffers.size())
            {
                uint32_t prevSize = commandBuffers.size();
                commandBuffers.resize(cnt + allocatedCnt);
                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = commandPool;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = cnt + allocatedCnt - prevSize;
                vkAllocateCommandBuffers(RenderEnvironment::GetInstance()->logicalDevice, &allocInfo, commandBuffers.data() + prevSize);
            }
        }

        VkCommandBuffer AllocateCommandBuffer()
        {
            if (allocatedCnt == commandBuffers.size())
                PreAllocateCommandBuffer(1);
            return commandBuffers[allocatedCnt++];
        }

        void Reset()
        {
            vkResetCommandPool(RenderEnvironment::GetInstance()->logicalDevice, commandPool, 0);
            allocatedCnt = 0;
        }

    private:
    };
}

#endif