#ifndef RENDERER_H
#define RENDERER_H

#include <render/gbuffer_pass.hpp>
#include <render/deferred_lighting.hpp>
#include <render/skybox_render.hpp>
#include <render/ui.hpp>
#include <render/light.hpp>
#include <render/camera.hpp>
#include <event.hpp>

namespace vke_render
{
    const uint32_t GLOBAL_DESCRIPTOR_SET_NO_LIGHT = 0;
    const uint32_t GLOBAL_DESCRIPTOR_SET_LIGHT = 1;

    class Renderer
    {
    private:
        static Renderer *instance;
        Renderer()
            : cameraIDAllocator(1), currentCamera(1), cameraInfoUpdateCnt(0), frameGraphUpdated(false) {};
        ~Renderer() {}

    public:
        RenderContext *context;
        uint32_t currentFrame;
        uint32_t passcnt;

        vke_ds::id32_t currentCamera;
        std::unique_ptr<LightManager> lightManager;
        vke_common::EventHub<glm::vec2> resizeEventHub;

        static Renderer *GetInstance()
        {
            VKE_FATAL_IF(instance == nullptr, "Renderer not initialized!")
            return instance;
        }

        static Renderer *Init(RenderContext *ctx,
                              std::vector<PassType> &passes,
                              std::vector<std::unique_ptr<RenderPassBase>> &customPasses)
        {
            instance = new Renderer();

            instance->context = ctx;
            instance->currentFrame = 0;
            instance->passcnt = passes.size();

            ctx->resizeEventHub->AddEventListener(instance,
                                                  vke_common::EventHub<RenderContext>::callback_t(OnWindowResize));

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
                instance->camInfoBuffers.emplace_back(sizeof(CameraInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

            instance->lightManager = std::make_unique<LightManager>();

            instance->initDescriptorSet();

            std::map<std::string, vke_ds::id32_t> blackboard;
            std::map<vke_ds::id32_t, vke_ds::id32_t> currentResourceNodeID;
            instance->constructFrameGraph(blackboard, currentResourceNodeID);

            instance->lightManager->ConstructFrameGraph(*(instance->frameGraph), blackboard, currentResourceNodeID);

            int customPassID = 0;
            for (int i = 0; i < passes.size(); i++)
            {
                PassType pass = passes[i];
                // RenderPassBase *customPass;
                switch (pass)
                {
                case CUSTOM_RENDERER:
                {
                    std::unique_ptr<RenderPassBase> &customPass = customPasses[customPassID++];
                    customPass->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPasses.push_back(std::move(customPass));
                    break;
                }
                case GBUFFER_PASS:
                {
                    std::unique_ptr<GBufferPass> gbufferPass = std::make_unique<GBufferPass>(ctx, instance->globalDescriptorSets[GLOBAL_DESCRIPTOR_SET_NO_LIGHT]);
                    gbufferPass->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPassMap[GBUFFER_PASS] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(gbufferPass));
                    break;
                }
                case DEFERRED_LIGHTING_PASS:
                {
                    std::unique_ptr<DeferredLightingPass> lightingPass = std::make_unique<DeferredLightingPass>(ctx, instance->globalDescriptorSets[GLOBAL_DESCRIPTOR_SET_LIGHT], instance->lightManager.get());
                    lightingPass->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPassMap[DEFERRED_LIGHTING_PASS] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(lightingPass));
                    break;
                }
                case SKYBOX_RENDERER:
                {
                    std::unique_ptr<SkyboxRenderer> skyboxRenderer = std::make_unique<SkyboxRenderer>(ctx, instance->globalDescriptorSets[GLOBAL_DESCRIPTOR_SET_NO_LIGHT]);
                    skyboxRenderer->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPassMap[SKYBOX_RENDERER] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(skyboxRenderer));
                    break;
                }
                case UI_RENDERER:
                {
                    std::unique_ptr<UIRenderer> uiRenderer = std::make_unique<UIRenderer>(ctx, instance->globalDescriptorSets[GLOBAL_DESCRIPTOR_SET_NO_LIGHT]);
                    uiRenderer->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPassMap[UI_RENDERER] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(uiRenderer));
                    break;
                }

                default:
                    break;
                }
            }
            instance->frameGraph->Compile();
            return instance;
        }

        static void Shutdown()
        {
            uint32_t imageIndex = instance->context->AcquireNextImage(instance->currentFrame);
            instance->frameGraph->Sync(instance->currentFrame);
        }

        static void WaitIdle()
        {
            VKE_LOG_INFO("WAIT_IDLE0")
            vkDeviceWaitIdle(globalLogicalDevice);
            VKE_LOG_INFO("WAIT_IDLE1")
            CPUCommandQueue *cpuQueue = (CPUCommandQueue *)RenderEnvironment::GetInstance()->commandQueues[CPU_QUEUE].get();
            cpuQueue->WaitIdle();
            VKE_LOG_INFO("WAIT_IDLE2")
        }

        static void Dispose()
        {
            instance->cleanup();
            delete instance;
        }

        static vke_ds::id32_t RegisterCamera(std::function<void()> callback)
        {
            vke_ds::id32_t ret = instance->cameraIDAllocator.Alloc();
            instance->cameras[ret] = callback;
            if (ret == instance->currentCamera)
                callback();
            return ret;
        }

        static void RemoveCamera(vke_ds::id32_t id)
        {
            instance->cameras.erase(id);
            if (instance->cameras.size() > 0)
                SetCurrentCamera(0);
        }

        static void SetCurrentCamera(vke_ds::id32_t id)
        {
            if (id != instance->currentCamera)
            {
                auto it = instance->cameras.find(id);
                if (it != instance->cameras.end())
                {
                    instance->currentCamera = id;
                    it->second();
                }
            }
        }

        static void UpdateCameraInfo(CameraInfo cameraInfo)
        {
            instance->hostCameraInfo = cameraInfo;
            instance->cameraInfoUpdateCnt = 2;
        }

        static void AddRenderUpdateCallback(vke_ds::id64_t id, std::function<void(uint32_t)> callback)
        {
            instance->renderUpdateCallbacks[id] = callback;
        }

        static void RemoveRenderUpdateCallback(vke_ds::id64_t id)
        {
            instance->renderUpdateCallbacks.erase(id);
        }

        static GBufferPass *GetGBufferPass()
        {
            return static_cast<GBufferPass *>(instance->subPasses[instance->subPassMap[GBUFFER_PASS]].get());
        }

        static void OnWindowResize(void *listener, RenderContext *ctx)
        {
            instance->recreate(ctx);
            glm::vec2 extent(ctx->width, ctx->height);
            for (auto &subpass : instance->subPasses)
                subpass->OnWindowResize(*instance->frameGraph, ctx);
            instance->frameGraphUpdated = true;
            instance->resizeEventHub.DispatchEvent(&extent);
        }

        void Update();

    private:
        bool frameGraphUpdated;
        vke_ds::id32_t colorAttachmentResourceID;
        vke_ds::id32_t depthAttachmentResourceID;
        DescriptorSetInfo globalDescriptorSetInfos[2]; // 0 no light, 1 light
        VkDescriptorSet globalDescriptorSets[2][MAX_FRAMES_IN_FLIGHT];
        std::vector<std::unique_ptr<RenderPassBase>> subPasses;
        std::map<PassType, int> subPassMap;
        std::unique_ptr<FrameGraph> frameGraph;

        uint32_t cameraInfoUpdateCnt;
        CameraInfo hostCameraInfo;
        std::vector<vke_render::HostCoherentBuffer> camInfoBuffers;
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> cameraIDAllocator;
        std::unordered_map<vke_ds::id32_t, std::function<void()>> cameras;

        std::unordered_map<vke_ds::id64_t, std::function<void(uint32_t)>> renderUpdateCallbacks;

        void initDescriptorSet();
        void constructFrameGraph(std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void cleanup();
        void recreate(RenderContext *ctx);
        void render();
    };
}

#endif