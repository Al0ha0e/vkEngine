#ifndef COMMAND_POOL_H
#define COMMAND_POOL_H
#include <render/environment.hpp>
#include <deque>

namespace vke_render
{
    class CommandPool
    {
    public:
        std::vector<VkCommandBuffer> commandBuffers;
        uint32_t allocatedCnt;

        CommandPool() : allocatedCnt(0) {}

        virtual ~CommandPool() {}

        CommandPool &operator=(const CommandPool &) = delete;
        CommandPool(const CommandPool &) = delete;

        CommandPool &operator=(CommandPool &&ano)
        {
            if (this != &ano)
            {
                commandBuffers = std::move(ano.commandBuffers);
                allocatedCnt = ano.allocatedCnt;
                ano.allocatedCnt = 0;
            }
            return *this;
        }

        CommandPool(CommandPool &&ano) : allocatedCnt(ano.allocatedCnt), commandBuffers(std::move(ano.commandBuffers))
        {
            ano.allocatedCnt = 0;
        }

        virtual VkCommandBuffer AllocateCommandBuffer()
        {
            if (allocatedCnt == commandBuffers.size())
                PreAllocateCommandBuffer(1);
            return commandBuffers[allocatedCnt++];
        }

        virtual VkCommandBuffer AllocateAndBegin(const VkCommandBufferBeginInfo *beginInfo) = 0;

        virtual void PreAllocateCommandBuffer(uint32_t cnt) = 0;

        virtual void Reset() = 0;
    };

    class GPUCommandPool : public CommandPool
    {
    public:
        GPUCommandPool() : commandPool(nullptr), CommandPool() {}

        GPUCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) : CommandPool()
        {
            VkCommandPoolCreateInfo commandPoolCreateInfo{};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCreateInfo.flags = flags;
            commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
            vkCreateCommandPool(RenderEnvironment::GetInstance()->logicalDevice,
                                &commandPoolCreateInfo, nullptr, &commandPool);
        }

        virtual ~GPUCommandPool() override
        {
            if (commandPool != nullptr)
            {
                VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
                if (commandBuffers.size() > 0)
                    vkFreeCommandBuffers(logicalDevice, commandPool, commandBuffers.size(), commandBuffers.data());
                vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
            }
        }

        GPUCommandPool &operator=(GPUCommandPool &&ano)
        {
            CommandPool::operator=(std::move(ano));
            if (this != &ano)
            {
                if (commandPool != nullptr)
                    vkDestroyCommandPool(RenderEnvironment::GetInstance()->logicalDevice, commandPool, nullptr);
                commandPool = ano.commandPool;
                ano.commandPool = nullptr;
            }
            return *this;
        }

        GPUCommandPool(GPUCommandPool &&ano) : CommandPool(std::move(ano)), commandPool(ano.commandPool)
        {
            ano.commandPool = nullptr;
        }

        virtual VkCommandBuffer AllocateAndBegin(const VkCommandBufferBeginInfo *beginInfo) override
        {
            VkCommandBuffer ret = AllocateCommandBuffer();
            vkBeginCommandBuffer(ret, beginInfo);
            return ret;
        }

        virtual void PreAllocateCommandBuffer(uint32_t cnt) override
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

        virtual void Reset() override
        {
            vkResetCommandPool(RenderEnvironment::GetInstance()->logicalDevice, commandPool, 0);
            allocatedCnt = 0;
        }

    private:
        VkCommandPool commandPool;
    };

    class CPUCommandPool : public CommandPool
    {
    public:
        CPUCommandPool() : CommandPool() {}

        CPUCommandPool &operator=(CPUCommandPool &&ano)
        {
            CommandPool::operator=(std::move(ano));
            if (this != &ano)
                callbackPool = std::move(ano.callbackPool);
            return *this;
        }

        CPUCommandPool(CPUCommandPool &&ano) : CommandPool(std::move(ano)), callbackPool(std::move(ano.callbackPool)) {}

        virtual VkCommandBuffer AllocateAndBegin(const VkCommandBufferBeginInfo *beginInfo = nullptr) override
        {
            return AllocateCommandBuffer();
        }

        virtual void PreAllocateCommandBuffer(uint32_t cnt) override
        {
            if (cnt + allocatedCnt > commandBuffers.size())
            {
                uint32_t prevSize = commandBuffers.size();
                commandBuffers.resize(cnt + allocatedCnt);
                callbackPool.resize(cnt + allocatedCnt);

                for (int i = prevSize; i < commandBuffers.size(); ++i)
                    commandBuffers[i] = (VkCommandBuffer)&callbackPool.at(i);
            }
        }

        virtual void Reset() override
        {
            allocatedCnt = 0;
        }

    private:
        std::deque<std::function<void()>> callbackPool;
    };
}

#endif