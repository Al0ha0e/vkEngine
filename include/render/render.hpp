#ifndef RENDERER_H
#define RENDERER_H

#include <common.hpp>
#include <render/environment.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <ds/id_allocator.hpp>
#include <vector>
#include <map>

namespace vke_render
{
    VkWriteDescriptorSet ConstructDescriptorSetWrite(VkDescriptorSet descriptorSet, DescriptorInfo &descriptorInfo, VkDescriptorBufferInfo *bufferInfo);

    class Renderer
    {
    };

    struct CameraInfo
    {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 viewPos;

        CameraInfo() = default;

        CameraInfo(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos)
            : view(view), projection(projection), viewPos(viewPos, 1.0) {}
    };

    struct RenderUnit
    {
        Mesh *mesh;
        VkDescriptorSet descriptorSet;

        RenderUnit() = default;

        RenderUnit(Mesh *msh) : mesh(msh), descriptorSet(nullptr) {}

        RenderUnit(
            Mesh *msh,
            DescriptorSetInfo &descriptorSetInfo,
            std::vector<DescriptorInfo> &descriptorInfos,
            std::vector<VkBuffer> &buffers) : mesh(msh)
        {
            descriptorSet = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(descriptorSetInfo);
            int descriptorCnt = descriptorInfos.size();
            std::vector<VkDescriptorBufferInfo> bufferInfos(descriptorCnt);
            std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorCnt);
            for (int i = 0; i < descriptorCnt; i++)
            {
                DescriptorInfo &info = descriptorInfos[i];
                VkDescriptorBufferInfo &bufferInfo = bufferInfos[i];
                bufferInfo.buffer = buffers[i];
                bufferInfo.offset = 0;
                bufferInfo.range = info.bufferSize;

                descriptorWrites[i] = ConstructDescriptorSetWrite(descriptorSet, info, bufferInfos.data() + i);
            }
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, descriptorCnt, descriptorWrites.data(), 0, nullptr);
        }

        void Render(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, int descriptorSetOffset)
        {
            if (descriptorSet)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    descriptorSetOffset,
                    1,
                    &descriptorSet,
                    0, nullptr);

            mesh->Render(commandBuffer);
        }
    };

    class RenderInfo
    {
    public:
        Material *material;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        DescriptorSetInfo commonDescriptorSetInfo;
        VkDescriptorSet commonDescriptorSet;
        DescriptorSetInfo perUnitDescriptorSetInfo;
        bool hasCommonDescriptorSet;
        bool hasPerUnitDescriptorSet;
        std::map<uint64_t, RenderUnit> units;

        RenderInfo() = default;

        RenderInfo(Material *mat)
            : material(mat),
              commonDescriptorSet(nullptr)
        {
            hasCommonDescriptorSet = mat->commonDescriptorInfos.size() > 0;
            hasPerUnitDescriptorSet = mat->perUnitDescriptorInfos.size() > 0;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            if (hasCommonDescriptorSet)
            {
                std::vector<DescriptorInfo> &commonDescriptorInfos = mat->commonDescriptorInfos;
                commonDescriptorSetInfo.uniformDescriptorCnt = 0;
                std::vector<VkDescriptorSetLayoutBinding> commonBindings;
                for (auto &dInfo : commonDescriptorInfos)
                {
                    if (dInfo.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                        commonDescriptorSetInfo.uniformDescriptorCnt++;
                    commonBindings.push_back(dInfo.bindingInfo);
                }

                layoutInfo.bindingCount = commonBindings.size();
                layoutInfo.pBindings = commonBindings.data();

                if (vkCreateDescriptorSetLayout(
                        RenderEnvironment::GetInstance()->logicalDevice,
                        &layoutInfo,
                        nullptr,
                        &(commonDescriptorSetInfo.layout)) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create descriptor set layout!");
                }
                commonDescriptorSet = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(commonDescriptorSetInfo);

                int descriptorCnt = commonDescriptorInfos.size();
                std::vector<VkDescriptorBufferInfo> bufferInfos(descriptorCnt);
                std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorCnt);
                for (int i = 0; i < descriptorCnt; i++)
                {
                    DescriptorInfo &info = commonDescriptorInfos[i];
                    VkDescriptorBufferInfo &bufferInfo = bufferInfos[i];
                    bufferInfo.buffer = material->commonBuffers[i];
                    bufferInfo.offset = 0;
                    bufferInfo.range = info.bufferSize;

                    descriptorWrites[i] = ConstructDescriptorSetWrite(commonDescriptorSet, info, &bufferInfo);
                }
                vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, descriptorCnt, descriptorWrites.data(), 0, nullptr);
            }

            if (hasPerUnitDescriptorSet)
            {
                perUnitDescriptorSetInfo.uniformDescriptorCnt = 0;
                std::vector<VkDescriptorSetLayoutBinding> perUnitBindings;
                for (auto &dInfo : mat->perUnitDescriptorInfos)
                {
                    if (dInfo.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                        perUnitDescriptorSetInfo.uniformDescriptorCnt++;
                    perUnitBindings.push_back(dInfo.bindingInfo);
                }

                layoutInfo.bindingCount = perUnitBindings.size();
                layoutInfo.pBindings = perUnitBindings.data();

                if (vkCreateDescriptorSetLayout(
                        RenderEnvironment::GetInstance()->logicalDevice,
                        &layoutInfo,
                        nullptr,
                        &(perUnitDescriptorSetInfo.layout)) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create descriptor set layout!");
                }
            }
        }

        ~RenderInfo()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            if (hasCommonDescriptorSet)
                vkDestroyDescriptorSetLayout(logicalDevice, commonDescriptorSetInfo.layout, nullptr);
            if (hasPerUnitDescriptorSet)
                vkDestroyDescriptorSetLayout(logicalDevice, perUnitDescriptorSetInfo.layout, nullptr);
        }

        void ApplyToPipeline(VkPipelineVertexInputStateCreateInfo &vertexInputInfo,
                             VkPipelineShaderStageCreateInfo *&stages,
                             std::vector<VkDescriptorSetLayout> &globalDescriptorSetLayouts)
        {
            material->ApplyToPipeline(vertexInputInfo, stages);

            if (hasCommonDescriptorSet)
                globalDescriptorSetLayouts.push_back(commonDescriptorSetInfo.layout);
            if (hasPerUnitDescriptorSet)
                globalDescriptorSetLayouts.push_back(perUnitDescriptorSetInfo.layout);
        }

        uint64_t AddUnit(Mesh *mesh, std::vector<VkBuffer> &buffers)
        {
            uint64_t id = allocator.Alloc();
            units[id] = hasPerUnitDescriptorSet ? RenderUnit(mesh, perUnitDescriptorSetInfo, material->perUnitDescriptorInfos, buffers)
                                                : RenderUnit(mesh);
            return id;
        }

        void Render(VkCommandBuffer &commandBuffer, VkDescriptorSet *globalDescriptorSet)
        {
            int setcnt = 0;
            if (*globalDescriptorSet)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    setcnt++,
                    1,
                    globalDescriptorSet,
                    0, nullptr);

            if (hasCommonDescriptorSet)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    setcnt++,
                    1,
                    &commonDescriptorSet,
                    0, nullptr);
            for (auto &unit : units)
                unit.second.Render(commandBuffer, pipelineLayout, setcnt);
        }

    private:
        vke_ds::NaiveIDAllocator<uint64_t> allocator;
    };

    class OpaqueRenderer
    {
    private:
        static OpaqueRenderer *instance;
        // OpaqueRenderer() : materialIDAllocator(10) {}
        OpaqueRenderer() = default;
        ~OpaqueRenderer() {}

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (OpaqueRenderer::instance != nullptr)
                    delete OpaqueRenderer::instance;
            }
        };
        static Deletor deletor;

    public:
        VkRenderPass renderPass;
        // VkPipelineLayout pipelineLayout;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        uint32_t currentFrame;

        std::vector<DescriptorInfo> globalDescriptorInfos;
        DescriptorSetInfo globalDescriptorSetInfo;
        VkDescriptorSet globalDescriptorSet;

        static OpaqueRenderer *GetInstance()
        {
            if (instance == nullptr)
                instance = new OpaqueRenderer();
            return instance;
        }

        static OpaqueRenderer *Init()
        {
            instance = new OpaqueRenderer();
            instance->currentFrame = 0;
            instance->environment = RenderEnvironment::GetInstance();
            instance->createGlobalDescriptorSet();
            instance->createRenderPass();
            instance->createFramebuffers();
            return instance;
        }

        static void Dispose()
        {
            for (auto framebuffer : instance->swapChainFramebuffers)
            {
                vkDestroyFramebuffer(instance->environment->logicalDevice, framebuffer, nullptr);
            }
            for (auto &renderInfo : instance->renderInfoMap)
            {
                vkDestroyPipeline(instance->environment->logicalDevice, renderInfo.second->pipeline, nullptr);
                vkDestroyPipelineLayout(instance->environment->logicalDevice, renderInfo.second->pipelineLayout, nullptr);
            }
            vkDestroyRenderPass(instance->environment->logicalDevice, instance->renderPass, nullptr);
        }

        void Update();

        static void RegisterCamera(VkBuffer buffer)
        {
            DescriptorInfo &info = instance->globalDescriptorInfos[0];
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = info.bufferSize;
            VkWriteDescriptorSet descriptorWrite = ConstructDescriptorSetWrite(instance->globalDescriptorSet, info, &bufferInfo);
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }

        static void RegisterMaterial(Material *material)
        {
            // uint32_t id = instance->materialIDAllocator.Alloc();
            auto &rMap = instance->renderInfoMap;
            if (rMap.find(material) != rMap.end())
                return;

            RenderInfo *info = new RenderInfo(material);
            instance->createGraphicsPipeline(*info);
            rMap[material] = info;
            // return id;
        }

        static uint64_t AddUnit(Material *material, Mesh *mesh, std::vector<VkBuffer> &buffers)
        {
            RegisterMaterial(material);
            return instance->renderInfoMap[material]->AddUnit(mesh, buffers);
        }

    private:
        RenderEnvironment *environment;
        // vke_ds::DynamicIDAllocator<uint32_t> materialIDAllocator;
        std::map<Material *, RenderInfo *> renderInfoMap;

        void createGlobalDescriptorSet();
        void createRenderPass();
        void createGraphicsPipeline(RenderInfo &renderInfo);
        void createFramebuffers();

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void drawFrame();
    };
}

#endif