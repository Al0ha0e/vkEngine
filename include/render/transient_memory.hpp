#ifndef TRANSIENT_MEMORY_H
#define TRANSIENT_MEMORY_H

#include <render/environment.hpp>
#include <algorithm>
#include <limits>
#include <map>
#include <vector>

namespace vke_render
{
    struct TransientMemoryAllocation
    {
        uint32_t type;
        uint32_t idx;
        size_t offset;
        size_t size;
        TransientMemoryAllocation() : type(0), idx(0), offset(0), size(0) {}
        TransientMemoryAllocation(uint32_t type, uint32_t idx, size_t offset, size_t size)
            : type(type), idx(idx), offset(offset), size(size) {}
    };

    class TransientMemoryInfoPool
    {
    public:
        struct MemoryBlock
        {
            size_t offset;
            size_t size;

            MemoryBlock() : offset(0), size(0) {}
            MemoryBlock(size_t offset, size_t size) : offset(offset), size(size) {}

            size_t End() const { return offset + size; }
        };

        VkMemoryRequirements info;

        TransientMemoryInfoPool() : totalSize(0) { info = VkMemoryRequirements{}; }
        TransientMemoryInfoPool(const VkMemoryRequirements req) : info(req), totalSize(0) {}
        TransientMemoryInfoPool(const TransientMemoryInfoPool &) = delete;
        TransientMemoryInfoPool &operator=(const TransientMemoryInfoPool &) = delete;

        TransientMemoryInfoPool(TransientMemoryInfoPool &&ano) noexcept
            : info(ano.info),
              totalSize(ano.totalSize), freeBlocks(std::move(ano.freeBlocks)),
              allocatedBlocks(std::move(ano.allocatedBlocks))
        {
            ano.totalSize = 0;
            ano.info = VkMemoryRequirements{};
        }

        TransientMemoryInfoPool &operator=(TransientMemoryInfoPool &&ano) noexcept
        {
            if (this != &ano)
            {
                Reset();
                info = ano.info;
                totalSize = ano.totalSize;
                freeBlocks = std::move(ano.freeBlocks);
                allocatedBlocks = std::move(ano.allocatedBlocks);

                ano.totalSize = 0;
                ano.info = VkMemoryRequirements{};
            }
            return *this;
        }

        ~TransientMemoryInfoPool() = default;

        bool CompatibleWith(uint32_t memoryTypeBits) const { return (info.memoryTypeBits & memoryTypeBits) != 0; }

        void SimulateAlloc(const VkMemoryRequirements &req, TransientMemoryAllocation &allocation)
        {
            info.memoryTypeBits &= req.memoryTypeBits;
            info.alignment = std::max(info.alignment, req.alignment);

            const size_t alignment = std::max<size_t>(1, req.alignment);
            const size_t reqSize = req.size;
            size_t bestIdx = freeBlocks.size();
            size_t bestOffset = 0;
            size_t bestPadding = 0;
            size_t bestRemain = std::numeric_limits<size_t>::max();

            for (size_t i = 0; i < freeBlocks.size(); ++i)
            {
                const MemoryBlock &block = freeBlocks[i];
                const size_t alignedOffset = alignUp(block.offset, alignment);
                if (alignedOffset < block.offset)
                    continue;

                const size_t padding = alignedOffset - block.offset;
                if (padding > block.size)
                    continue;

                const size_t usableSize = block.size - padding;
                if (usableSize < reqSize)
                    continue;

                const size_t consumed = padding + reqSize;
                const size_t remain = block.size - consumed;
                if (remain < bestRemain)
                {
                    bestIdx = i;
                    bestOffset = alignedOffset;
                    bestPadding = padding;
                    bestRemain = remain;
                }
            }

            if (bestIdx != freeBlocks.size())
            {
                MemoryBlock block = freeBlocks[bestIdx];
                freeBlocks.erase(freeBlocks.begin() + bestIdx);

                if (bestPadding > 0)
                    insertFreeBlock(MemoryBlock(block.offset, bestPadding));

                const size_t suffixOffset = bestOffset + reqSize;
                const size_t suffixSize = block.End() - suffixOffset;
                if (suffixSize > 0)
                    insertFreeBlock(MemoryBlock(suffixOffset, suffixSize));

                allocation.offset = bestOffset;
                allocation.size = reqSize;
                allocatedBlocks[allocation.offset] = allocation.size;
                return;
            }

            const size_t offset = alignUp(totalSize, alignment);
            allocation.offset = offset;
            allocation.size = reqSize;
            totalSize = offset + reqSize;
            allocatedBlocks[allocation.offset] = allocation.size;
        }

        size_t SimulateDealloc(const TransientMemoryAllocation &allocation)
        {
            if (allocation.size == 0)
                return totalSize;

            auto it = allocatedBlocks.find(allocation.offset);
            if (it != allocatedBlocks.end() && it->second == allocation.size)
                allocatedBlocks.erase(it);

            MemoryBlock released(allocation.offset, allocation.size);
            insertFreeBlock(released);

            if (freeBlocks.empty())
                return totalSize;

            std::vector<MemoryBlock> mergedBlocks;
            mergedBlocks.reserve(freeBlocks.size());
            mergedBlocks.push_back(freeBlocks[0]);
            for (size_t i = 1; i < freeBlocks.size(); ++i)
            {
                MemoryBlock &back = mergedBlocks.back();
                const MemoryBlock &current = freeBlocks[i];
                if (back.End() >= current.offset)
                    back.size = std::max(back.End(), current.End()) - back.offset;
                else
                    mergedBlocks.push_back(current);
            }

            freeBlocks = std::move(mergedBlocks);
            return totalSize;
        }

        void Reset()
        {
            info = VkMemoryRequirements{};
            totalSize = 0;
            freeBlocks.clear();
            allocatedBlocks.clear();
        }

        const VkMemoryRequirements &GetMemoryRequirements() const { return info; }
        size_t GetTotalSize() const { return totalSize; }

    private:
        size_t totalSize;
        std::vector<MemoryBlock> freeBlocks;
        std::map<size_t, size_t> allocatedBlocks;

        static size_t alignUp(size_t value, size_t alignment)
        {
            if (alignment <= 1)
                return value;
            return ((value + alignment - 1) / alignment) * alignment;
        }

        void insertFreeBlock(const MemoryBlock &block)
        {
            auto it = freeBlocks.begin();
            while (it != freeBlocks.end() && it->offset < block.offset)
                ++it;
            freeBlocks.insert(it, block);
        }
    };

    template <uint32_t TYPE_CNT>
    class TransientMemorySimulator
    {
    public:
        TransientMemoryAllocation PreAllocMemory(uint32_t type, const VkMemoryRequirements &req)
        {
            VKE_LOG_DEBUG("TRY PREALLOC TTYPE {} MEMORY SIZE {} ALIGN {} MTYPE {}", type, req.size, req.alignment, req.memoryTypeBits)
            uint32_t idx;
            TransientMemoryAllocation ret;
            ret.type = type;
            auto &memoryInfoPools = allMemoryInfoPools[type];
            for (idx = 0; idx < memoryInfoPools.size(); ++idx)
            {
                TransientMemoryInfoPool &pool = memoryInfoPools[idx];
                if (!pool.CompatibleWith(req.memoryTypeBits))
                    continue;
                ret.idx = idx;
                pool.SimulateAlloc(req, ret);
                return ret;
            }
            ret.idx = idx;
            memoryInfoPools.emplace_back(req);
            memoryInfoPools[idx].SimulateAlloc(req, ret);
            return ret;
        }

        void PreDeallocMemory(const TransientMemoryAllocation &allocation)
        {
            allMemoryInfoPools[allocation.type][allocation.idx].SimulateDealloc(allocation);
        }

        void Reset()
        {
            for (int i = 0; i < TYPE_CNT; ++i)
                allMemoryInfoPools[i].clear();
        }

        const TransientMemoryInfoPool &GetPool(uint32_t type, uint32_t idx) const
        {
            return allMemoryInfoPools[type][idx];
        }

        size_t GetPoolCnt(uint32_t type) const { return allMemoryInfoPools[type].size(); }

    private:
        std::vector<TransientMemoryInfoPool> allMemoryInfoPools[TYPE_CNT];
    };

    template <uint32_t TYPE_CNT>
    class TransientMemoryManager
    {
    public:
        TransientMemoryManager() = default;
        ~TransientMemoryManager()
        {
            for (int i = 0; i < TYPE_CNT; ++i)
                for (uint32_t j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j)
                    DestroyFrame(i, j);
        }

        TransientMemoryManager(const TransientMemoryManager &) = delete;
        TransientMemoryManager &operator=(const TransientMemoryManager &) = delete;

        void Realloc(uint32_t type, uint32_t currentFrame, const TransientMemorySimulator<TYPE_CNT> &simulator)
        {
            DestroyFrame(type, currentFrame);

            auto &frameAllocations = poolAllocations[type][currentFrame];
            frameAllocations.resize(simulator.GetPoolCnt(type), nullptr);
            VKE_LOG_DEBUG("TRANSIENT POOL TTYPE {} FRAME {} POOL_CNT {}", type, currentFrame, simulator.GetPoolCnt(type))

            for (uint32_t i = 0; i < simulator.GetPoolCnt(type); ++i)
            {
                const TransientMemoryInfoPool &pool = simulator.GetPool(type, i);
                if (pool.GetTotalSize() == 0)
                    continue;

                VkMemoryRequirements req = pool.GetMemoryRequirements();
                req.size = pool.GetTotalSize();
                req.alignment = std::max<VkDeviceSize>(1, req.alignment);

                VKE_LOG_DEBUG("TRY ALLOCATE TRANSIENT MEMORY SIZE {} ALIGN {} MTYPE {}", req.size, req.alignment, req.memoryTypeBits)

                VmaAllocationCreateInfo allocationCreateInfo = {};
                allocationCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;
                allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

                VKE_VK_CHECK(vmaAllocateMemory(RenderEnvironment::GetInstance()->vmaAllocator, &req, &allocationCreateInfo, &frameAllocations[i], nullptr),
                             "failed to allocate transient memory!")
            }
        }

        void DestroyFrame(uint32_t type, uint32_t currentFrame)
        {
            auto &frameAllocations = poolAllocations[type][currentFrame];
            for (auto &allocation : frameAllocations)
            {
                if (allocation != nullptr)
                {
                    vmaFreeMemory(RenderEnvironment::GetInstance()->vmaAllocator, allocation);
                    allocation = nullptr;
                }
            }
            frameAllocations.clear();
        }

        VmaAllocation GetPoolVmaAllocation(uint32_t type, uint32_t currentFrame, uint32_t idx) const
        {
            return poolAllocations[type][currentFrame][idx];
        }

    private:
        std::vector<VmaAllocation> poolAllocations[TYPE_CNT][MAX_FRAMES_IN_FLIGHT];
    };
}

#endif
