#ifndef RDOBJECT_H
#define RDOBJECT_H

#include <render/render.hpp>
#include <render/buffer.hpp>
#include <gameobject.hpp>

namespace vke_component
{
    class RenderableObject : public vke_common::Component
    {
    public:
        vke_render::Material *material;
        vke_render::Mesh *mesh;
        std::vector<vke_render::HostCoherentBuffer> buffers;

        RenderableObject(
            vke_render::Material *mat,
            vke_render::Mesh *msh,
            vke_common::GameObject *obj)
            : material(mat), mesh(msh), Component(obj)
        {
            vke_render::HostCoherentBuffer buffer(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            buffer.ToBuffer(0, &gameObject->transform.model, sizeof(glm::mat4));
            buffers.push_back(std::move(buffer));

            renderID = vke_render::OpaqueRenderer::AddUnit(material, mesh, buffers);
        }

        ~RenderableObject() {}

        void OnTransformed(vke_common::TransformParameter &param) override
        {
            buffers[0].ToBuffer(0, &param.model, sizeof(glm::mat4));
        }

    private:
        uint64_t renderID;
    };
}

#endif