#include <render/compute.hpp>
#include <render/resource.hpp>
#include <render/descriptor.hpp>

int main()
{
    vke_render::RenderResourceManager::Init();

    vke_render::ComputeShader *shader = vke_render::RenderResourceManager::LoadComputeShader("./tests/shader/test_compute.spv");
    std::vector<vke_render::DescriptorInfo> descriptorInfos;
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    descriptorInfos.push_back(vke_render::DescriptorInfo(binding, 1024 * sizeof(int)));

        vke_render::ComputeTask task(shader, std::move(descriptorInfos));

    uint64_t id = task.AddInstance();
    return 0;
}