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
                    desCnt,
                    (VkShaderStageFlagBits)reflectInfo.shader_stage,
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

            descriptorSetInfoMap[setID] = descriptorSetInfo;
            bindingInfoMap[setID] = std::move(bindings);
        }
    }

    void ShaderModuleSet::constructPushConstant(SpvReflectShaderModule &reflectInfo)
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

    void VertFragShader::CreatePipeline(std::vector<uint32_t> &vertexAttributeSizes,
                                        VkPipelineLayout &pipelineLayout,
                                        VkGraphicsPipelineCreateInfo &pipelineInfo,
                                        VkPipeline &pipeline)
    {
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        SpvReflectShaderModule &vertReflectInfo = shaderModule->modules[0].reflectInfo;
        uint32_t vertexAttributeOffset = 0;
        std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions(vertReflectInfo.input_variable_count);
        for (int i = 0; i < vertReflectInfo.input_variable_count; i++)
        {
            SpvReflectInterfaceVariable &variable = *(vertReflectInfo.input_variables[i]);
            vertexAttributeDescriptions[i].binding = 0;
            vertexAttributeDescriptions[i].location = variable.location;
            vertexAttributeDescriptions[i].format = (VkFormat)variable.format;
            vertexAttributeDescriptions[i].offset = vertexAttributeOffset;
            vertexAttributeOffset += vertexAttributeSizes[i];
        }

        VkVertexInputBindingDescription vertexBindingDescription{};
        vertexBindingDescription.binding = 0;
        vertexBindingDescription.stride = vertexAttributeOffset;
        vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f;          // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;          // Optional
        multisampling.pSampleMask = nullptr;            // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE;      // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        /////////////////////////////////////////////////////////////////////////////
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
        /////////////////////////////////////////////////////////////////////////////

        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderModule->stageCreateInfos.size();
        pipelineInfo.pStages = shaderModule->stageCreateInfos.data();

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        // pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;

        if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");
    }

    void ComputeShader::CreatePipeline(VkPipelineLayout &pipelineLayout, VkPipeline &pipeline)
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
}