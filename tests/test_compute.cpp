#include <render/compute.hpp>
#include <render/resource.hpp>
#include <render/descriptor.hpp>
#include <engine.hpp>
#include <vector>

int main()
{
    vke_common::Engine *engine = vke_common::Engine::Init(800, 600);

    vke_render::ComputeShader *shader = vke_render::RenderResourceManager::LoadComputeShader("./tests/shader/test_compute.spv");
    std::vector<vke_render::DescriptorInfo> descriptorInfos;
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    VkDeviceSize bufferSize = 1024 * sizeof(int);
    descriptorInfos.push_back(vke_render::DescriptorInfo(binding, bufferSize));

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vke_render::RenderEnvironment::CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    int *oridata = new int[1024];
    memset(oridata, 0, bufferSize);

    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    void *data;
    VkDevice logicalDevice = vke_render::RenderEnvironment::GetInstance()->logicalDevice;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, oridata, (size_t)bufferSize);
    // vkUnmapMemory(logicalDevice, stagingBufferMemory);
    vke_render::RenderEnvironment::CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer,
        bufferMemory);
    vke_render::RenderEnvironment::CopyBuffer(
        stagingBuffer,
        buffer,
        bufferSize);

    vke_render::ComputeTask task(shader, std::move(descriptorInfos));

    uint64_t id = task.AddInstance(
        std::move(std::vector<VkBuffer>{buffer}),
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

    vke_render::RenderEnvironment::CopyBuffer(
        buffer,
        stagingBuffer,
        bufferSize);
    memcpy(oridata, data, (size_t)bufferSize);

    for (int i = 0; i < 10; i++)
        std::cout << oridata[i] << " ";

    return 0;
}