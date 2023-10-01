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
    class Renderer
    {
    };

    struct RenderUnit
    {
        Mesh *mesh;
        VkDescriptorSet descriptorSet;

        void Render(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout)
        {
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0, 1,
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
        std::vector<RenderUnit> units;

        void AddUnit(RenderUnit unit)
        {
            units.push_back(unit);
        }

        void Render(VkCommandBuffer &commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets)
        {
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,
                globalDescriptorSets.size(),
                globalDescriptorSets.data(),
                0, nullptr);
            for (auto &unit : units)
                unit.Render(commandBuffer, pipelineLayout);
        }
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

        VkDescriptorSetLayout vpDescriptorSetLayout;
        VkDescriptorSet vpDescriptorSet;

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
            instance->createRenderPass();
            instance->createFramebuffers();
            instance->initDefaultDescriptorSet();
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
                vkDestroyPipeline(instance->environment->logicalDevice, renderInfo.second.pipeline, nullptr);
                vkDestroyPipelineLayout(instance->environment->logicalDevice, renderInfo.second.pipelineLayout, nullptr);
            }
            vkDestroyRenderPass(instance->environment->logicalDevice, instance->renderPass, nullptr);
        }

        void Update();

        static void RegisterMaterial(Material *material)
        {
            // uint32_t id = instance->materialIDAllocator.Alloc();
            auto &rMap = instance->renderInfoMap;
            if (rMap.find(material) != rMap.end())
                return;

            RenderInfo info;
            info.material = material;
            instance->createGraphicsPipelineForMaterial(info);
            rMap[material] = info;
            // return id;
        }

        static void AddUnit(Material *material, RenderUnit unit)
        {
            RegisterMaterial(material);
            instance->renderInfoMap[material].AddUnit(unit);
        }

        static void UpdateVPMatrix() {}

    private:
        RenderEnvironment *environment;
        // vke_ds::DynamicIDAllocator<uint32_t> materialIDAllocator;
        std::map<Material *, RenderInfo> renderInfoMap;

        void createRenderPass();

        void createGraphicsPipelineForMaterial(RenderInfo &renderInfo);
        void createFramebuffers();

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void drawFrame();

        void initDefaultDescriptorSet();
    };
}

#endif