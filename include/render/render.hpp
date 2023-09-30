#ifndef RENDERER_H
#define RENDERER_H

#include <common.hpp>
#include <render/environment.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <ds/id_allocator.hpp>

#include <map>

namespace vke_render
{
    class Renderer
    {
    };

    class RenderInfo
    {
    public:
        Material *material;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        std::vector<Mesh *> meshes;

        void AddMesh(Mesh *mesh)
        {
            meshes.push_back(mesh);
        }

        void Render(VkCommandBuffer &commandBuffer)
        {
            for (auto &mesh : meshes)
                mesh->Render(commandBuffer);
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

        static void AddMesh(Material *material, Mesh *mesh)
        {
            RegisterMaterial(material);
            instance->renderInfoMap[material].AddMesh(mesh);
        }

    private:
        RenderEnvironment *environment;
        // vke_ds::DynamicIDAllocator<uint32_t> materialIDAllocator;
        std::map<Material *, RenderInfo> renderInfoMap;

        void createRenderPass();

        void createGraphicsPipelineForMaterial(RenderInfo &renderInfo);
        void createFramebuffers();

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void drawFrame();
    };
}

#endif