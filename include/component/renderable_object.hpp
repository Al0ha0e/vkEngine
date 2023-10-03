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
            // TODO init buffer
            renderID = vke_render::OpaqueRenderer::AddUnit(material, mesh, buffers);
        }

        ~RenderableObject() {}

        void OnTransformed(vke_common::TransformParameter &param) override
        {
        }

    private:
        uint64_t renderID;
    };
}

#endif