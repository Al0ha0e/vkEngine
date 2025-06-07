#include <render/compute.hpp>
#include <asset.hpp>
#include <render/descriptor.hpp>
#include <render/buffer.hpp>
#include <engine.hpp>
#include <vector>

GLFWwindow *initWindow(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
}

int main()
{
    std::vector<vke_render::PassType> passes = {
        vke_render::BASE_RENDERER,
        vke_render::OPAQUE_RENDERER};
    std::vector<std::unique_ptr<vke_render::RenderPassBase>> customPasses;
    std::vector<vke_render::RenderPassInfo> customPassInfo;
    GLFWwindow *window = initWindow(800, 600);
    vke_common::EventSystem::Init();
    vke_render::RenderEnvironment *environment = vke_render::RenderEnvironment::Init(window);
    vke_common::Engine *engine = vke_common::Engine::Init(&(environment->rootRenderContext), passes, customPasses, customPassInfo);
    vke_common::AssetManager::LoadAssetLUT("./tests/scene/test_compute_desc.json");
    {
        std::shared_ptr<vke_render::ComputeShader> shader = vke_common::AssetManager::LoadComputeShader(1024);
        VkDescriptorSetLayoutBinding binding{};
        vke_render::InitDescriptorSetLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);

        VkDeviceSize bufferSize = 1024 * sizeof(int);

        float *oridata = new float[1024];
        memset(oridata, 0, bufferSize);

        VkDevice logicalDevice = environment->logicalDevice;

        vke_render::StagedBuffer buffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        buffer.ToBuffer(0, oridata, bufferSize);

        vke_render::ComputeTask task(shader);

        vke_ds::id64_t id = task.AddInstance(
            std::move(std::vector<VkSemaphore>{}),
            std::move(std::vector<VkPipelineStageFlags>{}),
            std::move(std::vector<VkSemaphore>{}));

        std::vector<VkWriteDescriptorSet> descriptorWrites(1);

        VkDescriptorBufferInfo bufferInfo{};
        vke_render::InitDescriptorBufferInfo(bufferInfo, buffer.buffer, 0, buffer.bufferSize);
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[0], 0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo);

        task.UpdateBindings(id, descriptorWrites);

        vke_render::CommandPool commandPool(environment->queueFamilyIndices.computeOnlyFamily.size() > 0 ? environment->queueFamilyIndices.computeOnlyFamily[0]
                                                                                                         : environment->queueFamilyIndices.graphicsAndComputeFamily.value(),
                                            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        VkCommandBuffer commandBuffer = commandPool.AllocateCommandBuffer();

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = 0;
        VkFence fence;
        vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence);

        task.Dispatch(id, commandBuffer, glm::ivec3(10, 1, 1), fence);

        buffer.FromBuffer(0, oridata, bufferSize);

        for (int i = 0; i < 10; i++)
            std::cout << oridata[i] << " ";

        vkDestroyFence(logicalDevice, fence, nullptr);
    }

    vke_common::Engine::Dispose();
    vke_render::RenderEnvironment::Dispose();
    vke_common::EventSystem::Dispose();
    return 0;
}