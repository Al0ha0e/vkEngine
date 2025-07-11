#ifndef RENDERER_H
#define RENDERER_H

#include <common.hpp>
#include <render/environment.hpp>
#include <render/base_render.hpp>
#include <render/opaque_render.hpp>
#include <render/frame_graph.hpp>
#include <event.hpp>
#include <vector>
#include <map>

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
                              std::vector<std::unique_ptr<RenderPassBase>> &customPasses,
                              std::vector<RenderPassInfo> &customPassInfo)
        {
            instance = new Renderer();

            instance->context = *ctx;
            instance->currentFrame = 0;
            instance->logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
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
                case BASE_RENDERER:
                {
                    std::unique_ptr<BaseRenderer> baseRenderer = std::make_unique<BaseRenderer>(ctx, instance->globalDescriptorSet);
                    baseRenderer->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPassMap[BASE_RENDERER] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(baseRenderer));
                    break;
                }
                case OPAQUE_RENDERER:
                {
                    std::unique_ptr<OpaqueRenderer> opaqueRenderer = std::make_unique<OpaqueRenderer>(ctx, instance->globalDescriptorSet);
                    opaqueRenderer->Init(i, *(instance->frameGraph), blackboard, currentResourceNodeID);
                    instance->subPassMap[OPAQUE_RENDERER] = instance->subPasses.size();
                    instance->subPasses.push_back(std::move(opaqueRenderer));
                    break;
                }
                default:
                    break;
                }
            }
            instance->frameGraph->Compile();
            return instance;
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

        static OpaqueRenderer *GetOpaqueRenderer()
        {
            return static_cast<OpaqueRenderer *>(instance->subPasses[instance->subPassMap[OPAQUE_RENDERER]].get());
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
        VkDevice logicalDevice;
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