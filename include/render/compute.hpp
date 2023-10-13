#ifndef COMPUTE_H
#define COMPUTE_H

#include <render/environment.hpp>
#include <render/shader.hpp>
#include <render/descriptor.hpp>

namespace vke_render
{
    class ComputeInfo
    {
    public:
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        ComputeShader *shader;
        DescriptorSetInfo dscriptorSetInfo;
        VkDescriptorSet descriptorSet;

        ComputeInfo() = default;
        void Dispatch()
        {
        }
    };

    class ComputeManager
    {
    private:
        static ComputeManager *instance;
        // OpaqueRenderer() : materialIDAllocator(10) {}
        ComputeManager() = default;
        ~ComputeManager() {}

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (ComputeManager::instance != nullptr)
                    delete ComputeManager::instance;
            }
        };
        static Deletor deletor;

    public:
        static ComputeManager *GetInstance()
        {
            if (instance == nullptr)
                instance = new ComputeManager();
            return instance;
        }

        static ComputeManager *Init(int subpassID, VkRenderPass renderPass)
        {
            instance = new OpaqueRenderer();
            instance->subpassID = subpassID;
            instance->renderPass = renderPass;
            instance->environment = RenderEnvironment::GetInstance();
            instance->createGlobalDescriptorSet();
            return instance;
        }

    private:
        std::vector<ComputeInfo> computeTasks;
        void createGraphicsPipeline(ComputeInfo &computeInfo);
    };
}

#endif