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
                constructPushConstant(module.reflectInfo);
            }
            constructDescriptorSetLayout();
        }

    private:
        void constructDescriptorSetInfo(SpvReflectShaderModule &reflectInfo);
        void constructPushConstant(SpvReflectShaderModule &reflectInfo);
        void constructDescriptorSetLayout();
    };

    class VertFragShader : public Shader
    {
    public:
        std::unique_ptr<ShaderModuleSet> shaderModule;
        VertFragShader() = default;
        VertFragShader(const vke_common::AssetHandle hdl, const std::vector<char> &vcode, const std::vector<char> &fcode)
            : Shader(hdl),
              shaderModule(std::make_unique<ShaderModuleSet>(vcode, fcode)) {}

        ~VertFragShader() {}

        void CreatePipeline(std::vector<uint32_t> &vertexAttributeSizes,
                            VkVertexInputRate vertexInputRate,
                            VkPipelineLayout &pipelineLayout,
                            VkGraphicsPipelineCreateInfo &pipelineInfo,
                            VkPipeline &pipeline) const;
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

        void CreatePipeline(VkPipelineLayout &pipelineLayout, VkPipeline &pipeline);
    };
}

#endif