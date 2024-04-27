#ifndef RENDERER_H
#define RENDERER_H

#include <common.hpp>
#include <render/environment.hpp>
#include <render/base_render.hpp>
#include <render/opaque_render.hpp>
#include <event.hpp>
#include <vector>
#include <map>

namespace vke_render
{

    class Renderer
    {
    private:
        static Renderer *instance;
        Renderer() = default;
        ~Renderer() {}

    public:
        uint32_t currentFrame;
        uint32_t imageCnt;
        uint32_t passcnt;

        static Renderer *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("Renderer not initialized!");
            return instance;
        }

        static Renderer *Init(std::vector<PassType> &passes, std::vector<std::unique_ptr<SubpassBase>> &customPasses, std::vector<RenderPassInfo> &customPassInfo)
        {
            instance = new Renderer();
            instance->framebufferResized = false;
            instance->currentFrame = 0;
            instance->environment = RenderEnvironment::GetInstance();
            instance->passcnt = passes.size();

            instance->createSwapChain();
            instance->createImageViews();

            std::vector<RenderPassInfo> passInfo;
            int customPassID = 0;
            for (int i = 0; i < passes.size(); i++)
            {
                PassType pass = passes[i];
                switch (pass)
                {
                case CUSTOM_RENDERER:
                    passInfo.push_back(customPassInfo[customPassID++]);
                    break;
                case BASE_RENDERER:
                    passInfo.push_back(RenderPassInfo(
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT));
                    break;
                case OPAQUE_RENDERER:
                    passInfo.push_back(RenderPassInfo(
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT));
                    break;
                default:
                    break;
                }
            }
            instance->createRenderPass(passInfo);
            instance->createFramebuffers();

            customPassID = 0;
            for (int i = 0; i < passes.size(); i++)
            {
                PassType pass = passes[i];
                // SubpassBase *customPass;
                switch (pass)
                {
                case CUSTOM_RENDERER:
                {
                    std::unique_ptr<SubpassBase> &customPass = customPasses[customPassID++];
                    customPass->Init(i, instance->renderPass);
                    instance->subPasses.push_back(std::move(customPass));
                    break;
                }
                case BASE_RENDERER:
                { // instance->baseRenderer = new BaseRenderer(i, instance->renderPass->renderPass);
                    std::unique_ptr<BaseRenderer> baseRenderer = std::make_unique<BaseRenderer>();
                    baseRenderer->Init(i, instance->renderPass);
                    instance->subPassMap[BASE_RENDERER] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(baseRenderer));
                    break;
                }
                case OPAQUE_RENDERER:
                { // instance->opaqueRenderer = new OpaqueRenderer(i, instance->renderPass->renderPass);
                    std::unique_ptr<OpaqueRenderer> opaqueRenderer = std::make_unique<OpaqueRenderer>();
                    opaqueRenderer->Init(i, instance->renderPass);
                    instance->subPassMap[OPAQUE_RENDERER] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(opaqueRenderer));
                    break;
                }
                default:
                    break;
                }
            }

            vke_common::EventSystem::AddEventListener(vke_common::EVENT_WINDOW_RESIZE, instance, vke_common::EventCallback(OnWindowResize));
            return instance;
        }

        static void Dispose()
        {
            vkDestroyRenderPass(instance->environment->logicalDevice, instance->renderPass, nullptr);
            instance->cleanupSwapChain();
            delete instance;
        }

        static void RegisterCamera(VkBuffer buffer)
        {
            for (auto &pass : instance->subPasses)
                pass->RegisterCamera(buffer);
        }

        static OpaqueRenderer *GetOpaqueRenderer()
        {
            return static_cast<OpaqueRenderer *>(instance->subPasses[instance->subPassMap[OPAQUE_RENDERER]].get());
        }

        static void OnWindowResize(void *listener, void *info)
        {
            instance->framebufferResized = true;
        }

        void Update();

    private:
        VkRenderPass renderPass;

        bool framebufferResized;
        RenderEnvironment *environment;

        std::vector<VkFramebuffer> frameBuffers;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;

        std::vector<VkImageView> swapChainImageViews;
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;
        std::vector<std::unique_ptr<SubpassBase>> subPasses;
        std::map<PassType, int> subPassMap;

        void createSwapChain();
        void cleanupSwapChain();
        void recreateSwapChain();
        void createImageViews();
        void createRenderPass(std::vector<RenderPassInfo> &passInfo);
        void createFramebuffers();
        void render();
    };
}

#endif