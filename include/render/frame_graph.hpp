#ifndef FRAME_GRAPH_H
#define FRAME_GRAPH_H

#include <unordered_set>
#include <render/environment.hpp>
#include <render/command_pool.hpp>
#include <render/semaphore_pool.hpp>
#include <render/transient_memory.hpp>
#include <ds/id_allocator.hpp>
#include <logger.hpp>

namespace vke_render
{
    using ResourceNodeIDMap = std::map<vke_ds::id32_t, vke_ds::id32_t>;

    class ResourceNode
    {
    public:
        std::string name;
        vke_ds::id32_t resourceNodeID;
        vke_ds::id32_t resourceID;
        vke_ds::id32_t srcTaskID;
        std::vector<vke_ds::id32_t> dstTaskIDs;

        ResourceNode() : resourceNodeID(0), resourceID(0), srcTaskID(0) {}
        ResourceNode(std::string &&name, const vke_ds::id32_t nodeID, const vke_ds::id32_t resourceID)
            : name(std::move(name)), resourceNodeID(nodeID), resourceID(resourceID), srcTaskID(0) {}
        ~ResourceNode() {}
        ResourceNode &operator=(const ResourceNode &) = delete;
        ResourceNode(const ResourceNode &) = delete;

        ResourceNode &operator=(ResourceNode &&ano)
        {
            if (this != &ano)
            {
                name = std::move(ano.name);
                resourceNodeID = ano.resourceNodeID;
                resourceID = ano.resourceID;
                srcTaskID = ano.srcTaskID;
                dstTaskIDs = std::move(ano.dstTaskIDs);
            }
            return *this;
        }

        ResourceNode(ResourceNode &&ano)
            : name(std::move(ano.name)), resourceNodeID(ano.resourceNodeID), resourceID(ano.resourceID),
              srcTaskID(ano.srcTaskID), dstTaskIDs(std::move(ano.dstTaskIDs)) {}
    };

    class TaskNode;

    class ResourceRef
    {
    public:
        vke_ds::id32_t inResourceNodeID;
        vke_ds::id32_t outResourceNodeID;
        VkAccessFlags2 accessMask;
        VkPipelineStageFlags2 stageMask;
        VkImageLayout imageLayout;
        VkAttachmentLoadOp loadOp;
        VkAttachmentStoreOp storeOp;
        TaskNode *crossQueueTask; // ref used by the next task in another queue ? ref : nullptr (nothing to do with in/output)

        ResourceRef() : inResourceNodeID(0), outResourceNodeID(0), accessMask(VK_ACCESS_NONE),
                        stageMask(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT), imageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
                        loadOp(VK_ATTACHMENT_LOAD_OP_DONT_CARE), storeOp(VK_ATTACHMENT_STORE_OP_DONT_CARE),
                        crossQueueTask(nullptr)
        {
        }

        ResourceRef(const vke_ds::id32_t inResourceNodeID, const vke_ds::id32_t outResourceNodeID,
                    const VkAccessFlags2 accessMask, const VkPipelineStageFlags2 stageMask,
                    const VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
                    const VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    const VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE)
            : inResourceNodeID(inResourceNodeID), outResourceNodeID(outResourceNodeID),
              accessMask(accessMask), stageMask(stageMask), imageLayout(layout),
              loadOp(loadOp), storeOp(storeOp), crossQueueTask(nullptr) {}

        vke_ds::id32_t GetResourceNodeID() { return inResourceNodeID == 0 ? outResourceNodeID : inResourceNodeID; }
    };

    enum TaskType
    {
        RENDER_TASK = 0,
        COMPUTE_TASK,
        TRANSFER_TASK,
        CPU_TASK,
        TASK_TYPE_CNT
    };

    class FrameGraph;

    using TaskNodeExecuteCallback = std::function<void(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)>;
    using TaskNodeTransientReadyCallback = std::function<void(TaskNode &node, FrameGraph &frameGraph, uint32_t currentFrame)>;

    class TaskNode
    {
    public:
        std::string name;
        vke_ds::id32_t taskID;
        TaskType taskType;
        TaskType actualTaskType;
        bool valid;
        bool needQueueSubmit;
        bool isFinalTask;
        uint32_t indeg;
        uint64_t lastSemaphoreValue;
        uint64_t currentSemaphoreValue;
        VkSemaphore lastSemaphore;
        VkSemaphore currentSemaphore;
        std::unordered_map<vke_ds::id32_t, ResourceRef> resourceRefs;
        TaskNodeExecuteCallback executeCallback;
        TaskNodeTransientReadyCallback transientReadyCallback;

        TaskNode() : taskID(0), taskType(RENDER_TASK), valid(false),
                     needQueueSubmit(false), isFinalTask(false), indeg(0),
                     lastSemaphoreValue(0), currentSemaphoreValue(0), lastSemaphore(nullptr), currentSemaphore(nullptr) {}

        TaskNode(std::string &&name, const TaskType type, const TaskType actualTaskType)
            : name(std::move(name)), taskID(0), taskType(type), actualTaskType(actualTaskType),
              valid(false), needQueueSubmit(false), isFinalTask(false),
              indeg(0), lastSemaphoreValue(0), currentSemaphoreValue(0), lastSemaphore(nullptr), currentSemaphore(nullptr) {}

        TaskNode(std::string &&name, const vke_ds::id32_t id, const TaskType type, const TaskType actualTaskType)
            : name(std::move(name)), taskID(id), taskType(type), actualTaskType(actualTaskType),
              valid(false), needQueueSubmit(false), isFinalTask(false),
              indeg(0), lastSemaphoreValue(0), currentSemaphoreValue(0), lastSemaphore(nullptr), currentSemaphore(nullptr) {}

        TaskNode(std::string &&name, const vke_ds::id32_t id, const TaskType type, const TaskType actualTaskType, const TaskNodeExecuteCallback &callback)
            : name(std::move(name)), taskID(id), taskType(type), actualTaskType(actualTaskType),
              valid(false), needQueueSubmit(false), isFinalTask(false),
              indeg(0), lastSemaphoreValue(0), currentSemaphoreValue(0), lastSemaphore(nullptr), currentSemaphore(nullptr), executeCallback(callback) {}

        ~TaskNode() {}
        TaskNode &operator=(const TaskNode &) = delete;
        TaskNode(const TaskNode &) = delete;

        TaskNode &operator=(TaskNode &&ano)
        {
            if (this != &ano)
            {
                name = std::move(ano.name);
                taskID = ano.taskID;
                taskType = ano.taskType;
                actualTaskType = ano.actualTaskType;
                valid = ano.valid;
                needQueueSubmit = ano.needQueueSubmit;
                isFinalTask = ano.isFinalTask;
                indeg = ano.indeg;
                lastSemaphoreValue = ano.lastSemaphoreValue;
                currentSemaphoreValue = ano.currentSemaphoreValue;
                lastSemaphore = ano.lastSemaphore;
                currentSemaphore = ano.currentSemaphore;
                resourceRefs = std::move(ano.resourceRefs);
                executeCallback = std::move(ano.executeCallback);
                transientReadyCallback = std::move(ano.transientReadyCallback);
            }
            return *this;
        }

        TaskNode(TaskNode &&ano)
            : name(std::move(ano.name)), taskID(ano.taskID), taskType(ano.taskType), actualTaskType(ano.actualTaskType),
              valid(ano.valid), needQueueSubmit(ano.needQueueSubmit), isFinalTask(ano.isFinalTask), indeg(ano.indeg),
              lastSemaphoreValue(ano.lastSemaphoreValue), currentSemaphoreValue(ano.currentSemaphoreValue),
              lastSemaphore(ano.lastSemaphore), currentSemaphore(ano.currentSemaphore),
              resourceRefs(std::move(ano.resourceRefs)), executeCallback(std::move(ano.executeCallback)),
              transientReadyCallback(std::move(ano.transientReadyCallback)) {}

        void AddResourceRef(const vke_ds::id32_t resourceID,
                            const vke_ds::id32_t inResourceNodeID, const vke_ds::id32_t outResourceNodeID,
                            const VkAccessFlags2 accessMask, const VkPipelineStageFlags2 stageMask,
                            const VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
                            const VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            const VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE)
        {
            resourceRefs.emplace(resourceID, ResourceRef(inResourceNodeID, outResourceNodeID,
                                                         accessMask, stageMask, layout, loadOp, storeOp));
        }

        void Reset()
        {
            valid = false;
            needQueueSubmit = false;
            isFinalTask = false;
            indeg = 0;
            ResetCurrentSemaphore();
        }

        void ResetCurrentSemaphore()
        {
            currentSemaphore = nullptr;
            currentSemaphoreValue = 0;
        }

        bool HasCurrentSemaphore() const { return currentSemaphore != nullptr; }
        bool HasLastSemaphore() const { return lastSemaphore != nullptr; }
    };

    enum ResourceType
    {
        IMAGE_RESOURCE,
        BUFFER_RESOURCE,
    };

    class RenderResource
    {
    public:
        std::string name;
        vke_ds::id32_t resourceID;
        ResourceType resourceType;
        bool isTransient;
        bool framesInFlight;
        vke_ds::id32_t firstAccessTaskID;
        vke_ds::id32_t lastAccessTaskID;
        ResourceRef *prevUsedRef;
        TaskNode *prevUsedTask;
        bool prevWrite;
        ResourceRef *tmpPrevUsedRef;
        TaskNode *tmpPrevUsedTask;
        std::unordered_set<vke_ds::id32_t> resourceNodeIDs;

        RenderResource() : resourceID(0), resourceType(IMAGE_RESOURCE),
                           firstAccessTaskID(0), lastAccessTaskID(0), prevUsedRef(nullptr), prevUsedTask(nullptr), prevWrite(false), isTransient(false), framesInFlight(false) {}
        RenderResource(const ResourceType type) : resourceID(0), resourceType(type),
                                                  firstAccessTaskID(0), lastAccessTaskID(0), prevUsedRef(nullptr), prevUsedTask(nullptr), prevWrite(false), isTransient(false), framesInFlight(false) {}
        RenderResource(std::string &&name, const vke_ds::id32_t id, const ResourceType type, const bool isTransient, const bool framesInFlight)
            : name(name), resourceID(id), resourceType(type), isTransient(isTransient), framesInFlight(framesInFlight),
              firstAccessTaskID(0), lastAccessTaskID(0), prevUsedRef(nullptr), prevUsedTask(nullptr), prevWrite(false) {}

        uint32_t GetCurrentSubIdx(uint32_t currentFrame, uint32_t) { return framesInFlight ? currentFrame : 0; }

        void ResetTmpValues()
        {
            firstAccessTaskID = 0;
            lastAccessTaskID = 0;
            tmpPrevUsedRef = 0;
            tmpPrevUsedTask = 0;
        }

        void ResetPrev()
        {
            prevWrite = false;
            prevUsedRef = nullptr;
            prevUsedTask = nullptr;
        }
    };

    class ImageResource : public RenderResource
    {
    public:
        VkImageAspectFlags aspectMask;
        uint32_t mipLevelCnt;
        uint32_t layerCnt;
        bool dependOnSwapchain;
        std::vector<VkImage> images;

        ImageResource()
            : RenderResource(IMAGE_RESOURCE), aspectMask(VK_IMAGE_ASPECT_NONE), dependOnSwapchain(false),
              mipLevelCnt(1), layerCnt(1)
        {
            images.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
        }
        ImageResource(std::string &&name, const vke_ds::id32_t id, const bool isTransient, const bool framesInFlight, VkImage *imgs,
                      const VkImageAspectFlags aspect, const bool dependOnSwapchain)
            : RenderResource(std::move(name), id, IMAGE_RESOURCE, isTransient, framesInFlight), aspectMask(aspect), dependOnSwapchain(dependOnSwapchain),
              mipLevelCnt(1), layerCnt(1)
        {
            int cnt = dependOnSwapchain ? RenderEnvironment::GetInstance()->imageCnt : (framesInFlight ? MAX_FRAMES_IN_FLIGHT : 1);
            images.resize(cnt, VK_NULL_HANDLE);
            for (int i = 0; i < cnt; ++i)
                images[i] = imgs[i];

            VKE_LOG_DEBUG("IMAGE RESOURCE {}", name)
        }

        ImageResource(std::string &&name, const vke_ds::id32_t id, const bool isTransient, const bool framesInFlight, VkImage *imgs,
                      const VkImageAspectFlags aspect, uint32_t mipLevelCnt, uint32_t layerCnt, const bool dependOnSwapchain)
            : RenderResource(std::move(name), id, IMAGE_RESOURCE, isTransient, framesInFlight), aspectMask(aspect), dependOnSwapchain(dependOnSwapchain),
              mipLevelCnt(mipLevelCnt), layerCnt(layerCnt)
        {
            int cnt = dependOnSwapchain ? RenderEnvironment::GetInstance()->imageCnt : (framesInFlight ? MAX_FRAMES_IN_FLIGHT : 1);
            images.resize(cnt, VK_NULL_HANDLE);
            for (int i = 0; i < cnt; ++i)
                images[i] = imgs[i];

            VKE_LOG_DEBUG("IMAGE RESOURCE {}", name)
        }

        uint32_t GetCurrentSubIdx(uint32_t currentFrame, uint32_t imageIndex)
        {
            return dependOnSwapchain ? imageIndex : RenderResource::GetCurrentSubIdx(currentFrame, imageIndex);
        }

        VkImage GetCurrentImage(uint32_t currentFrame, uint32_t imageIndex) { return images[GetCurrentSubIdx(currentFrame, imageIndex)]; }
    };

    class BufferResource : public RenderResource
    {
    public:
        VkBuffer buffers[MAX_FRAMES_IN_FLIGHT];
        VkDeviceSize offset;
        VkDeviceSize size;

        BufferResource()
            : RenderResource(BUFFER_RESOURCE), offset(0), size(0)
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
                buffers[i] = 0;
        }
        BufferResource(std::string &&name, const vke_ds::id32_t id, const bool isTransient, const bool framesInFlight,
                       VkBuffer *buffs, const VkDeviceSize offset, const VkDeviceSize size)
            : RenderResource(std::move(name), id, BUFFER_RESOURCE, isTransient, framesInFlight), offset(offset), size(size)
        {
            int cnt = framesInFlight ? MAX_FRAMES_IN_FLIGHT : 1;
            for (int i = 0; i < cnt; ++i)
                buffers[i] = buffs[i];
            VKE_LOG_DEBUG("BUFFER RESOURCE {}", name)
        }

        VkBuffer GetCurrentBuffer(uint32_t currentFrame, uint32_t imageIndex) { return buffers[GetCurrentSubIdx(currentFrame, imageIndex)]; }
    };

    struct PermanentResourceState
    {
        VkPipelineStageFlags2 stStage;
        std::optional<VkImageLayout> stImageLayout;
        std::optional<VkImageLayout> enImageLayout;

        PermanentResourceState() {}
        PermanentResourceState(const VkPipelineStageFlags2 stStage, const std::optional<VkImageLayout> stLayout, const std::optional<VkImageLayout> enLayout)
            : stStage(stStage), stImageLayout(stLayout), enImageLayout(enLayout) {}
    };

    class FrameGraph
    {
    public:
        std::unordered_map<vke_ds::id32_t, std::unique_ptr<RenderResource>> resources;
        std::unordered_map<vke_ds::id32_t, PermanentResourceState> permanentResourceStates;
        std::unordered_set<vke_ds::id32_t> targetResources;
        std::unordered_map<vke_ds::id32_t, std::unique_ptr<ResourceNode>> resourceNodes;
        std::unordered_map<vke_ds::id32_t, std::unique_ptr<TaskNode>> taskNodes;
        bool needRecompile;

        FrameGraph(const uint32_t framesInFlight)
            : framesInFlight(framesInFlight), resourceIDAllocator(1), taskNodeIDAllocator(1), resourceNodeIDAllocator(1), transientMemoryUpdateCnt(0), needRecompile(true)
        {
            init();
        }

        FrameGraph(const FrameGraph &) = delete;
        FrameGraph &operator=(const FrameGraph &) = delete;

        ~FrameGraph()
        {
            for (int i = 1; i < TASK_TYPE_CNT - 1; i++)
                if (RenderEnvironment::HasQueue(QueueType(i)))
                    for (int j = 0; j < framesInFlight; j++)
                        vkDestroyFence(globalLogicalDevice, fences[i - 1][j], nullptr);
        }

        void MarkNeedRecompile() { needRecompile = true; }

        vke_ds::id32_t AddPermanentImageResource(std::string &&name, const bool framesInFlight, VkImage *images, const VkImageAspectFlags aspectMask, const bool dependOnSwapchain,
                                                 const VkPipelineStageFlags2 stStage, const std::optional<VkImageLayout> stLayout, const std::optional<VkImageLayout> enLayout)
        {
            vke_ds::id32_t id = resourceIDAllocator.Alloc();
            resources.emplace(id, std::make_unique<ImageResource>(std::move(name), id, false, framesInFlight, images, aspectMask, dependOnSwapchain));
            permanentResourceStates.emplace(id, PermanentResourceState(stStage, stLayout, enLayout));
            return id;
        }

        vke_ds::id32_t AddPermanentImageResource(std::string &&name, const bool framesInFlight, VkImage *images,
                                                 const VkImageAspectFlags aspectMask, uint32_t mipLevelCnt, uint32_t layerCnt, const bool dependOnSwapchain,
                                                 const VkPipelineStageFlags2 stStage, const std::optional<VkImageLayout> stLayout, const std::optional<VkImageLayout> enLayout)
        {
            vke_ds::id32_t id = resourceIDAllocator.Alloc();
            resources.emplace(id, std::make_unique<ImageResource>(std::move(name), id, false, framesInFlight, images, aspectMask, mipLevelCnt, layerCnt, dependOnSwapchain));
            permanentResourceStates.emplace(id, PermanentResourceState(stStage, stLayout, enLayout));
            return id;
        }

        vke_ds::id32_t AddPermanentBufferResource(std::string &&name, const bool framesInFlight,
                                                  VkBuffer *buffers, const VkDeviceSize offset, const VkDeviceSize size,
                                                  const VkPipelineStageFlags2 stStage)
        {
            vke_ds::id32_t id = resourceIDAllocator.Alloc();
            resources.emplace(id, std::make_unique<BufferResource>(std::move(name), id, false, framesInFlight,
                                                                   buffers, offset, size));
            permanentResourceStates.emplace(id, PermanentResourceState(stStage, std::nullopt, std::nullopt));
            return id;
        }

        vke_ds::id32_t AddTransientImageResource(std::string &&name, VkImage *images, const VkImageAspectFlags aspectMask)
        {
            vke_ds::id32_t id = resourceIDAllocator.Alloc();
            resources.emplace(id, std::make_unique<ImageResource>(std::move(name), id, true, true, images, aspectMask, false));
            return id;
        }

        vke_ds::id32_t AddTransientImageResource(std::string &&name, VkImage *images,
                                                 const VkImageAspectFlags aspectMask,
                                                 uint32_t mipLevelCnt, uint32_t layerCnt)
        {
            vke_ds::id32_t id = resourceIDAllocator.Alloc();
            resources.emplace(id, std::make_unique<ImageResource>(std::move(name), id, true, true, images, aspectMask, mipLevelCnt, layerCnt, false));
            return id;
        }

        vke_ds::id32_t AddTransientBufferResource(std::string &&name, VkBuffer *buffers, const VkDeviceSize offset, const VkDeviceSize size)
        {
            vke_ds::id32_t id = resourceIDAllocator.Alloc();
            resources.emplace(id, std::make_unique<BufferResource>(std::move(name), id, true, true, buffers, offset, size));
            return id;
        }

        void RemoveTransientResource(vke_ds::id32_t resourceID)
        {
            auto it = resources.find(resourceID);
            if (it == resources.end())
                return;
            auto &resource = *(it->second);
            VKE_FATAL_IF(!resource.isTransient, "removed resource not transient")

            for (vke_ds::id32_t nodeID : resource.resourceNodeIDs)
            {
                auto &resourceNode = *resourceNodes[nodeID];
                if (resourceNode.srcTaskID)
                    taskNodes[resourceNode.srcTaskID]->resourceRefs.erase(resourceID);
                for (vke_ds::id32_t taskID : resourceNode.dstTaskIDs)
                    taskNodes[taskID]->resourceRefs.erase(resourceID);
                resourceNodes.erase(nodeID);
            }

            resources.erase(it);
            MarkNeedRecompile();
        }

        void AddTargetResource(const uint32_t resourceID)
        {
            auto it = resources.find(resourceID);
            if (it == resources.end())
                return;
            auto &resource = *(it->second);
            VKE_FATAL_IF(resource.isTransient, "target resource not permanent")
            targetResources.insert(resourceID);
            MarkNeedRecompile();
        }

        void RemoveTargetResource(const uint32_t resourceID)
        {
            targetResources.erase(resourceID);
            MarkNeedRecompile();
        }

        vke_ds::id32_t AllocResourceNode(std::string &&name, const vke_ds::id32_t resourceID)
        {
            vke_ds::id32_t id = resourceNodeIDAllocator.Alloc();
            resourceNodes.emplace(id, std::make_unique<ResourceNode>(std::move(name), id, resourceID));
            resources[resourceID]->resourceNodeIDs.insert(id);
            return id;
        }

        vke_ds::id32_t AllocTaskNode(std::string &&name, const TaskType taskType, const TaskNodeExecuteCallback callback)
        {
            vke_ds::id32_t id = taskNodeIDAllocator.Alloc();
            TaskType actualTaskType;
            switch (taskType)
            {
            case RENDER_TASK:
            case CPU_TASK:
                actualTaskType = taskType;
                break;
            case COMPUTE_TASK:
                actualTaskType = RenderEnvironment::HasQueue(COMPUTE_QUEUE) ? COMPUTE_TASK : RENDER_TASK;
                break;
            case TRANSFER_TASK:
                actualTaskType = RenderEnvironment::HasQueue(TRANSFER_QUEUE) ? TRANSFER_TASK : RENDER_TASK;
                break;
            default:
                actualTaskType = RENDER_TASK;
            }

            taskNodes.emplace(id, std::make_unique<TaskNode>(std::move(name), id, taskType, actualTaskType, callback));
            return id;
        }

        void AddTaskNodeResourceRef(const vke_ds::id32_t taskID,
                                    const vke_ds::id32_t inResourceNodeID, const vke_ds::id32_t outResourceNodeID,
                                    const VkAccessFlags2 accessMask, const VkPipelineStageFlags2 stageMask,
                                    const VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                    const VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                    const VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED)
        {
            TaskNode &taskNode = *taskNodes[taskID];
            taskNode.AddResourceRef(resourceNodes[inResourceNodeID ? inResourceNodeID : outResourceNodeID]->resourceID, inResourceNodeID, outResourceNodeID, accessMask, stageMask, layout, loadOp, storeOp);
            if (inResourceNodeID > 0)
                resourceNodes[inResourceNodeID]->dstTaskIDs.push_back(taskID);
            if (outResourceNodeID > 0)
                resourceNodes[outResourceNodeID]->srcTaskID = taskID;
            MarkNeedRecompile();
        }

        void SetTaskNodeTransientReadyCallback(const vke_ds::id32_t taskID, const TaskNodeTransientReadyCallback &callback)
        {
            taskNodes[taskID]->transientReadyCallback = callback;
        }

        void Compile();
        void Sync(const uint32_t currentFrame);
        void PrepareForExecute(const uint32_t currentFrame);
        void Execute(const uint32_t currentFrame, const uint32_t imageIndex);

    private:
        uint32_t framesInFlight;
        uint32_t queueFamilies[TASK_TYPE_CNT - 1];
        uint32_t submitCntEstimates[TASK_TYPE_CNT];
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> resourceIDAllocator;
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> taskNodeIDAllocator;
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> resourceNodeIDAllocator;
        std::vector<std::unique_ptr<CommandPool>> commandPools[TASK_TYPE_CNT];
        std::unique_ptr<SemaphorePool> semaphorePools[MAX_FRAMES_IN_FLIGHT];
        std::vector<vke_ds::id32_t> orderedTasks;
        VkFence fences[TASK_TYPE_CNT - 2][MAX_FRAMES_IN_FLIGHT];
        TransientMemorySimulator<2> transientMemorySimulator;
        TransientMemoryManager<2> transientMemoryManager;
        std::unordered_map<vke_ds::id32_t, TransientMemoryAllocation> transientMemoryAllocationMap;
        uint32_t transientMemoryUpdateCnt;
        std::vector<std::unique_ptr<std::atomic<bool>>> cpuSemaphores;

        void init();
        void ensureTaskSemaphore(const uint32_t currentFrame, TaskNode &taskNode);
        bool assignNextTaskSemaphore(const uint32_t currentFrame, TaskNode &taskNode, TaskNode &nextTaskNode);
        void syncResources(const uint32_t currentFrame, const uint32_t imageIndex,
                           ResourceRef &ref, TaskNode &taskNode,
                           bool &needQueueSubmit,
                           std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                           std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers,
                           std::map<VkSemaphore, uint64_t> &waitSemaphoreMap,
                           VkPipelineStageFlags2 &waitDstStageMask);
        void endResourcesUse(const uint32_t currentFrame, const uint32_t imageIndex,
                             ResourceRef &ref, TaskNode &taskNode,
                             bool &needQueueSubmit,
                             std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                             std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers);
        void checkCrossQueue(ResourceRef &ref, TaskNode &taskNode);
        void updateTransientMemory(const uint32_t currentFrame);
    };
}
#endif
