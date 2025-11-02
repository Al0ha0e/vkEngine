#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <render/render_common.hpp>
#include <map>
#include <vector>
#include <stdexcept>

namespace vke_render
{

    const uint32_t DEFAULT_BINDLESS_CNT = 1024;

    void InitDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding &dst,
                                        uint32_t binding,
                                        VkDescriptorType descriptorType,
                                        uint32_t descriptorCount,
                                        VkShaderStageFlags stageFlags,
                                        const VkSampler *pImmutableSamplers);

    struct DescriptorSetInfo
    {
        VkDescriptorSetLayout layout;
        uint32_t variableDescriptorCnt;
        std::map<VkDescriptorType, uint32_t> descriptorCntMap;

        DescriptorSetInfo() : layout(nullptr), variableDescriptorCnt(0) {}

        DescriptorSetInfo(VkDescriptorSetLayout layout, uint32_t variableDescriptorCnt)
            : layout(layout),
              variableDescriptorCnt(variableDescriptorCnt) {}

        DescriptorSetInfo(const DescriptorSetInfo &) = delete;
        DescriptorSetInfo &operator=(const DescriptorSetInfo &) = delete;

        DescriptorSetInfo &operator=(DescriptorSetInfo &&ano)
        {
            if (this != &ano)
            {
                layout = ano.layout;
                variableDescriptorCnt = ano.variableDescriptorCnt;
                descriptorCntMap = std::move(ano.descriptorCntMap);
            }
            return *this;
        }

        DescriptorSetInfo(DescriptorSetInfo &&ano)
            : layout(ano.layout), variableDescriptorCnt(ano.variableDescriptorCnt), descriptorCntMap(std::move(ano.descriptorCntMap)) {}

        ~DescriptorSetInfo()
        {
            if (layout)
                vkDestroyDescriptorSetLayout(globalLogicalDevice, layout, nullptr);
        }

        void AddCnt(VkDescriptorType type, int cnt)
        {
            auto it = descriptorCntMap.find(type);
            if (it == descriptorCntMap.end())
            {
                descriptorCntMap[type] = cnt;
                return;
            }
            it->second += cnt;
        }

        void AddCnt(VkDescriptorSetLayoutBinding info)
        {
            AddCnt(info.descriptorType, info.descriptorCount);
        }
    };

    struct DescriptorSetPoolInfo
    {
        uint32_t setCnt;
        std::map<VkDescriptorType, uint32_t> descriptorCntMap;

        DescriptorSetPoolInfo() : setCnt(0) {}
        DescriptorSetPoolInfo(uint32_t setCnt, std::map<VkDescriptorType, uint32_t> &&descriptorCntMap)
            : setCnt(setCnt), descriptorCntMap(std::move(descriptorCntMap)) {}

        DescriptorSetPoolInfo(const DescriptorSetPoolInfo &) = delete;
        DescriptorSetPoolInfo &operator=(const DescriptorSetPoolInfo &) = delete;

        DescriptorSetPoolInfo &operator=(DescriptorSetPoolInfo &&ano)
        {
            if (this != &ano)
            {
                setCnt = ano.setCnt;
                descriptorCntMap = std::move(ano.descriptorCntMap);
            }
            return *this;
        }

        DescriptorSetPoolInfo(DescriptorSetPoolInfo &&ano)
            : setCnt(ano.setCnt), descriptorCntMap(std::move(ano.descriptorCntMap)) {}

        bool SuitableToAllocate(DescriptorSetInfo &info)
        {
            if (setCnt == 0)
                return false;
            for (auto &kv : info.descriptorCntMap)
            {
                auto it = descriptorCntMap.find(kv.first);
                if (it == descriptorCntMap.end() || it->second < kv.second)
                    return false;
            }
            return true;
        }

        void UpdateForAllocate(DescriptorSetInfo &info)
        {
            setCnt--;
            for (auto &kv : info.descriptorCntMap)
                descriptorCntMap[kv.first] -= kv.second;
        }

        void ConstructPoolSizes(std::vector<VkDescriptorPoolSize> &poolSizes)
        {
            for (auto &kv : descriptorCntMap)
            {
                VkDescriptorPoolSize uniformPoolSize{};
                uniformPoolSize.type = kv.first;
                uniformPoolSize.descriptorCount = kv.second;
                poolSizes.push_back(uniformPoolSize);
            }
        }
    };

    class DescriptorSetAllocator
    {
    private:
        static DescriptorSetAllocator *instance;
        static const int MAX_SET_CNT = 10;
        static const int MAX_COMBINED_IMAGE_SAMPLER_DESC_CNT = 20;
        static const int MAX_STORAGE_IMAGE_DESC_CNT = 10;
        static const int MAX_UNIFORM_DESC_CNT = 10;
        static const int MAX_STORAGE_DESC_CNT = 10;

        DescriptorSetAllocator()
        {
            DescriptorSetPoolInfo info(MAX_SET_CNT,
                                       {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_COMBINED_IMAGE_SAMPLER_DESC_CNT},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_STORAGE_IMAGE_DESC_CNT},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_UNIFORM_DESC_CNT},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_STORAGE_DESC_CNT}});
            VkDescriptorPool pool = createDescriptorPool(info);
            descriptorSetPools[pool] = std::move(info);
        }

        ~DescriptorSetAllocator() {}

        DescriptorSetAllocator(const DescriptorSetAllocator &);
        DescriptorSetAllocator &operator=(const DescriptorSetAllocator);

    public:
        static DescriptorSetAllocator *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("DescriptorSetAllocator not initialized!");
            return instance;
        }

        static DescriptorSetAllocator *Init()
        {
            instance = new DescriptorSetAllocator();
            return instance;
        }

        static void Dispose()
        {
            for (auto &kv : instance->descriptorSetPools)
                vkDestroyDescriptorPool(globalLogicalDevice, kv.first, nullptr);
            delete DescriptorSetAllocator::instance;
        }

        static VkDescriptorSet AllocateDescriptorSet(DescriptorSetInfo &info)
        {
            for (auto &pool : instance->descriptorSetPools)
            {
                DescriptorSetPoolInfo &poolInfo = pool.second;
                if (poolInfo.SuitableToAllocate(info))
                {
                    poolInfo.UpdateForAllocate(info);
                    return instance->allocateDescriptorSet(pool.first, &(info.layout), info.variableDescriptorCnt);
                }
            }

            std::map<VkDescriptorType, uint32_t> descriptorCntMap;
            for (auto &kv : info.descriptorCntMap)
                descriptorCntMap[kv.first] = 2 * kv.second;

            DescriptorSetPoolInfo poolInfo(MAX_SET_CNT, std::move(descriptorCntMap));
            VkDescriptorPool pool = instance->createDescriptorPool(poolInfo);
            instance->descriptorSetPools[pool] = std::move(poolInfo);
            return instance->allocateDescriptorSet(pool, &(info.layout), info.variableDescriptorCnt);
        }

        // TODO Free Descriptor Set

    private:
        std::map<VkDescriptorPool, DescriptorSetPoolInfo> descriptorSetPools;

        VkDescriptorPool createDescriptorPool(DescriptorSetPoolInfo &info)
        {
            std::vector<VkDescriptorPoolSize> poolSizes;
            info.ConstructPoolSizes(poolSizes);

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = poolSizes.size();
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
            poolInfo.maxSets = info.setCnt;

            VkDescriptorPool ret;
            if (vkCreateDescriptorPool(globalLogicalDevice, &poolInfo, nullptr, &ret) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create descriptor pool!");
            }
            return ret;
        }

        VkDescriptorSet allocateDescriptorSet(const VkDescriptorPool &pool,
                                              const VkDescriptorSetLayout *layout,
                                              const uint32_t variableDescriptorCnt)
        {
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.descriptorPool = pool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = layout;
            VkDescriptorSetVariableDescriptorCountAllocateInfo countAllocateInfo{};
            if (variableDescriptorCnt > 0) // bindless
            {
                countAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
                countAllocateInfo.descriptorSetCount = 1;
                countAllocateInfo.pDescriptorCounts = &variableDescriptorCnt;
                allocInfo.pNext = &countAllocateInfo;
            }

            VkDescriptorSet ret;
            if (vkAllocateDescriptorSets(
                    globalLogicalDevice,
                    &allocInfo,
                    &ret) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }
            return ret;
        }
    };

    void ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                     VkDescriptorSet descriptorSet,
                                     uint32_t binding,
                                     VkDescriptorType descriptorType,
                                     VkDescriptorBufferInfo *bufferInfo,
                                     int st = 0,
                                     int cnt = 1);

    void ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                     VkDescriptorSet descriptorSet,
                                     uint32_t binding,
                                     VkDescriptorType descriptorType,
                                     VkDescriptorImageInfo *imageInfo,
                                     int st = 0,
                                     int cnt = 1);
}

#endif