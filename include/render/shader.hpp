#ifndef SHADER_H
#define SHADER_H

#include <render/environment.hpp>
#include <string>
#include <iostream>

namespace vke_render
{
    class Shader
    {
    public:
        Shader() = default;
        virtual ~Shader() {}

    protected:
        VkShaderModule createShaderModule(const std::vector<char> &code)
        {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
            RenderEnvironment *env = RenderEnvironment::GetInstance();
            VkShaderModule shaderModule;
            if (vkCreateShaderModule(env->logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create shader module!");
            }

            return shaderModule;
        }
    };

    class VertFragShader : public Shader
    {
    public:
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        VertFragShader() = default;
        VertFragShader(const std::vector<char> &vcode, const std::vector<char> &fcode) : Shader()
        {
            vertShaderModule = createShaderModule(vcode);
            fragShaderModule = createShaderModule(fcode);

            VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
            vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertShaderStageInfo.module = vertShaderModule;
            vertShaderStageInfo.pName = "main";

            VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = fragShaderModule;
            fragShaderStageInfo.pName = "main";

            shaderStages = {vertShaderStageInfo, fragShaderStageInfo};
        }

        ~VertFragShader()
        {
            RenderEnvironment *env = RenderEnvironment::GetInstance();
            vkDestroyShaderModule(env->logicalDevice, fragShaderModule, nullptr);
            vkDestroyShaderModule(env->logicalDevice, vertShaderModule, nullptr);
        }

    private:
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };

    class ComputeShader : public Shader
    {
    public:
        VkPipelineShaderStageCreateInfo shaderStageInfo;

        ComputeShader() = default;
        ComputeShader(const std::vector<char> &code) : Shader()
        {
            shaderModule = createShaderModule(code);

            shaderStageInfo = VkPipelineShaderStageCreateInfo{};
            shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            shaderStageInfo.module = shaderModule;
            shaderStageInfo.pName = "main";
        }

        ~ComputeShader()
        {
            RenderEnvironment *env = RenderEnvironment::GetInstance();
            vkDestroyShaderModule(env->logicalDevice, shaderModule, nullptr);
        }

    private:
        VkShaderModule shaderModule;
    };
}

#endif