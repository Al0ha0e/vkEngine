#ifndef RDOBJECT_H
#define RDOBJECT_H

#include <render/render.hpp>
#include <gameobject.hpp>

namespace vke_component
{
    class RenderableObject : public vke_common::Component
    {
    public:
        vke_render::Material *material;
        vke_render::Mesh *mesh;
        std::vector<VkBuffer> buffers;

        RenderableObject(
            vke_render::Material *mat,
            vke_render::Mesh *msh,
            vke_common::GameObject *obj)
            : material(mat), mesh(msh), Component(obj)
        {
            VkBuffer buffer;
            VkDeviceMemory bufferMemory;
            void *mappedBufferMemory;

            VkDeviceSize bufferSize = sizeof(glm::mat4);
            vke_render::RenderEnvironment::CreateBuffer(
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                buffer, bufferMemory);
            vkMapMemory(
                vke_render::RenderEnvironment::GetInstance()->logicalDevice,
                bufferMemory,
                0,
                bufferSize,
                0,
                &mappedBufferMemory);

            memcpy(mappedBufferMemory, &gameObject->transform.model, sizeof(glm::mat4));
            vke_render::OpaqueRenderer::RegisterCamera(buffer);

            buffers.push_back(buffer);
            bufferMemories.push_back(bufferMemory);
            mappedBufferMemories.push_back(mappedBufferMemory);

            renderID = vke_render::OpaqueRenderer::AddUnit(material, mesh, buffers);
        }

        ~RenderableObject() {}

        void OnTransformed(vke_common::TransformParameter &param) override
        {
            memcpy(mappedBufferMemories[0], &param.model, sizeof(glm::mat4));
        }

    private:
        uint64_t renderID;
        std::vector<VkDeviceMemory> bufferMemories;
        std::vector<void *> mappedBufferMemories;
    };
}

#endif