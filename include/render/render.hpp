#ifndef RENDERER_H
#define RENDERER_H

#include <render/gbuffer_pass.hpp>
#include <render/deferred_lighting.hpp>
#include <render/skybox_render.hpp>
#include <event.hpp>

namespace vke_render
{

    class Renderer
    {
    private:
        static Renderer *instance;
        Renderer()
            : camInfoBuffer(sizeof(CameraInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
              cameraIDAllocator(1), currentCamera(1), cameraInfoUpdated(false), frameGraphUpdated(false) {};
        ~Renderer() {}

    public:
        RenderContext context;
        uint32_t currentFrame;
        uint32_t passcnt;

        vke_render::StagedBuffer camInfoBuffer;
        vke_ds::id32_t currentCamera;
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> cameraIDAllocator;
        std::unordered_map<vke_ds::id32_t, std::function<void()>> cameras;

        vke_common::EventHub<glm::vec2> resizeEventHub;

        static Renderer *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("Renderer not initialized!");
            return instance;
        }

        static Renderer *Init(RenderContext *ctx,
                              std::vector<PassType> &passes,
                              std::vector<std::unique_ptr<RenderPassBase>> &customPasses)
        {
            instance = new Renderer();

            instance->context = *ctx;
            instance->currentFrame = 0;
            instance->passcnt = passes.size();

            ctx->resizeEventHub->AddEventListener(instance,
                                                  vke_common::EventHub<RenderContext>::callback_t(OnWindowResize));

            instance->initDescriptorSet();

            std::map<std::string, vke_ds::id32_t> blackboard;
            std::map<vke_ds::id32_t, vke_ds::id32_t> currentResourceNodeID;
            instance->initFrameGraph(blackboard, currentResourceNodeID);

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
                    std::unique_ptr<GBufferPass> gbufferPass = std::make_unique<GBufferPass>(ctx, instance->globalDescriptorSet);
                    gbufferPass->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPassMap[GBUFFER_PASS] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(gbufferPass));
                    break;
                }
                case DEFERRED_LIGHTING_PASS:
                {
                    std::unique_ptr<DeferredLightingPass> lightingPass = std::make_unique<DeferredLightingPass>(ctx, instance->globalDescriptorSet);
                    lightingPass->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPassMap[DEFERRED_LIGHTING_PASS] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(lightingPass));
                    break;
                }
                case SKYBOX_RENDERER:
                {
                    std::unique_ptr<SkyboxRenderer> skyboxRenderer = std::make_unique<SkyboxRenderer>(ctx, instance->globalDescriptorSet);
                    skyboxRenderer->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPassMap[SKYBOX_RENDERER] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(skyboxRenderer));
                    break;
                }

                default:
                    break;
                }
            }
            instance->frameGraph->Compile();
            return instance;
        }

        static void WaitIdle()
        {
            vkDeviceWaitIdle(globalLogicalDevice);
            CPUCommandQueue *cpuQueue = (CPUCommandQueue *)RenderEnvironment::GetInstance()->commandQueues[CPU_QUEUE].get();
            cpuQueue->WaitIdle();
        }

        static void Dispose()
        {
            instance->cleanup();
            delete instance;
        }

        static vke_ds::id32_t RegisterCamera(vke_render::HostCoherentBuffer **buffer, std::function<void()> callback)
        {
            *buffer = &(instance->camInfoBuffer.stagingBuffer);
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

        static void UpdateCameraInfo(CameraInfo &cameraInfo)
        {
            instance->camInfoBuffer.stagingBuffer.ToBuffer(0, &cameraInfo, sizeof(vke_render::CameraInfo));
            instance->cameraInfoUpdated = true;
        }

        static GBufferPass *GetGBufferPass()
        {
            return static_cast<GBufferPass *>(instance->subPasses[instance->subPassMap[GBUFFER_PASS]].get());
        }

        static void OnWindowResize(void *listener, RenderContext *ctx)
        {
            instance->recreate(ctx);
            glm::vec2 extent(ctx->width, ctx->height);
            instance->resizeEventHub.DispatchEvent(&extent);
        }

        void Update();

    private:
        bool cameraInfoUpdated;
        bool frameGraphUpdated;
        vke_ds::id32_t colorAttachmentResourceID;
        vke_ds::id32_t cameraUpdateTaskID;
        vke_ds::id32_t cameraResourceNodeID;
        DescriptorSetInfo globalDescriptorSetInfo;
        VkDescriptorSet globalDescriptorSet;
        std::vector<std::unique_ptr<RenderPassBase>> subPasses;
        std::map<PassType, int> subPassMap;
        std::unique_ptr<FrameGraph> frameGraph;

        void initDescriptorSet();
        void initFrameGraph(std::map<std::string, vke_ds::id32_t> &blackboard,
                            std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void cleanup();
        void recreate(RenderContext *ctx);
        void render();
    };
}

#endif