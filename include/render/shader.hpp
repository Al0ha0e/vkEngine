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
        Shader(const std::vector<char> &vcode, const std::vector<char> &fcode)
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

        ~Shader()
        {
            RenderEnvironment *env = RenderEnvironment::GetInstance();
            vkDestroyShaderModule(env->logicalDevice, fragShaderModule, nullptr);
            vkDestroyShaderModule(env->logicalDevice, vertShaderModule, nullptr);
        }

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    private:
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;

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
}

#endif