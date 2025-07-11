#ifndef RDOBJECT_H
#define RDOBJECT_H

#include <render/render.hpp>
#include <render/buffer.hpp>
#include <asset.hpp>
#include <gameobject.hpp>

namespace vke_component
{
    class RenderableObject : public vke_common::Component
    {
    public:
        std::shared_ptr<vke_render::Material> material;
        std::shared_ptr<const vke_render::Mesh> mesh;

        RenderableObject(
            std::shared_ptr<vke_render::Material> &mat,
            std::shared_ptr<const vke_render::Mesh> &msh,
            vke_common::GameObject *obj)
            : material(mat), mesh(msh), Component(obj)
        {
            init();
        }

        RenderableObject(vke_common::GameObject *obj, nlohmann::json &json) : Component(obj)
        {
            material = vke_common::AssetManager::LoadMaterial(json["material"]);
            mesh = vke_common::AssetManager::LoadMesh(json["mesh"]);
            init();
        }

        ~RenderableObject()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderer->GetOpaqueRenderer()->RemoveUnit(material.get(), renderID);
        }

        std::string ToJSON() override
        {
            std::string ret = "{\n\"type\":\"renderableObject\"";
            ret += ",\n\"material\": " + std::to_string(material->handle);
            ret += ",\n\"mesh\": " + std::to_string(mesh->handle);
            ret += "\n}";
            return ret;
        }

    private:
        vke_ds::id64_t renderID;

        void init()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderID = renderer->GetOpaqueRenderer()->AddUnit(material, mesh, &gameObject->transform.model, sizeof(glm::mat4));
        }
    };
}

#endif