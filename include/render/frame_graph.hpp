#ifndef FRAME_GRAPH_H
#define FRAME_GRAPH_H

#include <render/environment.hpp>
#include <render/command_pool.hpp>
#include <ds/id_allocator.hpp>
#include <set>

namespace vke_render
{
    class ResourceNode
    {
    public:
        bool isTransient;
        vke_ds::id32_t resourceNodeID;
        vke_ds::id32_t resourceID;
        vke_ds::id32_t srcTaskID;
        std::vector<vke_ds::id32_t> dstTaskIDs;

        ResourceNode() : isTransient(false), resourceNodeID(0), resourceID(0), srcTaskID(0) {}
        ResourceNode(bool isTransient, vke_ds::id32_t nodeID, vke_ds::id32_t resourceID)
            : isTransient(isTransient), resourceNodeID(nodeID), resourceID(resourceID), srcTaskID(0) {}
        ~ResourceNode() {}
        ResourceNode &operator=(const ResourceNode &) = delete;
        ResourceNode(const ResourceNode &) = delete;

        ResourceNode &operator=(ResourceNode &&ano)
        {
            if (this != &ano)
            {
                isTransient = ano.isTransient;
                resourceNodeID = ano.resourceNodeID;
                resourceID = ano.resourceID;
                srcTaskID = ano.srcTaskID;
                dstTaskIDs = std::move(ano.dstTaskIDs);
            }
            return *this;
        }

        ResourceNode(ResourceNode &&ano)
            : isTransient(ano.isTransient), resourceNodeID(ano.resourceNodeID), resourceID(ano.resourceID),
              srcTaskID(ano.srcTaskID), dstTaskIDs(std::move(ano.dstTaskIDs)) {}
    };

    class TaskNode;

    class ResourceRef
    {
    public:
        bool isTransient;
        vke_ds::id32_t inResourceNodeID;
        vke_ds::id32_t outResourceNodeID;
        VkAccessFlags accessMask;
        VkPipelineStageFlags stageMask;
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
                    VkAccessFlags accessMask, VkPipelineStageFlags stageMask,
                    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
                    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE)
            : isTransient(isTransient), inResourceNodeID(inResourceNodeID), outResourceNodeID(outResourceNodeID),
              accessMask(accessMask), stageMask(stageMask), imageLayout(layout),
              loadOp(loadOp), storeOp(storeOp), crossQueueRef(nullptr), crossQueueTask(nullptr) {}
    };

    enum TaskType
    {
        RENDER_TASK,
        COMPUTE_TASK,
        TRANSFER_TASK,
    };

    class FrameGraph;

    using TaskNodeExecuteCallback = std::function<void(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)>;

    class TaskNode
    {
    public:
        vke_ds::id32_t taskID;
        TaskType taskType;
        TaskType actualTaskType;
        bool valid;
        bool needQueueSubmit;
        uint32_t indeg;
        uint64_t order;
        std::vector<ResourceRef> resourceRefs;
        TaskNodeExecuteCallback executeCallback;

        TaskNode() : taskID(0), taskType(RENDER_TASK), valid(false), needQueueSubmit(false), indeg(0), order(0) {}

        TaskNode(TaskType type, TaskType actualTaskType)
            : taskID(0), taskType(type), actualTaskType(actualTaskType), valid(false), needQueueSubmit(false), indeg(0), order(0) {}

        TaskNode(vke_ds::id32_t id, TaskType type, TaskType actualTaskType)
            : taskID(id), taskType(type), actualTaskType(actualTaskType), valid(false), needQueueSubmit(false), indeg(0), order(0) {}

        TaskNode(vke_ds::id32_t id, TaskType type, TaskType actualTaskType, TaskNodeExecuteCallback &callback)
            : taskID(id), taskType(type), actualTaskType(actualTaskType),
              valid(false), needQueueSubmit(false), indeg(0), order(0), executeCallback(callback) {}

        ~TaskNode() {}
        TaskNode &operator=(const TaskNode &) = delete;
        TaskNode(const TaskNode &) = delete;

        TaskNode &operator=(TaskNode &&ano)
        {
            if (this != &ano)
            {
                taskID = ano.taskID;
                taskType = ano.taskType;
                valid = ano.valid;
                indeg = ano.indeg;
                order = ano.order;
                resourceRefs = std::move(ano.resourceRefs);
                executeCallback = std::move(ano.executeCallback);
            }
            return *this;
        }

        TaskNode(TaskNode &&ano)
            : taskID(ano.taskID), taskType(ano.taskType), valid(ano.valid), indeg(ano.indeg), order(ano.order),
              resourceRefs(std::move(ano.resourceRefs)), executeCallback(std::move(ano.executeCallback)) {}

        void AddResourceRef(bool isTransient, vke_ds::id32_t inResourceNodeID, vke_ds::id32_t outResourceNodeID,
                            VkAccessFlags accessMask, VkPipelineStageFlags stageMask,
                            VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
                            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE)
        {
            resourceRefs.emplace_back(isTransient, inResourceNodeID, outResourceNodeID,
                                      accessMask, stageMask, layout, loadOp, storeOp);
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
        vke_ds::id32_t resourceID;
        ResourceType resourceType;
        ResourceRef *lastUsedRef;
        TaskNode *lastUsedTask;

        RenderResource() : resourceID(0), resourceType(IMAGE_RESOURCE), lastUsedRef(nullptr), lastUsedTask(nullptr) {}
        RenderResource(ResourceType type) : resourceID(0), resourceType(type), lastUsedRef(nullptr), lastUsedTask(nullptr) {}
        RenderResource(vke_ds::id32_t id, ResourceType type)
            : resourceID(id), resourceType(type), lastUsedRef(nullptr), lastUsedTask(nullptr) {}
    };

    class ImageResource : public RenderResource
    {
    public:
        VkImage image;
        VkImageAspectFlags aspectMask;
        VkDescriptorImageInfo info; // VkSampler sampler; VkImageView imageView; VkImageLayout imageLayout;
        ImageResource()
            : RenderResource(IMAGE_RESOURCE), image(nullptr), aspectMask(VK_IMAGE_ASPECT_NONE) {}
        ImageResource(vke_ds::id32_t id, VkImage image, VkImageAspectFlags aspect, VkDescriptorImageInfo info)
            : RenderResource(id, IMAGE_RESOURCE), image(image), aspectMask(aspect), info(info) {}
    };

    class BufferResource : public RenderResource
    {
    public:
        VkDescriptorBufferInfo info; // VkBuffer buffer; VkDeviceSize offset; VkDeviceSize range;
        BufferResource()
            : RenderResource(BUFFER_RESOURCE) {}
        BufferResource(vke_ds::id32_t id, VkDescriptorBufferInfo info)
            : RenderResource(id, BUFFER_RESOURCE), info(info) {}
    };

    struct PermanentResourceState
    {
        VkPipelineStageFlags stStage;
        std::optional<VkImageLayout> stImageLayout;
        std::optional<VkImageLayout> enImageLayout;

        PermanentResourceState() {}
        PermanentResourceState(VkPipelineStageFlags stStage, std::optional<VkImageLayout> stLayout, std::optional<VkImageLayout> enLayout)
            : stStage(stStage), stImageLayout(stLayout), enImageLayout(enLayout) {}
    };

    class FrameGraph
    {
    public:
        uint64_t timelineSemaphoreBase;
        bool haveQueue[3];
        std::vector<std::unique_ptr<RenderResource>> permanentResources;
        std::map<vke_ds::id32_t, std::unique_ptr<RenderResource>> transientResources;
        std::vector<PermanentResourceState> permanentResourceStates;
        std::set<vke_ds::id32_t> targetResources;
        std::map<vke_ds::id32_t, std::unique_ptr<ResourceNode>> resourceNodes;
        std::map<vke_ds::id32_t, std::unique_ptr<TaskNode>> taskNodes;

        FrameGraph(uint32_t framesInFlight)
            : framesInFlight(framesInFlight), timelineSemaphoreBase(0), taskIDAllocator(1), resourceIDAllocator(1), lastTimelineValue(0)
        {
            init();
        }

        FrameGraph(const FrameGraph &) = delete;
        FrameGraph &operator=(const FrameGraph &) = delete;

        ~FrameGraph()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkDestroySemaphore(logicalDevice, timelineSemaphore, nullptr);
            for (int i = 1; i < 3; i++)
                if (haveQueue[i])
                    for (int j = 0; j < framesInFlight; j++)
                        vkDestroyFence(logicalDevice, fences[i - 1][j], nullptr);
        }

        vke_ds::id32_t AddPermanentResource(std::unique_ptr<RenderResource> &&resource, PermanentResourceState &resourceState)
        {
            vke_ds::id32_t id = permanentResources.size();
            resource->resourceID = id;
            permanentResources.push_back(std::move(resource));
            permanentResourceStates.push_back(resourceState);
            return id;
        }

        vke_ds::id32_t AddPermanentImageResource(VkImage image, VkImageAspectFlags aspectMask, VkDescriptorImageInfo info,
                                                 VkPipelineStageFlags stStage, std::optional<VkImageLayout> stLayout, std::optional<VkImageLayout> enLayout)
        {
            vke_ds::id32_t id = permanentResources.size();
            permanentResources.push_back(std::make_unique<ImageResource>(id, image, aspectMask, info));
            permanentResourceStates.emplace_back(stStage, stLayout, enLayout);
            return id;
        }

        vke_ds::id32_t AddPermanentBufferResource(VkDescriptorBufferInfo info, VkPipelineStageFlags stStage)
        {
            vke_ds::id32_t id = permanentResources.size();
            permanentResources.push_back(std::make_unique<BufferResource>(id, info));
            permanentResourceStates.emplace_back(stStage, std::nullopt, std::nullopt);
            return id;
        }

        void AddTargetResource(uint32_t resourceID) { targetResources.insert(resourceID); }

        void RemoveTargetResource(uint32_t resourceID) { targetResources.erase(resourceID); }

        vke_ds::id32_t AllocResourceNode(bool isTransient, vke_ds::id32_t resourceID)
        {
            vke_ds::id32_t id = resourceIDAllocator.Alloc();
            resourceNodes.emplace(id, std::make_unique<ResourceNode>(isTransient, id, resourceID));
            return id;
        }

        vke_ds::id32_t AllocTaskNode(TaskType taskType, TaskNodeExecuteCallback callback)
        {
            vke_ds::id32_t id = taskIDAllocator.Alloc();
            TaskType actualTaskType = (taskType == COMPUTE_TASK && haveQueue[COMPUTE_TASK])
                                          ? COMPUTE_TASK
                                          : ((taskType == TRANSFER_TASK && haveQueue[TRANSFER_TASK]) ? TRANSFER_TASK : RENDER_TASK);
            taskNodes.emplace(id, std::make_unique<TaskNode>(id, taskType, actualTaskType, callback));
            return id;
        }

        void AddTaskNodeResourceRef(vke_ds::id32_t taskID,
                                    bool isTransient, vke_ds::id32_t inResourceNodeID, vke_ds::id32_t outResourceNodeID,
                                    VkAccessFlags accessMask, VkPipelineStageFlags stageMask,
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
        uint32_t queueFamilies[3];
        uint32_t submitCnts[3];
        uint64_t lastTimelineValue;
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> taskIDAllocator;
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> resourceIDAllocator;
        std::vector<std::unique_ptr<CommandPool>> commandPools[3];
        std::vector<vke_ds::id32_t> orderedTasks;
        VkSemaphore timelineSemaphore;
        VkQueue gpuQueues[3];
        VkFence fences[2][MAX_FRAMES_IN_FLIGHT];

        void init();
        void syncResources(VkCommandBuffer commandBuffer, ResourceRef &ref, TaskNode &taskNode,
                           std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                           std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers,
                           uint64_t &waitSemaphoreValue, VkPipelineStageFlags &waitDstStageMask);
        void endResourcesUse(VkCommandBuffer commandBuffer, ResourceRef &ref, TaskNode &taskNode,
                             std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
                             std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers);
        void clearLastUsedInfo();
        void checkCrossQueue(ResourceRef &ref, TaskNode &taskNode);
    };
}
#endif