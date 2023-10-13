#include <render/compute.hpp>

namespace vke_render
{
    ComputeManager *ComputeManager::instance = nullptr;

    void ComputeManager::createGraphicsPipeline(ComputeInfo &computeInfo)
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &(computeInfo.dscriptorSetInfo.layout);

        VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;

        if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &(computeInfo.pipelineLayout)) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = computeInfo.pipelineLayout;
        pipelineInfo.stage = computeInfo.shader->shaderStageInfo;

        if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computeInfo.pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create compute pipeline!");
        }
    }
}