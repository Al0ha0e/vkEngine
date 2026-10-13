#include <render/layered_2d.hpp>
#include <asset/asset_manager.hpp>
#include <algorithm>
#include <cstring>

namespace vke_render
{
    bool QuadIDVertexBufferPool::Ensure(Allocation &allocation, uint32_t count)
    {
        if (count == 0)
        {
            Release(allocation);
            return true;
        }

        if (allocation.Valid() && count <= allocation.capacity)
            return true;

        Release(allocation);
        allocation = allocateCapacity(capacityClass(count));
        return allocation.Valid();
    }

    void QuadIDVertexBufferPool::Update(const Allocation &allocation, const std::vector<QuadID> &quadIDs,
                                         uint32_t currentFrame)
    {
        if (!allocation.Valid() || quadIDs.empty())
            return;
        chunks[allocation.chunkIndex].buffers[currentFrame]->ToBuffer(
            static_cast<size_t>(allocation.offset) * sizeof(QuadID),
            quadIDs.data(),
            quadIDs.size() * sizeof(QuadID));
    }

    void QuadIDVertexBufferPool::Release(Allocation &allocation)
    {
        if (!allocation.Valid())
            return;

        allocation.count = 0;
        freeLists[allocation.capacity].push_back(allocation);
        allocation = {};
    }

    VkBuffer QuadIDVertexBufferPool::GetBuffer(const Allocation &allocation, uint32_t currentFrame) const
    {
        return chunks[allocation.chunkIndex].buffers[currentFrame]->buffer;
    }

    VkDeviceSize QuadIDVertexBufferPool::GetOffset(const Allocation &allocation) const
    {
        return static_cast<VkDeviceSize>(allocation.offset) * sizeof(QuadID);
    }

    QuadIDVertexBufferPool::Allocation QuadIDVertexBufferPool::allocateCapacity(uint32_t capacity)
    {
        auto &freeList = freeLists[capacity];
        if (!freeList.empty())
        {
            Allocation allocation = freeList.back();
            freeList.pop_back();
            allocation.count = 0;
            return allocation;
        }

        for (uint32_t chunkIndex = 0; chunkIndex < chunks.size(); ++chunkIndex)
        {
            Chunk &chunk = chunks[chunkIndex];
            if (capacity <= chunk.capacity - chunk.used)
            {
                Allocation allocation{chunkIndex, chunk.used, capacity, 0};
                chunk.used += capacity;
                return allocation;
            }
        }

        const uint32_t chunkCapacity = std::max(DEFAULT_CHUNK_CAPACITY, capacity);
        Chunk chunk;
        for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
            chunk.buffers[frame] = std::make_unique<HostCoherentBuffer>(
                static_cast<VkDeviceSize>(chunkCapacity) * sizeof(QuadID), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        chunk.capacity = chunkCapacity;
        chunks.push_back(std::move(chunk));

        Allocation allocation{static_cast<uint32_t>(chunks.size() - 1), 0, capacity, 0};
        chunks.back().used = capacity;
        return allocation;
    }

    uint32_t QuadIDVertexBufferPool::capacityClass(uint32_t count)
    {
        uint32_t capacity = 1;
        while (capacity < count)
            capacity <<= 1;
        return capacity;
    }

    Layered2DRenderUnit::~Layered2DRenderUnit()
    {
        if (quadIDPool != nullptr)
            quadIDPool->Release(quadIDAllocation);
    }

    void Layered2DRenderUnit::SetQuadIDs(std::vector<QuadID> ids)
    {
        quadIDs = std::move(ids);
        if (quadIDs.empty())
        {
            quadIDPool->Release(quadIDAllocation);
            quadIDUpdateCnt = 0;
            return;
        }

        if (quadIDPool->Ensure(quadIDAllocation, static_cast<uint32_t>(quadIDs.size())))
        {
            quadIDAllocation.count = static_cast<uint32_t>(quadIDs.size());
            quadIDUpdateCnt = MAX_FRAMES_IN_FLIGHT;
        }
    }

    void Layered2DRenderUnit::Render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                                     const glm::vec2 &viewportSize, const glm::vec2 &atlasSize, uint32_t currentFrame)
    {
        if (quadIDs.empty() || !quadIDAllocation.Valid() || quadIDPool == nullptr)
            return;

        if (quadIDUpdateCnt > 0)
        {
            quadIDPool->Update(quadIDAllocation, quadIDs, currentFrame);
            --quadIDUpdateCnt;
        }
        VkBuffer quadIDBuffer = quadIDPool->GetBuffer(quadIDAllocation, currentFrame);
        VkDeviceSize offset = quadIDPool->GetOffset(quadIDAllocation);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &quadIDBuffer, &offset);
        Layered2DRenderPushConstants constants{modelMatrix, viewportSize, atlasSize};
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(constants), &constants);
        vkCmdDraw(commandBuffer, 6, quadIDAllocation.count, 0, 0);
    }

    Layered2DRenderInfo::Layered2DRenderInfo(std::shared_ptr<Material> material, RenderContext *context)
        : material(std::move(material))
    {
        if (!this->material->textures.empty())
        {
            commonDescriptorSet = this->material->shader->CreateDescriptorSet(1);
            if (commonDescriptorSet != VK_NULL_HANDLE)
                this->material->UpdateDescriptorSet(commonDescriptorSet);
        }
        createPipeline(context);
    }

    void Layered2DRenderInfo::Render(VkCommandBuffer commandBuffer, VkDescriptorSet rendererDescriptorSet,
                                     const glm::vec2 &viewportSize, const glm::vec2 &atlasSize,
                                     uint32_t currentFrame) const
    {
        if (units.empty())
            return;
        renderPipeline->Bind(commandBuffer);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderPipeline->pipelineLayout, 0, 1, &rendererDescriptorSet, 0, nullptr);
        if (commonDescriptorSet != VK_NULL_HANDLE)
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    renderPipeline->pipelineLayout, 1, 1, &commonDescriptorSet, 0, nullptr);
        material->SetPushConstants(commandBuffer, renderPipeline->pipelineLayout);
        for (const auto &entry : units)
            entry.second->Render(commandBuffer, renderPipeline->pipelineLayout, viewportSize, atlasSize, currentFrame);
    }

    void Layered2DRenderInfo::createPipeline(RenderContext *context)
    {
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &context->colorFormat;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if (material->blendMode == MaterialBlendMode::ADDITIVE)
        {
            blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        }
        else if (material->blendMode == MaterialBlendMode::PREMULTIPLIED_ALPHA)
        {
            blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        }
        else
        {
            blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        }

        VkPipelineColorBlendStateCreateInfo blendState{};
        blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendState.attachmentCount = 1;
        blendState.pAttachments = &blendAttachment;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &blendState;
        renderPipeline = std::make_unique<GraphicsPipeline>(
            material->shader, std::vector<uint32_t>{sizeof(QuadID)}, VK_VERTEX_INPUT_RATE_INSTANCE, pipelineInfo);
    }

    void Layered2DRenderLayer::AddUnit(const std::shared_ptr<Material> &material,
                                       Layered2DRenderUnit *unit, RenderContext *context)
    {
        auto it = renderInfos.find(material.get());
        if (it == renderInfos.end())
            it = renderInfos.emplace(material.get(), std::make_unique<Layered2DRenderInfo>(material, context)).first;
        it->second->AddUnit(unit);
    }

    void Layered2DRenderLayer::RemoveUnit(Material *material, vke_ds::id32_t unitID)
    {
        auto it = renderInfos.find(material);
        if (it == renderInfos.end())
            return;
        it->second->RemoveUnit(unitID);
    }

    void Layered2DRenderLayer::Render(VkCommandBuffer commandBuffer, VkDescriptorSet rendererDescriptorSet,
                                      const glm::vec2 &viewportSize, const glm::vec2 &atlasSize,
                                      uint32_t currentFrame) const
    {
        for (const auto &entry : renderInfos)
            entry.second->Render(commandBuffer, rendererDescriptorSet, viewportSize, atlasSize, currentFrame);
    }

    void Layered2DRenderer::Init(int subpassID, FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 ResourceNodeIDMap &currentResourceNodeID)
    {
        RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
        font = vke_common::AssetManager::LoadFont(vke_common::BUILTIN_FONT_ARIAL_ID);
        textShader = vke_common::AssetManager::LoadVertFragShader(vke_common::BUILTIN_VFSHADER_TEXT_ID);
        defaultMaterial = std::make_shared<Material>();
        defaultMaterial->shader = textShader;
        defaultMaterial->renderMode = MaterialRenderMode::BLEND_MODE;

        for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
        {
            dynamicAtlasStagingBuffers[frame] = std::make_unique<HostCoherentBuffer>(
                static_cast<VkDeviceSize>(vke_common::Font::ATLAS_SIZE) * vke_common::Font::ATLAS_SIZE,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            dynamicAtlasTextures[frame] = std::make_unique<Texture2D>(
                font->handle, const_cast<uint8_t *>(font->GetDynamicAtlasPixels().data()),
                vke_common::Font::ATLAS_SIZE, vke_common::Font::ATLAS_SIZE,
                VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false, false);
        }

        constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
        createDescriptorSets();
    }

    bool Layered2DRenderer::AllocateUnit(vke_ds::id32_t unitID, const std::shared_ptr<Material> &material,
                                         const glm::mat4 &modelMatrix, const std::vector<QuadID> &quadIDs)
    {
        if (units.find(unitID) != units.end())
            return false;

        UnitState state;
        state.material = material == nullptr ? defaultMaterial : material;
        state.unit = std::make_unique<Layered2DRenderUnit>(unitID, modelMatrix, &quadIDPool);
        state.unit->SetQuadIDs(quadIDs);
        units.emplace(unitID, std::move(state));
        return true;
    }

    void Layered2DRenderer::DestroyUnit(vke_ds::id32_t unitID)
    {
        auto it = units.find(unitID);
        if (it == units.end())
            return;
        if (it->second.layerID != std::numeric_limits<vke_ds::id32_t>::max())
            RemoveUnitFromLayer(unitID, it->second.layerID);
        units.erase(it);
    }

    bool Layered2DRenderer::UpdateUnitQuadIDs(vke_ds::id32_t unitID, const std::vector<QuadID> &quadIDs)
    {
        auto it = units.find(unitID);
        if (it == units.end())
            return false;

        it->second.unit->SetQuadIDs(quadIDs);
        return true;
    }

    bool Layered2DRenderer::AddUnitToLayer(vke_ds::id32_t unitID, vke_ds::id32_t layerID)
    {
        auto unitIt = units.find(unitID);
        if (unitIt == units.end())
            return false;
        if (unitIt->second.layerID == layerID)
            return true;
        if (unitIt->second.layerID != std::numeric_limits<vke_ds::id32_t>::max())
            RemoveUnitFromLayer(unitID, unitIt->second.layerID);

        auto layerIt = layers.find(layerID);
        if (layerIt == layers.end())
        {
            layerIt = layers.emplace(layerID, std::make_unique<Layered2DRenderLayer>(layerID)).first;
            layerOrder.push_back(layerID);
        }
        layerIt->second->AddUnit(unitIt->second.material, unitIt->second.unit.get(), context);
        unitIt->second.layerID = layerID;
        return true;
    }

    void Layered2DRenderer::RemoveUnitFromLayer(vke_ds::id32_t unitID, vke_ds::id32_t layerID)
    {
        auto unitIt = units.find(unitID);
        auto layerIt = layers.find(layerID);
        if (unitIt == units.end() || layerIt == layers.end())
            return;
        layerIt->second->RemoveUnit(unitIt->second.material.get(), unitID);
        unitIt->second.layerID = std::numeric_limits<vke_ds::id32_t>::max();
    }

    void Layered2DRenderer::SetLayerOrder(const std::vector<vke_ds::id32_t> &backToFrontLayerIDs)
    {
        layerOrder.clear();
        for (vke_ds::id32_t layerID : backToFrontLayerIDs)
            if (layers.find(layerID) != layers.end())
                layerOrder.push_back(layerID);
        for (const auto &entry : layers)
            if (std::find(layerOrder.begin(), layerOrder.end(), entry.first) == layerOrder.end())
                layerOrder.push_back(entry.first);
    }

    Layered2DRenderUnit *Layered2DRenderer::GetUnit(vke_ds::id32_t unitID)
    {
        auto it = units.find(unitID);
        return it == units.end() ? nullptr : it->second.unit.get();
    }

    void Layered2DRenderer::constructFrameGraph(FrameGraph &frameGraph,
                                                std::map<std::string, vke_ds::id32_t> &blackboard,
                                                ResourceNodeIDMap &currentResourceNodeID)
    {
        const vke_ds::id32_t colorResourceID = blackboard["colorAttachment"];
        const vke_ds::id32_t outColorNodeID = frameGraph.AllocResourceNode("layered2DOutColor", colorResourceID);

        const vke_ds::id32_t taskNodeID = frameGraph.AllocTaskNode(
            "layered 2D render", RENDER_TASK,
            std::bind(&Layered2DRenderer::Render, this, std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        frameGraph.AddTaskNodeResourceRef(taskNodeID, currentResourceNodeID[colorResourceID], outColorNodeID,
                                          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        currentResourceNodeID[colorResourceID] = outColorNodeID;
    }

    void Layered2DRenderer::createDescriptorSets()
    {
        for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
        {
            rendererDescriptorSets[frame] = textShader->CreateDescriptorSet(0);
            VkDescriptorBufferInfo quadBufferInfo = quadManager->GetDescriptorBufferInfo(frame);

            VkDescriptorImageInfo staticAtlasInfo{
                font->atlasTexture->textureSampler, font->atlasTexture->textureImageView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo dynamicAtlasInfo{
                dynamicAtlasTextures[frame]->textureSampler, dynamicAtlasTextures[frame]->textureImageView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet writes[3];
            ConstructDescriptorSetWrite(writes[0], rendererDescriptorSets[frame], 0,
                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &staticAtlasInfo);
            ConstructDescriptorSetWrite(writes[1], rendererDescriptorSets[frame], 1,
                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &dynamicAtlasInfo);
            ConstructDescriptorSetWrite(writes[2], rendererDescriptorSets[frame], 2,
                                        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &quadBufferInfo);
            vkUpdateDescriptorSets(globalLogicalDevice, 3, writes, 0, nullptr);
        }
    }

    void Layered2DRenderer::Render(TaskNode &, FrameGraph &, VkCommandBuffer commandBuffer,
                                   uint32_t currentFrame, uint32_t imageIndex)
    {
        syncDynamicAtlas(commandBuffer, currentFrame);
        if (layers.empty())
            return;

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = context->colorImageViews[imageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, {context->width, context->height}};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        VkViewport viewport{0.0f, 0.0f, static_cast<float>(context->width),
                            static_cast<float>(context->height), 0.0f, 1.0f};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, {context->width, context->height}};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        const glm::vec2 viewportSize(static_cast<float>(context->width), static_cast<float>(context->height));
        const glm::vec2 atlasSize(static_cast<float>(font->atlasWidth), static_cast<float>(font->atlasHeight));
        for (vke_ds::id32_t layerID : layerOrder)
        {
            auto it = layers.find(layerID);
            if (it != layers.end())
                it->second->Render(commandBuffer, rendererDescriptorSets[currentFrame], viewportSize, atlasSize,
                                   currentFrame);
        }
        vkCmdEndRendering(commandBuffer);
    }

    void Layered2DRenderer::OnWindowResize(FrameGraph &, RenderContext *ctx)
    {
        context = ctx;
    }

    void Layered2DRenderer::syncDynamicAtlas(VkCommandBuffer commandBuffer, uint32_t currentFrame)
    {
        if (font->GetDynamicAtlasUpdateCnt() == 0)
            return;
        const auto &pixels = font->GetDynamicAtlasPixels();
        dynamicAtlasStagingBuffers[currentFrame]->ToBuffer(0, pixels.data(), pixels.size());

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = dynamicAtlasTextures[currentFrame]->textureImage;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
        RenderEnvironment::CopyBufferToImage(commandBuffer, dynamicAtlasStagingBuffers[currentFrame]->buffer,
                                             dynamicAtlasTextures[currentFrame]->textureImage,
                                             vke_common::Font::ATLAS_SIZE, vke_common::Font::ATLAS_SIZE);
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
        font->ConsumeDynamicAtlasUpdate();
    }
}
