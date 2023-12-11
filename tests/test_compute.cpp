#include <render/compute.hpp>
#include <render/resource.hpp>
#include <render/descriptor.hpp>
#include <render/buffer.hpp>
#include <engine.hpp>
#include <vector>

int main()
{
    std::vector<vke_render::PassType> passes = {
        vke_render::BASE_RENDERER,
        vke_render::OPAQUE_RENDERER};
    std::vector<vke_render::SubpassBase *> customPasses;
    std::vector<vke_render::RenderPassInfo> customPassInfo;
    vke_common::Engine *engine = vke_common::Engine::Init(800, 600, passes, customPasses, customPassInfo);

    vke_render::ComputeShader *shader = vke_render::RenderResourceManager::LoadComputeShader("./tests/shader/test_compute.spv");
    std::vector<vke_render::DescriptorInfo> descriptorInfos;
    VkDescriptorSetLayoutBinding binding{};
    vke_render::InitDescriptorSetLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);

    VkDeviceSize bufferSize = 1024 * sizeof(int);
    descriptorInfos.push_back(vke_render::DescriptorInfo(binding, bufferSize));

    int *oridata = new int[1024];
    memset(oridata, 0, bufferSize);

    VkDevice logicalDevice = vke_render::RenderEnvironment::GetInstance()->logicalDevice;

    vke_render::StagedBuffer buffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    buffer.ToBuffer(0, oridata, bufferSize);

    vke_render::ComputeTask task(shader, std::move(descriptorInfos));

    uint64_t id = task.AddInstance(
        std::move(std::vector<VkBuffer>{buffer.buffer}),
        std::move(std::vector<VkSemaphore>{}),
        std::move(std::vector<VkPipelineStageFlags>{}),
        std::move(std::vector<VkSemaphore>{}));

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vke_render::RenderEnvironment::GetInstance()->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;

    if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;
    VkFence fence;
    vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence);

    task.Dispatch(id, commandBuffer, glm::ivec3(1, 1, 1), fence);

    vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX);

    buffer.FromBuffer(0, oridata, bufferSize);

    for (int i = 0; i < 10; i++)
        std::cout << oridata[i] << " ";

    return 0;
}