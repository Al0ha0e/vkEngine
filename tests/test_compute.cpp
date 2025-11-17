#include <engine.hpp>

GLFWwindow *initWindow(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
}

int main()
{
    std::vector<vke_render::PassType> passes = {
        vke_render::SKYBOX_RENDERER};
    std::vector<std::unique_ptr<vke_render::RenderPassBase>> customPasses;
    GLFWwindow *window = initWindow(800, 600);
    vke_common::Engine *engine = vke_common::Engine::Init(window, nullptr, passes, customPasses);
    vke_render::RenderEnvironment *environment = vke_render::RenderEnvironment::GetInstance();
    vke_common::AssetManager::LoadAssetLUT("./tests/scene/test_compute_desc.json");
    {
        std::shared_ptr<vke_render::ShaderModuleSet> shader = vke_common::AssetManager::LoadComputeShader(2048);

        VkDeviceSize bufferSize = 1024 * sizeof(int);

        vke_render::StagedBuffer buffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        memset(buffer.data, 0, bufferSize);
        buffer.ToBuffer(0, bufferSize);

        vke_render::ComputePipeline task(shader);
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkWriteDescriptorSet> descriptorWrites(1);

        task.CreateDescriptorSets(descriptorSets);

        VkDescriptorBufferInfo bufferInfo = buffer.GetDescriptorBufferInfo();
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[0], descriptorSets[0], 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo);
        vkUpdateDescriptorSets(vke_render::globalLogicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        vke_render::GPUCommandPool commandPool(environment->queueFamilyIndices.computeOnlyFamily.has_value() ? environment->queueFamilyIndices.computeOnlyFamily.value()
                                                                                                             : environment->queueFamilyIndices.graphicsAndComputeFamily.value(),
                                               VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        VkCommandBuffer commandBuffer = commandPool.AllocateCommandBuffer();

        /////////////////////////////////////////////////////////////////

        vke_render::FrameGraph frameGraph(1);

        vke_ds::id32_t resourceID = frameGraph.AddPermanentBufferResource("testbuffer", bufferInfo, VK_PIPELINE_STAGE_NONE);

        bufferInfo.buffer = buffer.stagingBuffer.buffer;
        vke_ds::id32_t stagingResourceID = frameGraph.AddPermanentBufferResource("stagingBuffer", bufferInfo, VK_PIPELINE_STAGE_NONE);

        frameGraph.AddTargetResource(stagingResourceID);
        vke_ds::id32_t computeOutResourceNodeID = frameGraph.AllocResourceNode("computeOut", false, resourceID);
        vke_ds::id32_t transferOutResourceNodeID = frameGraph.AllocResourceNode("transferOut", false, stagingResourceID);
        // vke_ds::id32_t cpuOutResourceNodeID = frameGraph.AllocResourceNode("cpuOut", false, stagingResourceID);

        auto computeCallback = [&task, &descriptorSets](vke_render::TaskNode &node, vke_render::FrameGraph &frameGraph,
                                                        VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
        {
            task.Dispatch(commandBuffer, descriptorSets, glm::ivec3(10, 1, 1));
        };

        vke_ds::id32_t computeTaskNodeID = frameGraph.AllocTaskNode("compute task", vke_render::COMPUTE_TASK, computeCallback);

        auto transferCallback = [&buffer](vke_render::TaskNode &node, vke_render::FrameGraph &frameGraph,
                                          VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
        {
            buffer.FromBufferAsync(commandBuffer, 0, buffer.bufferSize);
        };

        vke_ds::id32_t transferTaskNodeID = frameGraph.AllocTaskNode("transfer task", vke_render::TRANSFER_TASK, transferCallback);

        auto cpuOP = [&buffer]()
        {
            for (int i = 0; i < 10; i++)
                std::cout << ((float *)buffer.data)[i] << " ";
            std::cout << "\n";
        };

        auto cpuCallback = [&cpuOP](vke_render::TaskNode &node, vke_render::FrameGraph &frameGraph,
                                    VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
        {
            *(std::function<void()> *)commandBuffer = cpuOP;
        };

        vke_ds::id32_t cpuTaskNodeID = frameGraph.AllocTaskNode("cpu task", vke_render::CPU_TASK, cpuCallback);

        frameGraph.AddTaskNodeResourceRef(computeTaskNodeID, false, 0, computeOutResourceNodeID,
                                          VK_ACCESS_2_SHADER_WRITE_BIT,
                                          VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

        frameGraph.AddTaskNodeResourceRef(transferTaskNodeID, false, computeOutResourceNodeID, 0,
                                          VK_ACCESS_2_TRANSFER_READ_BIT,
                                          VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);

        frameGraph.AddTaskNodeResourceRef(transferTaskNodeID, false, 0, transferOutResourceNodeID,
                                          VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                          VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

        frameGraph.AddTaskNodeResourceRef(cpuTaskNodeID, false, transferOutResourceNodeID, 0,
                                          VK_ACCESS_2_HOST_READ_BIT,
                                          VK_PIPELINE_STAGE_2_HOST_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        frameGraph.Compile();
        frameGraph.Execute(0, 0);
        /////////////////////////////////////////////////////////////////
        vke_common::Engine::WaitIdle();
    }
    vke_common::Engine::Dispose();
    return 0;
}