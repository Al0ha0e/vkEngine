#ifndef SKELETON_ANIMATOR_H
#define SKELETON_ANIMATOR_H

#include <time.hpp>
#include <component/transform.hpp>
#include <animation.hpp>
#include <render/render.hpp>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/containers/vector.h>

namespace vke_component
{

    const uint32_t MAX_BONE_PER_SKELETON = 256;

    class SkeletonAnimator
    {
    public:
        std::shared_ptr<vke_render::Material> material;
        std::shared_ptr<vke_common::Skeleton> skeleton;
        std::shared_ptr<vke_common::Animation> animation;
        std::unique_ptr<vke_render::RenderUnit> renderUnit;

        SkeletonAnimator(
            const vke_common::Transform &transform,
            std::shared_ptr<vke_render::Material> &mat,
            std::shared_ptr<const vke_render::Mesh> &mesh,
            std::shared_ptr<vke_common::Skeleton> &skeleton,
            std::shared_ptr<vke_common::Animation> &animation)
            : material(mat), skeleton(skeleton), animation(animation)
        {
            init(transform, mesh);
        }

        SkeletonAnimator(const vke_common::Transform &transform, const nlohmann::json &json)
        {
            material = vke_common::AssetManager::LoadMaterial(json["material"]);
            std::shared_ptr<const vke_render::Mesh> mesh = vke_common::AssetManager::LoadMesh(json["mesh"]);
            skeleton = vke_common::AssetManager::LoadSkeleton(json["skeleton"]);
            animation = vke_common::AssetManager::LoadAnimation(json["animation"]);
            init(transform, mesh);
        }

        ~SkeletonAnimator() {}

        void LoadToEngine()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderID = renderer->GetGBufferPass()->AddUnit(material, renderUnit.get(), true);
            renderer->AddRenderUpdateCallback(renderID, std::bind(&SkeletonAnimator::update, this, std::placeholders::_1));
        }

        void UnloadFromEngine()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderer->GetGBufferPass()->RemoveUnit(material.get(), renderID);
            renderer->RemoveRenderUpdateCallback(renderID);
        }

        nlohmann::json ToJSON()
        {
            nlohmann::json ret = {
                {"type", "animator"},
                {"material", material->handle},
                {"mesh", renderUnit->mesh->handle},
                {"skeleton", skeleton->handle},
                {"animation", animation->handle}};
            return ret;
        }

        void SetTimeRatio(float ratio)
        {
            timeRatio = loop ? ratio - glm::floor(ratio) : glm::clamp(ratio, 0.0f, 1.0f);
        }

    private:
        vke_ds::id64_t renderID;

        void init(const vke_common::Transform &transform, std::shared_ptr<const vke_render::Mesh> &mesh)
        {
            timeRatio = 0.0f;
            playbackSpeed = 1.0f;
            playing = true;
            loop = true;

            int numSOAJoints = skeleton->skeleton.num_soa_joints();
            locals.resize(numSOAJoints);
            int numJoints = skeleton->skeleton.num_joints();
            models.resize(numJoints);
            context.Resize(numJoints);
            skinningMatrices.resize(mesh->joints.size());

            for (int i = 0; i < vke_render::MAX_FRAMES_IN_FLIGHT; ++i)
                skeletonBuffers.emplace_back(sizeof(float) * 16 * MAX_BONE_PER_SKELETON, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = 0;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            layoutBinding.descriptorCount = 1;
            layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

            vke_render::DescriptorSetInfo descriptorSetInfo;
            descriptorSetInfo.AddCnt(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &layoutBinding;
            vkCreateDescriptorSetLayout(vke_render::globalLogicalDevice, &layoutInfo, nullptr, &(descriptorSetInfo.layout));

            descriptorSets.resize(vke_render::MAX_FRAMES_IN_FLIGHT);
            for (int i = 0; i < vke_render::MAX_FRAMES_IN_FLIGHT; ++i)
                descriptorSets[i] = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(descriptorSetInfo);

            VkWriteDescriptorSet descriptorSetWrite{};
            for (int i = 0; i < vke_render::MAX_FRAMES_IN_FLIGHT; ++i)
            {
                VkDescriptorBufferInfo bufferInfo = skeletonBuffers[i].GetDescriptorBufferInfo();
                vke_render::ConstructDescriptorSetWrite(descriptorSetWrite, descriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
                vkUpdateDescriptorSets(vke_render::globalLogicalDevice, 1, &descriptorSetWrite, 0, nullptr);
            }

            renderUnit = std::make_unique<vke_render::RenderUnit>(mesh, &transform.model, sizeof(glm::mat4), descriptorSets[0]);

            const auto &names = skeleton->skeleton.joint_names();
            for (auto &n : names)
            {
                std::cout << std::string(n) << "\n";
            }
        }

        void update(uint32_t currentFrame)
        {
            if (playing)
            {
                float dt = vke_common::TimeManager::GetInstance()->deltaTime;
                float newRatio = timeRatio + dt * playbackSpeed / animation->Duration();
                SetTimeRatio(newRatio);
            }

            ozz::animation::SamplingJob sampling_job;
            sampling_job.animation = &(animation->animation);
            sampling_job.context = &context;
            sampling_job.ratio = timeRatio;
            sampling_job.output = make_span(locals);
            if (!sampling_job.Run())
                VKE_LOG_ERROR("SAMPLING FAIL")

            ozz::animation::LocalToModelJob ltm_job;
            ltm_job.skeleton = &(skeleton->skeleton);
            ltm_job.input = make_span(locals);
            ltm_job.output = make_span(models);
            if (!ltm_job.Run())
                VKE_LOG_ERROR("LTM FAIL")

            const vke_render::Mesh &mesh = *(renderUnit->mesh);
            for (int i = 0; i < skinningMatrices.size(); ++i)
            {
                int idx = mesh.joints[i];
                skinningMatrices[i] = i < mesh.invBindMatrices.size()
                                          ? models[idx] * mesh.invBindMatrices[i]
                                          : models[idx];
            }

            float *bufferp = (float *)(skeletonBuffers[currentFrame].data);
            for (int i = 0; i < skinningMatrices.size(); ++i)
                for (int j = 0; j < 4; ++j)
                    ozz::math::StorePtrU(skinningMatrices[i].cols[j], bufferp + (i << 4) + (j << 2));
            renderUnit->perUnitDescriptorSet = descriptorSets[currentFrame];
        }

        float timeRatio;
        float playbackSpeed;
        bool playing;
        bool loop;
        ozz::animation::SamplingJob::Context context;
        ozz::vector<ozz::math::SoaTransform> locals;
        ozz::vector<ozz::math::Float4x4> models;
        ozz::vector<ozz::math::Float4x4> skinningMatrices;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<vke_render::HostCoherentBuffer> skeletonBuffers;
    };
}

#endif