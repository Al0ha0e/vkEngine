#ifndef FRAME_GRAPH_H
#define FRAME_GRAPH_H

#include <render/environment.hpp>
#include <render/command_pool.hpp>
#include <render/semaphore_pool.hpp>
#include <ds/id_allocator.hpp>
#include <logger.hpp>

namespace vke_render
{
    class ResourceNode
    {
    public:
        std::string name;
        bool isTransient;
        vke_ds::id32_t resourceNodeID;
        vke_ds::id32_t resourceID;
        vke_ds::id32_t srcTaskID;
        std::vector<vke_ds::id32_t> dstTaskIDs;

        ResourceNode() : isTransient(false), resourceNodeID(0), resourceID(0), srcTaskID(0) {}
        ResourceNode(std::string &&name, bool isTransient, vke_ds::id32_t nodeID, vke_ds::id32_t resourceID)
            : name(std::move(name)), isTransient(isTransient), resourceNodeID(nodeID), resourceID(resourceID), srcTaskID(0) {}
        ~ResourceNode() {}
        ResourceNode &operator=(const ResourceNode &) = delete;
        ResourceNode(const ResourceNode &) = delete;

        ResourceNode &operator=(ResourceNode &&ano)
        {
            if (this != &ano)
            {
                name = std::move(ano.name);
                isTransient = ano.isTransient;
                resourceNodeID = ano.resourceNodeID;
                resourceID = ano.resourceID;
                srcTaskID = ano.srcTaskID;
                dstTaskIDs = std::move(ano.dstTaskIDs);
            }
            return *this;
        }

        ResourceNode(ResourceNode &&ano)
            : name(std::move(ano.name)), isTransient(ano.isTransient), resourceNodeID(ano.resourceNodeID), resourceID(ano.resourceID),
              srcTaskID(ano.srcTaskID), dstTaskIDs(std::move(ano.dstTaskIDs)) {}
    };

    class TaskNode;

    class ResourceRef
    {
    public:
        bool isTransient;
        vke_ds::id32_t inResourceNodeID;
        vke_ds::id32_t outResourceNodeID;
        VkAccessFlags2 accessMask;
        VkPipelineStageFlags2 stageMask;
        VkImageLayout imageLayout;
        VkAttachmentLoadOp loadOp;
        VkAttachmentStoreOp storeOp;
        ResourceRef *crossQueueRef; // ref used by the next task in another queue ? ref : nullptr (nothing to do with in/output)
        TaskNode *crossQueueTask;

        ResourceRef() : isTransient(false), inResourceNodeID(0), outResourceNodeID(0), accessMask(VK_ACCESS_NONE),
                        stageMask(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT), imageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
                        loadOp(VK_ATTACHMENT_LOAD_OP_DONT_CARE), storeOp(VK_ATTACHMENT_STORE_OP_DONT_CARE),
                        crossQueueRef(nullptr), crossQueueTask(nullptr)
        {
        }

        ResourceRef(bool isTransient, vke_ds::id32_t inResourceNodeID, vke_ds::id32_t outResourceNodeID,
                    VkAccessFlags2 accessMask, VkPipelineStageFlags2 stageMask,
                    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
                    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE)
            : isTransient(isTransient), inResourceNodeID(inResourceNodeID), outResourceNodeID(outResourceNodeID),
              accessMask(accessMask), stageMask(stageMask), imageLayout(layout),
              loadOp(loadOp), storeOp(storeOp), crossQueueRef(nullptr), crossQueueTask(nullptr) {}
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
        uint64_t semaphoreValueOffset;
        VkSemaphore lastSemaphore;
        VkSemaphore currentSemaphore;
        std::vector<ResourceRef> resourceRefs;
        TaskNodeExecuteCallback executeCallback;

        TaskNode() : taskID(0), taskType(RENDER_TASK), valid(false),
                     needQueueSubmit(false), isFinalTask(false), indeg(0),
                     lastSemaphoreValue(0), semaphoreValueOffset(0), lastSemaphore(nullptr), currentSemaphore(nullptr) {}

        TaskNode(std::string &&name, TaskType type, TaskType actualTaskType)
            : name(std::move(name)), taskID(0), taskType(type), actualTaskType(actualTaskType),
              valid(false), needQueueSubmit(false), isFinalTask(false),
              indeg(0), lastSemaphoreValue(0), semaphoreValueOffset(0), lastSemaphore(nullptr), currentSemaphore(nullptr) {}

        TaskNode(std::string &&name, vke_ds::id32_t id, TaskType type, TaskType actualTaskType)
            : name(std::move(name)), taskID(id), taskType(type), actualTaskType(actualTaskType),
              valid(false), needQueueSubmit(false), isFinalTask(false),
              indeg(0), lastSemaphoreValue(0), semaphoreValueOffset(0), lastSemaphore(nullptr), currentSemaphore(nullptr) {}

        TaskNode(std::string &&name, vke_ds::id32_t id, TaskType type, TaskType actualTaskType, TaskNodeExecuteCallback &callback)
            : name(std::move(name)), taskID(id), taskType(type), actualTaskType(actualTaskType),
              valid(false), needQueueSubmit(false), isFinalTask(false),
              indeg(0), lastSemaphoreValue(0), semaphoreValueOffset(0), lastSemaphore(nullptr), currentSemaphore(nullptr), executeCallback(callback) {}

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
                semaphoreValueOffset = ano.semaphoreValueOffset;
                lastSemaphore = ano.lastSemaphore;
                currentSemaphore = ano.currentSemaphore;
                resourceRefs = std::move(ano.resourceRefs);
                executeCallback = std::move(ano.executeCallback);
            }
            return *this;
        }

        TaskNode(TaskNode &&ano)
            : name(std::move(name)), taskID(ano.taskID), taskType(ano.taskType), actualTaskType(ano.actualTaskType),
              valid(ano.valid), needQueueSubmit(needQueueSubmit), isFinalTask(isFinalTask), indeg(ano.indeg),
              lastSemaphoreValue(ano.lastSemaphoreValue), semaphoreValueOffset(ano.semaphoreValueOffset),
              lastSemaphore(ano.lastSemaphore), currentSemaphore(ano.currentSemaphore),
              resourceRefs(std::move(ano.resourceRefs)), executeCallback(std::move(ano.executeCallback)) {}

        void AddResourceRef(bool isTransient, vke_ds::id32_t inResourceNodeID, vke_ds::id32_t outResourceNodeID,
                            VkAccessFlags2 accessMask, VkPipelineStageFlags2 stageMask,
                            VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
                            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE)
        {
            resourceRefs.emplace_back(isTransient, inResourceNodeID, outResourceNodeID,
                                      accessMask, stageMask, layout, loadOp, storeOp);
        }

        void Reset()
        {
            valid = false;
            needQueueSubmit = false;
            isFinalTask = false;
            indeg = 0;
            currentSemaphore = nullptr;
            semaphoreValueOffset = 0;
        }
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
        vke_ds::id32_t firstAccessTaskID;
        vke_ds::id32_t lastAccessTaskID;
        ResourceRef *prevUsedRef;
        TaskNode *prevUsedTask;
        ResourceRef *tmpPrevUsedRef;
        TaskNode *tmpPrevUsedTask;

        RenderResource() : resourceID(0), resourceType(IMAGE_RESOURCE),
                           firstAccessTaskID(0), lastAccessTaskID(0), prevUsedRef(nullptr), prevUsedTask(nullptr) {}
        RenderResource(ResourceType type) : resourceID(0), resourceType(type),
                                            firstAccessTaskID(0), lastAccessTaskID(0), prevUsedRef(nullptr), prevUsedTask(nullptr) {}
        RenderResource(std::string &&name, vke_ds::id32_t id, ResourceType type)
            : name(name), resourceID(id), resourceType(type),
              firstAccessTaskID(0), lastAccessTaskID(0), prevUsedRef(nullptr), prevUsedTask(nullptr) {}

        void ResetTmpValues()
        {
            firstAccessTaskID = 0;
            lastAccessTaskID = 0;
            tmpPrevUsedRef = 0;
            tmpPrevUsedTask = 0;
        }
    };

    class ImageResource : public RenderResource
    {
    public:
        VkImage image;
        VkImageAspectFlags aspectMask;
        VkDescriptorImageInfo info; // VkSampler sampler; VkImageView imageView; VkImageLayout imageLayout;
        ImageResource()
            : RenderResource(IMAGE_RESOURCE), image(nullptr), aspectMask(VK_IMAGE_ASPECT_NONE) {}
        ImageResource(std::string &&name, vke_ds::id32_t id, VkImage image, VkImageAspectFlags aspect, VkDescriptorImageInfo info)
            : RenderResource(std::move(name), id, IMAGE_RESOURCE), image(image), aspectMask(aspect), info(info) { VKE_LOG_DEBUG("IMAGE RESOURCE {} {}", name, (void *)image) }
    };

    class BufferResource : public RenderResource
    {
    public:
        VkDescriptorBufferInfo info; // VkBuffer buffer; VkDeviceSize offset; VkDeviceSize range;
        BufferResource()
            : RenderResource(BUFFER_RESOURCE) {}
        BufferResource(std::string &&name, vke_ds::id32_t id, VkDescriptorBufferInfo info)
            : RenderResource(std::move(name), id, BUFFER_RESOURCE), info(info) { VKE_LOG_DEBUG("BUFFER RESOURCE {} {}", name, (void *)(info.buffer)) }
    };

    struct PermanentResourceState
    {
        VkPipelineStageFlags2 stStage;
        std::optional<VkImageLayout> stImageLayout;
        std::optional<VkImageLayout> enImageLayout;

        PermanentResourceState() {}
        PermanentResourceState(VkPipelineStageFlags2 stStage, std::optional<VkImageLayout> stLayout, std::optional<VkImageLayout> enLayout)
            : stStage(stStage), stImageLayout(stLayout), enImageLayout(enLayout) {}
    };

    class FrameGraph
    {
    public:
        uint64_t semaphoreValueBase;
        std::vector<std::unique_ptr<RenderResource>> permanentResources;
        std::map<vke_ds::id32_t, std::unique_ptr<RenderResource>> transientResources;
        std::vector<PermanentResourceState> permanentResourceStates;
        std::set<vke_ds::id32_t> targetResources;
        std::map<vke_ds::id32_t, std::unique_ptr<ResourceNode>> resourceNodes;
        std::map<vke_ds::id32_t, std::unique_ptr<TaskNode>> taskNodes;

        FrameGraph(uint32_t framesInFlight)
            : framesInFlight(framesInFlight), semaphoreValueBase(0), taskIDAllocator(1), resourceIDAllocator(1)
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

        vke_ds::id32_t AddPermanentResource(std::unique_ptr<RenderResource> &&resource, PermanentResourceState &resourceState)
        {
            vke_ds::id32_t id = permanentResources.size();
            resource->resourceID = id;
            permanentResources.push_back(std::move(resource));
            permanentResourceStates.push_back(resourceState);
            return id;
        }

        vke_ds::id32_t AddPermanentImageResource(std::string &&name, VkImage image, VkImageAspectFlags aspectMask, VkDescriptorImageInfo info,
                                                 VkPipelineStageFlags2 stStage, std::optional<VkImageLayout> stLayout, std::optional<VkImageLayout> enLayout)
        {
            vke_ds::id32_t id = permanentResources.size();
            permanentResources.push_back(std::make_unique<ImageResource>(std::move(name), id, image, aspectMask, info));
            permanentResourceStates.emplace_back(stStage, stLayout, enLayout);
            return id;
        }

        vke_ds::id32_t AddPermanentBufferResource(std::string &&name, VkDescriptorBufferInfo info, VkPipelineStageFlags2 stStage)
        {
            vke_ds::id32_t id = permanentResources.size();
            permanentResources.push_back(std::make_unique<BufferResource>(std::move(name), id, info));
            permanentResourceStates.emplace_back(stStage, std::nullopt, std::nullopt);
            return id;
        }

        void AddTargetResource(uint32_t resourceID) { targetResources.insert(resourceID); }

        void RemoveTargetResource(uint32_t resourceID) { targetResources.erase(resourceID); }

        vke_ds::id32_t AllocResourceNode(std::string &&name, bool isTransient, vke_ds::id32_t resourceID)
        {
            vke_ds::id32_t id = resourceIDAllocator.Alloc();
            resourceNodes.emplace(id, std::make_unique<ResourceNode>(std::move(name), isTransient, id, resourceID));
            return id;
        }

        vke_ds::id32_t AllocTaskNode(std::string &&name, TaskType taskType, TaskNodeExecuteCallback callback)
        {
            vke_ds::id32_t id = taskIDAllocator.Alloc();
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

        void AddTaskNodeResourceRef(vke_ds::id32_t taskID,
                                    bool isTransient, vke_ds::id32_t inResourceNodeID, vke_ds::id32_t outResourceNodeID,
                                    VkAccessFlags2 accessMask, VkPipelineStageFlags2 stageMask,
                                    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
                                    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE)
        {
            TaskNode &taskNode = *taskNodes[taskID];
            taskNode.AddResourceRef(isTransient, inResourceNodeID, outResourceNodeID, accessMask, stageMask, layout, loadOp, storeOp);
            if (inResourceNodeID > 0)
                resourceNodes[inResourceNodeID]->dstTaskIDs.push_back(taskID);
            if (outResourceNodeID > 0)
                resourceNodes[outResourceNodeID]->srcTaskID = taskID;
        }

        void ClearTaskNodes() { taskNodes.clear(); }

        void ClearResourceNodes() { resourceNodes.clear(); }

        void ClearTransientResources() { transientResources.clear(); }

        void ClearPermanentResources() { permanentResources.clear(); }

        void Compile();
        void Execute(uint32_t currentFrame, uint32_t imageIndex);

    private:
        uint32_t framesInFlight;
        uint32_t queueFamilies[TASK_TYPE_CNT - 1];
        uint32_t submitCntEstimates[TASK_TYPE_CNT];
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> taskIDAllocator;
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> resourceIDAllocator;
        std::vector<std::unique_ptr<CommandPool>> commandPools[TASK_TYPE_CNT];
        std::unique_ptr<SemaphorePool> semaphorePool;
        std::vector<vke_ds::id32_t> orderedTasks;
        VkFence fences[TASK_TYPE_CNT - 2][MAX_FRAMES_IN_FLIGHT];
        std::vector<std::unique_ptr<std::atomic<bool>>> cpuSemaphores;

        void init();
        void syncResources(ResourceRef &ref, TaskNode &taskNode,
                           bool &needQueueSubmit,
                           std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                           std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers,
                           std::map<VkSemaphore, uint64_t> &waitSemaphoreMap,
                           VkPipelineStageFlags2 &waitDstStageMask);
        void endResourcesUse(ResourceRef &ref, TaskNode &taskNode,
                             bool &needQueueSubmit,
                             std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                             std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers);
        void checkCrossQueue(ResourceRef &ref, TaskNode &taskNode);
    };
}
#endif