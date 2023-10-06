#ifndef OPAQUE_RENDER_H
#define OPAQUE_RENDER_H

#include <render/render.hpp>
#include <render/renderinfo.hpp>

namespace vke_render
{
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