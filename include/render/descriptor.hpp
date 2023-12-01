#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <render/environment.hpp>

namespace vke_render
{

    void InitDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding &dst,
                                        uint32_t binding,
                                        VkDescriptorType descriptorType,
                                        uint32_t descriptorCount,
                                        VkShaderStageFlags stageFlags,
                                        const VkSampler *pImmutableSamplers);

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
    };

    struct DescriptorSetInfo
    {
        VkDescriptorSetLayout layout;
        int uniformDescriptorCnt;
        int combinedImageSamplerCnt;
        int storageDescriptorCnt;

        DescriptorSetInfo() = default;
        DescriptorSetInfo(VkDescriptorSetLayout layout, int uniformDescriptorCnt, int combinedImageSamplerCnt, int storageDescriptorCnt)
            : layout(layout),
              uniformDescriptorCnt(uniformDescriptorCnt),
              combinedImageSamplerCnt(combinedImageSamplerCnt),
              storageDescriptorCnt(storageDescriptorCnt) {}
        void AddCnt(VkDescriptorType type)
        {
            switch (type)
            {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                uniformDescriptorCnt++;
                break;
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                combinedImageSamplerCnt++;
                break;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                storageDescriptorCnt++;
                break;
            }
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

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (DescriptorSetAllocator::instance != nullptr)
                    delete DescriptorSetAllocator::instance;
            }
        };
        static Deletor deletor;

    public:
        static DescriptorSetAllocator *GetInstance()
        {
            if (instance == nullptr)
                instance = new DescriptorSetAllocator();
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

        // DescriptorSetAllocator(int poolCnt, DescriptorSetPoolInfo info)
        // {
        //     for (int i = 0; i < poolCnt; i++)
        //     {
        //         VkDescriptorPool pool = createDescriptorPool(info);
        //         descriptorSetPools[pool] = info;
        //     }
        // }

        static VkDescriptorSet AllocateDescriptorSet(DescriptorSetInfo &info)
        {
            for (auto &pool : instance->descriptorSetPools)
            {
                DescriptorSetPoolInfo &poolInfo = pool.second;
                if (poolInfo.setCnt > 0 &&
                    poolInfo.uniformDescriptorCnt >= info.uniformDescriptorCnt &&
                    poolInfo.combinedImageSamplerCnt >= info.combinedImageSamplerCnt)
                {
                    poolInfo.setCnt--;
                    poolInfo.uniformDescriptorCnt -= info.uniformDescriptorCnt;
                    poolInfo.combinedImageSamplerCnt -= info.combinedImageSamplerCnt;
                    return instance->allocateDescriptorSet(pool.first, &(info.layout));
                }
            }

            DescriptorSetPoolInfo poolInfo(
                MAX_SET_CNT,
                info.uniformDescriptorCnt * 2,
                info.combinedImageSamplerCnt * 2,
                info.storageDescriptorCnt * 2);
            VkDescriptorPool pool = instance->createDescriptorPool(poolInfo);
            instance->descriptorSetPools[pool] = poolInfo;
            return instance->allocateDescriptorSet(pool, &(info.layout));
        }

    private:
        std::map<VkDescriptorPool, DescriptorSetPoolInfo> descriptorSetPools;

        VkDescriptorPool createDescriptorPool(DescriptorSetPoolInfo &info)
        {
            VkDescriptorPoolSize uniformPoolSize{};
            uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniformPoolSize.descriptorCount = info.uniformDescriptorCnt;

            VkDescriptorPoolSize combinedImageSamplerPoolSize{};
            combinedImageSamplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            combinedImageSamplerPoolSize.descriptorCount = info.combinedImageSamplerCnt;

            std::vector<VkDescriptorPoolSize> poolSizes = {uniformPoolSize, combinedImageSamplerPoolSize};

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = poolSizes.size();
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = info.setCnt;

            VkDescriptorPool ret;
            if (vkCreateDescriptorPool(RenderEnvironment::GetInstance()->logicalDevice, &poolInfo, nullptr, &ret) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create descriptor pool!");
            }
            return ret;
        }

        VkDescriptorSet allocateDescriptorSet(const VkDescriptorPool &pool, const VkDescriptorSetLayout *layout)
        {
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = pool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = layout;

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

    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkDescriptorSet descriptorSet, DescriptorInfo &descriptorInfo, VkDescriptorBufferInfo *bufferInfo);
    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkDescriptorSet descriptorSet, DescriptorInfo &descriptorInfo, VkDescriptorImageInfo *imageInfo);
}

#endif