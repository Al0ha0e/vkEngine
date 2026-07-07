#include <editor/wireframe_collision_pass.hpp>
#include <scene.hpp>
#include <asset.hpp>
#include <component/rigidbody.hpp>
#include <component/sensor.hpp>
#include <physics/physics.hpp>
#include <render/render.hpp>

namespace vke_editor
{
    static const vke_common::AssetHandle WIREFRAME_COLLISION_SHADER_ID = 512;

    WireframeCollisionPass::WireframeCollisionPass(vke_render::RenderContext *ctx, VkDescriptorSet *globalDescriptorSets)
        : vke_render::RenderPassBase(vke_render::WIREFRAME_COLLISION_PASS, ctx, globalDescriptorSets),
          hdrColorManager(nullptr), taskNodeID(0), hdrColorImageIndex(0)
    {
    }

    void WireframeCollisionPass::Init(int subpassID,
                                      vke_render::FrameGraph &frameGraph,
                                      std::map<std::string, vke_ds::id32_t> &blackboard,
                                      vke_render::ResourceNodeIDMap &currentResourceNodeID)
    {
        vke_render::RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);

        context = vke_render::Renderer::GetInstance()->context;
        hdrColorManager = vke_render::Renderer::GetHDRColorManager();

        shader = vke_common::AssetManager::LoadVertFragShaderUnique(WIREFRAME_COLLISION_SHADER_ID);

        createGraphicsPipeline();
        constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
    }

    void WireframeCollisionPass::constructFrameGraph(vke_render::FrameGraph &frameGraph,
                                                     std::map<std::string, vke_ds::id32_t> &blackboard,
                                                     vke_render::ResourceNodeIDMap &currentResourceNodeID)
    {
        hdrColorImageIndex = hdrColorManager->GetLatestImageIndex();
        const vke_ds::id32_t outputNodeID = frameGraph.AllocResourceNode(
            "wireframeOutHDRColor", hdrColorManager->GetResourceID(hdrColorImageIndex));
        taskNodeID = frameGraph.AllocTaskNode(
            "wireframeCollision", vke_render::RENDER_TASK,
            std::bind(&WireframeCollisionPass::Render, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

        frameGraph.AddTaskNodeResourceRef(
            taskNodeID, hdrColorManager->GetResourceNodeID(hdrColorImageIndex), outputNodeID,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        const vke_ds::id32_t depthResourceID = blackboard.at("depthAttachment");
        frameGraph.AddTaskNodeResourceRef(
            taskNodeID, currentResourceNodeID.at(depthResourceID), 0,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        hdrColorManager->UpdateResourceNode(hdrColorImageIndex, outputNodeID);
    }

    void WireframeCollisionPass::createGraphicsPipeline()
    {
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = 1;
        static constexpr VkFormat hdrFormat = vke_render::HDRColorManager::HDR_COLOR_FORMAT;
        renderingInfo.pColorAttachmentFormats = &hdrFormat;
        renderingInfo.depthAttachmentFormat = context->depthFormat;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 4.0f;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo blendState{};
        blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendState.attachmentCount = 1;
        blendState.pAttachments = &blendAttachment;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &blendState;

        static const std::vector<uint32_t> vertexAttrSizes = { sizeof(glm::vec3) };
        pipeline = std::make_unique<vke_render::GraphicsPipeline>(
            shader, vertexAttrSizes, VK_VERTEX_INPUT_RATE_VERTEX, pipelineInfo);
    }

    void WireframeCollisionPass::extractCollisionLines(entt::entity entity, std::vector<glm::vec3> &outVertices)
    {
        outVertices.clear();

        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        if (!scene || !scene->registry.valid(entity))
            return;

        JPH::BodyID bodyID;

        auto *rigidBody = scene->registry.try_get<vke_component::RigidBody>(entity);
        auto *sensor = scene->registry.try_get<vke_component::Sensor>(entity);

        if (rigidBody && rigidBody->shape && rigidBody->shape->shapeRef)
        {
            bodyID = rigidBody->bodyID;
        }
        else if (sensor && sensor->shape && sensor->shape->shapeRef)
        {
            bodyID = sensor->bodyID;
        }
        else
        {
            return;
        }

        JPH::BodyInterface &bodyInterface = vke_physics::PhysicsManager::GetBodyInterface();
        if (!bodyInterface.IsAdded(bodyID))
            return;

        JPH::TransformedShape ts = bodyInterface.GetTransformedShape(bodyID);

        struct TriangleCollector final : JPH::TransformedShapeCollector
        {
            std::vector<glm::vec3> &outVertices;

            TriangleCollector(std::vector<glm::vec3> &out)
                : outVertices(out) {}

            void AddHit(const JPH::TransformedShape &leaf) override
            {
                JPH::Shape::GetTrianglesContext ctx;
                leaf.GetTrianglesStart(ctx, JPH::AABox::sBiggest(), JPH::RVec3::sZero());

                const int batchSize = 1024;
                JPH::Float3 triangleVerts[batchSize * 3];
                int numTriangles;
                while ((numTriangles = leaf.GetTrianglesNext(ctx, batchSize, triangleVerts, nullptr)) > 0)
                {
                    for (int t = 0; t < numTriangles; t++)
                    {
                        JPH::Float3 &v0 = triangleVerts[t * 3 + 0];
                        JPH::Float3 &v1 = triangleVerts[t * 3 + 1];
                        JPH::Float3 &v2 = triangleVerts[t * 3 + 2];

                        outVertices.emplace_back(v0.x, v0.y, v0.z);
                        outVertices.emplace_back(v1.x, v1.y, v1.z);
                        outVertices.emplace_back(v1.x, v1.y, v1.z);
                        outVertices.emplace_back(v2.x, v2.y, v2.z);
                        outVertices.emplace_back(v2.x, v2.y, v2.z);
                        outVertices.emplace_back(v0.x, v0.y, v0.z);
                    }
                }
            }
        };

        TriangleCollector collector(outVertices);
        ts.CollectTransformedShapes(JPH::AABox::sBiggest(), collector);
    }

    void WireframeCollisionPass::Render(vke_render::TaskNode &, vke_render::FrameGraph &,
                                        VkCommandBuffer commandBuffer,
                                        uint32_t currentFrame, uint32_t)
    {
        if (selectedEntity == entt::null)
            return;

        std::vector<glm::vec3> lineVertices;
        extractCollisionLines(selectedEntity, lineVertices);
        if (lineVertices.empty())
            return;

        glm::mat4 wireframeModel(1.0f);

        size_t neededBytes = lineVertices.size() * sizeof(glm::vec3);
        auto &buf = vertexBuffers[currentFrame];
        if (!buf || buf->bufferSize < neededBytes)
        {
            buf = std::make_unique<vke_render::HostCoherentBuffer>(
                neededBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        }

        buf->ToBuffer(0, lineVertices.data(), neededBytes);

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = hdrColorManager->GetImageView(hdrColorImageIndex, currentFrame);
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = context->depthImageViews[currentFrame];
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, {context->width, context->height}};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        VkViewport viewport{0.0f, 0.0f, static_cast<float>(context->width),
                            static_cast<float>(context->height), 0.0f, 1.0f};
        VkRect2D scissor{{0, 0}, {context->width, context->height}};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        const vke_render::CameraInfo &camInfo = vke_render::Renderer::GetCameraInfo();
        glm::mat4 viewProj = camInfo.projection * camInfo.view;

        PushConstants pc;
        pc.viewProj = viewProj * wireframeModel;
        pc.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

        pipeline->Bind(commandBuffer);
        vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout,
                           VK_SHADER_STAGE_ALL,
                           0, sizeof(PushConstants), &pc);

        VkBuffer vkBuf = vertexBuffers[currentFrame]->buffer;
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkBuf, &offset);
        vkCmdDraw(commandBuffer, static_cast<uint32_t>(lineVertices.size()), 1, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }
}
