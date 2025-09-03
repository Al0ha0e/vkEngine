#include <render/shader.hpp>

namespace vke_render
{
    void ShaderModuleSet::constructDescriptorSetInfo(SpvReflectShaderModule &reflectInfo)
    {
        int setCnt = reflectInfo.descriptor_set_count;
        for (int i = 0; i < setCnt; i++)
        {
            SpvReflectDescriptorSet &ds = reflectInfo.descriptor_sets[i];
            uint32_t setID = ds.set;

            auto it = bindingInfoMap.find(setID);
            if (it != bindingInfoMap.end())
                continue;

            DescriptorSetInfo descriptorSetInfo;
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            int bindingCnt = ds.binding_count;
            for (int j = 0; j < bindingCnt; j++)
            {
                SpvReflectDescriptorBinding &bindingInfo = *ds.bindings[j];
                uint32_t desCnt = 1;
                for (int k = 0; k < bindingInfo.array.dims_count; k++)
                    desCnt *= bindingInfo.array.dims[k];

                VkDescriptorSetLayoutBinding binding{
                    bindingInfo.binding,
                    (VkDescriptorType)bindingInfo.descriptor_type,
                    desCnt,
                    VK_SHADER_STAGE_ALL,
                    nullptr};
                if (desCnt == 0) // bindless
                {
                    binding.descriptorCount = DEFAULT_BINDLESS_CNT;
                    descriptorSetInfo.variableDescriptorCnt = DEFAULT_BINDLESS_CNT;
                    descriptorSetInfo.AddCnt(binding.descriptorType, binding.descriptorCount);
                }
                else
                    descriptorSetInfo.AddCnt(binding.descriptorType, binding.descriptorCount);
                bindings.push_back(binding);
            }

            descriptorSetInfoMap[setID] = std::move(descriptorSetInfo);
            bindingInfoMap[setID] = std::move(bindings);
        }
    }

    void ShaderModuleSet::constructPushConstant(SpvReflectShaderModule &reflectInfo)
    {
        int pushConstantCnt = reflectInfo.push_constant_block_count;
        for (int i = 0; i < pushConstantCnt; i++)
        {
            SpvReflectBlockVariable &block = reflectInfo.push_constant_blocks[i];

            auto it = pushConstantRangeMap.find(block.name);
            if (it != pushConstantRangeMap.end())
                continue;

            VkPushConstantRange range{};
            range.stageFlags = VK_SHADER_STAGE_ALL;
            range.offset = block.offset;
            range.size = block.padded_size;
            pushConstantRangeMap[block.name] = range;
        }
    }

    void ShaderModuleSet::constructDescriptorSetLayout()
    {
        for (auto &it : descriptorSetInfoMap)
        {
            DescriptorSetInfo &descriptorSetInfo = it.second;
            std::vector<VkDescriptorSetLayoutBinding> &bindings = bindingInfoMap[it.first];

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.pNext = nullptr;
            layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            layoutInfo.bindingCount = bindings.size();
            layoutInfo.pBindings = bindings.data();

            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{};
            std::vector<VkDescriptorBindingFlags> flags;
            if (descriptorSetInfo.variableDescriptorCnt > 0) // bindless
            {
                flags.resize(bindings.size());
                for (int i = 0; i < bindings.size() - 1; i++)
                    flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
                flags[bindings.size() - 1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                                             VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
                                             VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

                bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                bindingFlagsCreateInfo.pNext = nullptr;
                bindingFlagsCreateInfo.bindingCount = bindings.size();
                bindingFlagsCreateInfo.pBindingFlags = flags.data();
                layoutInfo.pNext = &bindingFlagsCreateInfo;
            }

            if (vkCreateDescriptorSetLayout(RenderEnvironment::GetInstance()->logicalDevice,
                                            &layoutInfo,
                                            nullptr,
                                            &(descriptorSetInfo.layout)) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
            descriptorSetLayouts.push_back(descriptorSetInfo.layout);
        }
    }
}