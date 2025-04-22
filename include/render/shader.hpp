#ifndef SHADER_H
#define SHADER_H

#include <render/environment.hpp>
#include <render/descriptor.hpp>
#include <string>
#include <iostream>
#include <spirv_reflect.h>

namespace vke_render
{
    class Shader
    {
    public:
        vke_common::AssetHandle handle;
        Shader() = default;
        Shader(const vke_common::AssetHandle hdl) : handle(hdl) {}
        virtual ~Shader() {}
    };

    class ShaderModule
    {
    public:
        VkShaderModule module;
        SpvReflectShaderModule reflectInfo;

        ShaderModule() : module(nullptr) {}

        ShaderModule(const std::vector<char> &code)
        {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
            RenderEnvironment *env = RenderEnvironment::GetInstance();
            if (vkCreateShaderModule(env->logicalDevice, &createInfo, nullptr, &module) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create shader module!");
            }

            SpvReflectResult result = spvReflectCreateShaderModule(code.size(), code.data(), &reflectInfo);
            assert(result == SPV_REFLECT_RESULT_SUCCESS);
        }

        ShaderModule(const ShaderModule &) = delete;
        ShaderModule &operator=(const ShaderModule &) = delete;

        ShaderModule(ShaderModule &&ano)
        {
            if (this == &ano)
                return;
            module = ano.module;
            reflectInfo = ano.reflectInfo;
            ano.module = nullptr;
        }

        ShaderModule &operator=(ShaderModule &&ano)
        {
            if (this == &ano)
                return *this;
            dispose();

            module = ano.module;
            reflectInfo = ano.reflectInfo;
            ano.module = nullptr;

            return *this;
        }

        ~ShaderModule()
        {
            dispose();
        }

    private:
        void dispose()
        {
            if (module != nullptr)
            {
                RenderEnvironment *env = RenderEnvironment::GetInstance();
                vkDestroyShaderModule(env->logicalDevice, module, nullptr);
                spvReflectDestroyShaderModule(&reflectInfo);
            }
        }
    };

    class ShaderModuleSet
    {
    public:
        std::vector<ShaderModule> modules;
        std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
        std::map<uint32_t, DescriptorSetInfo> descriptorSetInfoMap;
        std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindingInfoMap;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::map<std::string, VkPushConstantRange> pushConstantRangeMap;

        ShaderModuleSet() {}

        ShaderModuleSet(const ShaderModuleSet &) = delete;
        ShaderModuleSet &operator=(const ShaderModuleSet &) = delete;

        ShaderModuleSet(ShaderModuleSet &&ano)
        {
            if (this == &ano)
                return;
            modules = std::move(ano.modules);
            stageCreateInfos = std::move(ano.stageCreateInfos);
            descriptorSetInfoMap = std::move(ano.descriptorSetInfoMap);
            bindingInfoMap = std::move(ano.bindingInfoMap);
            descriptorSetLayouts = std::move(ano.descriptorSetLayouts);
            pushConstantRangeMap = std::move(ano.pushConstantRangeMap);
        }

        ShaderModuleSet &operator=(ShaderModuleSet &&ano)
        {
            if (this == &ano)
                return *this;
            modules = std::move(ano.modules);
            stageCreateInfos = std::move(ano.stageCreateInfos);
            descriptorSetInfoMap = std::move(ano.descriptorSetInfoMap);
            bindingInfoMap = std::move(ano.bindingInfoMap);
            descriptorSetLayouts = std::move(ano.descriptorSetLayouts);
            pushConstantRangeMap = std::move(ano.pushConstantRangeMap);

            return *this;
        }

        template <typename... Args>
        ShaderModuleSet(Args &&...code)
        {
            (modules.emplace_back(std::forward<Args>(code)), ...);
            for (auto &module : modules)
            {
                VkPipelineShaderStageCreateInfo stageCreateInfo = VkPipelineShaderStageCreateInfo{};
                stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageCreateInfo.stage = (VkShaderStageFlagBits)module.reflectInfo.shader_stage;
                stageCreateInfo.module = module.module;
                stageCreateInfo.pName = module.reflectInfo.entry_point_name;
                stageCreateInfos.push_back(stageCreateInfo);
                constructDescriptorSetInfo(module.reflectInfo);
            }
            constructDescriptorSetLayout();
        }

    private:
        void constructDescriptorSetInfo(SpvReflectShaderModule &reflectInfo)
        {
            int setCnt = reflectInfo.descriptor_set_count;
            for (int i = 0; i < setCnt; i++)
            {
                SpvReflectDescriptorSet &ds = reflectInfo.descriptor_sets[i];
                uint32_t setID = ds.set;

                auto &it = bindingInfoMap.find(setID);
                if (it != bindingInfoMap.end())
                {
                    std::vector<VkDescriptorSetLayoutBinding> &bindings = it->second;
                    for (auto &binding : bindings)
                        binding.stageFlags |= reflectInfo.shader_stage;
                    continue;
                }

                DescriptorSetInfo descriptorSetInfo(nullptr, 0, 0, 0, 0);
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
                        DEFAULT_BINDLESS_CNT,
                        (VkShaderStageFlagBits)reflectInfo.shader_stage,
                        nullptr};
                    bindings.push_back(binding);
                    if (binding.descriptorCount == 0) // bindless
                    {
                        descriptorSetInfo.variableDescriptorCnt = DEFAULT_BINDLESS_CNT;
                        descriptorSetInfo.AddCnt(binding.descriptorType, binding.descriptorCount);
                    }
                    else
                        descriptorSetInfo.AddCnt(binding.descriptorType, binding.descriptorCount);
                }

                descriptorSetInfoMap[setID] = descriptorSetInfo;
                bindingInfoMap[setID] = std::move(bindings);
            }
        }

        void constructPushConstant(SpvReflectShaderModule &reflectInfo)
        {
            int pushConstantCnt = reflectInfo.push_constant_block_count;
            for (int i = 0; i < pushConstantCnt; i++)
            {
                SpvReflectBlockVariable &block = reflectInfo.push_constant_blocks[i];

                auto &it = pushConstantRangeMap.find(block.name);
                if (it != pushConstantRangeMap.end())
                {
                    it->second.stageFlags |= (VkShaderStageFlagBits)reflectInfo.shader_stage;
                    continue;
                }

                VkPushConstantRange range{};
                range.stageFlags = (VkShaderStageFlagBits)reflectInfo.shader_stage;
                range.offset = block.offset;
                range.size = block.padded_size;
                pushConstantRangeMap[block.name] = range;
            }
        }

        void constructDescriptorSetLayout()
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
    };

    class VertFragShader : public Shader
    {
    public:
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        VertFragShader() = default;
        VertFragShader(const vke_common::AssetHandle hdl, const std::vector<char> &vcode, const std::vector<char> &fcode)
            : Shader(hdl),
              vertShaderModule(std::make_unique<ShaderModule>(vcode)),
              fragShaderModule(std::make_unique<ShaderModule>(fcode))
        {
            VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
            vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertShaderStageInfo.module = vertShaderModule->module;
            vertShaderStageInfo.pName = "main";

            VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = fragShaderModule->module;
            fragShaderStageInfo.pName = "main";

            shaderStages = {vertShaderStageInfo, fragShaderStageInfo};
        }

        ~VertFragShader() {}

    private:
        std::unique_ptr<ShaderModule> vertShaderModule;
        std::unique_ptr<ShaderModule> fragShaderModule;
    };

    class ComputeShader : public Shader
    {
    public:
        std::unique_ptr<ShaderModuleSet> shaderModule;
        ComputeShader() = default;
        ComputeShader(const vke_common::AssetHandle hdl, const std::vector<char> &code)
            : Shader(hdl),
              shaderModule(std::make_unique<ShaderModuleSet>(code)) {}

        ~ComputeShader() {}

        void CreatePipeline(VkPipelineLayout &pipelineLayout, VkPipeline &pipeline)
        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = shaderModule->descriptorSetLayouts.size();
            pipelineLayoutInfo.pSetLayouts = shaderModule->descriptorSetLayouts.data();
            std::vector<VkPushConstantRange> pushConstantRanges(shaderModule->pushConstantRangeMap.size());
            int i = 0;
            for (auto &kv : shaderModule->pushConstantRangeMap)
                pushConstantRanges[i++] = kv.second;

            pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
            pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;

            if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &(pipelineLayout)) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create compute pipeline layout!");
            }

            VkComputePipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.layout = pipelineLayout;
            pipelineInfo.stage = shaderModule->stageCreateInfos[0];

            if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create compute pipeline!");
            }
        }
    };
}

#endif