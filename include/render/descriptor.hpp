#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <render/environment.hpp>

namespace vke_render
{

    const uint32_t DEFAULT_BINDLESS_CNT = 1024;

    void InitDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding &dst,
                                        uint32_t binding,
                                        VkDescriptorType descriptorType,
                                        uint32_t descriptorCount,
                                        VkShaderStageFlags stageFlags,
                                        const VkSampler *pImmutableSamplers);

    void InitDescriptorBufferInfo(VkDescriptorBufferInfo &bufferInfo,
                                  VkBuffer buffer,
                                  VkDeviceSize offset,
                                  VkDeviceSize range);

    struct DescriptorInfo
    {
        VkDescriptorSetLayoutBinding bindingInfo;
        size_t bufferSize;
        VkImageView imageView;
        VkSampler imageSampler;

        DescriptorInfo(VkDescriptorSetLayoutBinding bInfo, size_t bSize)
            : bindingInfo(bInfo), bufferSize(bSize), imageView(nullptr), imageSampler(nullptr) {}

        DescriptorInfo(VkDescriptorSetLayoutBinding bInfo, VkImageView view, VkSampler sampler)
            : bindingInfo(bInfo), bufferSize(0), imageView(view), imageSampler(sampler) {}

        DescriptorInfo(DescriptorInfo &&ori)
        {
            bindingInfo = ori.bindingInfo;
            bufferSize = ori.bufferSize;
            imageView = ori.imageView;
            imageSampler = ori.imageSampler;
            ori.imageView = nullptr;
            ori.imageSampler = nullptr;
        }
    };

    struct DescriptorSetInfo
    {
        VkDescriptorSetLayout layout;
        uint32_t variableDescriptorCnt;
        int uniformDescriptorCnt;
        int combinedImageSamplerCnt;
        int storageDescriptorCnt;

        DescriptorSetInfo()
            : layout(nullptr), variableDescriptorCnt(0), uniformDescriptorCnt(0), combinedImageSamplerCnt(0), storageDescriptorCnt(0) {}

        DescriptorSetInfo(VkDescriptorSetLayout layout, uint32_t variableDescriptorCnt, int uniformDescriptorCnt, int combinedImageSamplerCnt, int storageDescriptorCnt)
            : layout(layout),
              variableDescriptorCnt(variableDescriptorCnt),
              uniformDescriptorCnt(uniformDescriptorCnt),
              combinedImageSamplerCnt(combinedImageSamplerCnt),
              storageDescriptorCnt(storageDescriptorCnt) {}

        ~DescriptorSetInfo()
        {
            if (layout)
            {
                VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
                vkDestroyDescriptorSetLayout(logicalDevice, layout, nullptr);
            }
        }

        void AddCnt(VkDescriptorType type, int cnt)
        {
            switch (type)
            {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                uniformDescriptorCnt += cnt;
                break;
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                combinedImageSamplerCnt += cnt;
                break;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                storageDescriptorCnt += cnt;
                break;
            }
        }

        void AddCnt(VkDescriptorSetLayoutBinding info)
        {
            AddCnt(info.descriptorType, info.descriptorCount);
        }
    };

    struct DescriptorSetPoolInfo
    {
        int setCnt;
        int uniformDescriptorCnt;
        int combinedImageSamplerCnt;
        int storageDescriptorCnt;

        DescriptorSetPoolInfo() = default;
        DescriptorSetPoolInfo(int setCnt, int uniformDescriptorCnt, int combinedImageSamplerCnt, int storageDescriptorCnt)
            : setCnt(setCnt),
              uniformDescriptorCnt(uniformDescriptorCnt),
              combinedImageSamplerCnt(combinedImageSamplerCnt),
              storageDescriptorCnt(storageDescriptorCnt) {}

        bool SuitableToAllocate(DescriptorSetInfo &info)
        {
            return setCnt > 0 &&
                   uniformDescriptorCnt >= info.uniformDescriptorCnt &&
                   combinedImageSamplerCnt >= info.combinedImageSamplerCnt &&
                   storageDescriptorCnt >= info.storageDescriptorCnt;
        }

        void UpdateForAllocate(DescriptorSetInfo &info)
        {
            setCnt--;
            uniformDescriptorCnt -= info.uniformDescriptorCnt;
            combinedImageSamplerCnt -= info.combinedImageSamplerCnt;
            storageDescriptorCnt -= info.storageDescriptorCnt;
        }

        void ConstructPoolSizes(std::vector<VkDescriptorPoolSize> &poolSizes)
        {
            if (uniformDescriptorCnt > 0)
            {
                VkDescriptorPoolSize uniformPoolSize{};
                uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uniformPoolSize.descriptorCount = uniformDescriptorCnt;
                poolSizes.push_back(uniformPoolSize);
            }

            if (combinedImageSamplerCnt > 0)
            {
                VkDescriptorPoolSize combinedImageSamplerPoolSize{};
                combinedImageSamplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                combinedImageSamplerPoolSize.descriptorCount = combinedImageSamplerCnt;
                poolSizes.push_back(combinedImageSamplerPoolSize);
            }

            if (storageDescriptorCnt > 0)
            {
                VkDescriptorPoolSize storagePoolSize{};
                storagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                storagePoolSize.descriptorCount = storageDescriptorCnt;
                poolSizes.push_back(storagePoolSize);
            }
        }
    };

    class DescriptorSetAllocator
    {
    private:
        static DescriptorSetAllocator *instance;
        static const int MAX_SET_CNT = 10;
        static const int MAX_UNIFORM_DESC_CNT = 10;
        static const int MAX_COMBINED_IMAGE_SAMPLER_DESC_CNT = 20;
        static const int MAX_STORAGE_DESC_CNT = 10;

        DescriptorSetAllocator()
        {
            DescriptorSetPoolInfo info(MAX_SET_CNT, MAX_UNIFORM_DESC_CNT, MAX_COMBINED_IMAGE_SAMPLER_DESC_CNT, MAX_STORAGE_DESC_CNT);
            VkDescriptorPool pool = createDescriptorPool(info);
            descriptorSetPools[pool] = info;
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

        static DescriptorSetAllocator *Init(int poolCnt, DescriptorSetPoolInfo info)
        {
            instance = new DescriptorSetAllocator();
            for (int i = 0; i < poolCnt; i++)
            {
                VkDescriptorPool pool = instance->createDescriptorPool(info);
                instance->descriptorSetPools[pool] = info;
            }
            return instance;
        }

        static void Dispose()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            for (auto kv : instance->descriptorSetPools)
                vkDestroyDescriptorPool(logicalDevice, kv.first, nullptr);
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

            DescriptorSetPoolInfo poolInfo(
                MAX_SET_CNT,
                info.uniformDescriptorCnt * 2,
                info.combinedImageSamplerCnt * 2,
                info.storageDescriptorCnt * 2);
            VkDescriptorPool pool = instance->createDescriptorPool(poolInfo);
            instance->descriptorSetPools[pool] = poolInfo;
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
            if (vkCreateDescriptorPool(RenderEnvironment::GetInstance()->logicalDevice, &poolInfo, nullptr, &ret) != VK_SUCCESS)
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
                    RenderEnvironment::GetInstance()->logicalDevice,
                    &allocInfo,
                    &ret) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }
            return ret;
        }
    };
    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     uint32_t binding,
                                                     VkDescriptorType descriptorType,
                                                     int st,
                                                     int cnt,
                                                     VkDescriptorBufferInfo *bufferInfo);

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     uint32_t binding,
                                                     VkDescriptorType descriptorType,
                                                     int st,
                                                     int cnt,
                                                     VkDescriptorImageInfo *imageInfo);

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     DescriptorInfo &descriptorInfo,
                                                     int st,
                                                     int cnt,
                                                     VkDescriptorBufferInfo *bufferInfo);

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     DescriptorInfo &descriptorInfo,
                                                     int st,
                                                     int cnt,
                                                     VkDescriptorImageInfo *imageInfo);

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     uint32_t binding,
                                                     VkDescriptorType descriptorType,
                                                     VkDescriptorBufferInfo *bufferInfo);

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     uint32_t binding,
                                                     VkDescriptorType descriptorType,
                                                     VkDescriptorImageInfo *imageInfo);

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     DescriptorInfo &descriptorInfo,
                                                     VkDescriptorBufferInfo *bufferInfo);

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkWriteDescriptorSet &descriptorWrite,
                                                     VkDescriptorSet descriptorSet,
                                                     DescriptorInfo &descriptorInfo,
                                                     VkDescriptorImageInfo *imageInfo);
}

#endif