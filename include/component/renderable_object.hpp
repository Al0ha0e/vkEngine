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
        std::vector<glm::ivec4> textureIndices;
        std::unique_ptr<vke_render::RenderUnit> renderUnit;

        RenderableObject(
            std::shared_ptr<vke_render::Material> &mat,
            std::shared_ptr<const vke_render::Mesh> &mesh,
            vke_common::GameObject *obj)
            : material(mat), Component(obj)
        {
            init(mesh);
        }

        RenderableObject(vke_common::GameObject *obj, const nlohmann::json &json) : Component(obj)
        {
            material = vke_common::AssetManager::LoadMaterial(json["material"]);
            std::shared_ptr<const vke_render::Mesh> mesh = vke_common::AssetManager::LoadMesh(json["mesh"]);
            if (json.contains("textureIndices"))
            {
                auto &indices = json["textureIndices"];
                for (auto &index : indices)
                    textureIndices.push_back(glm::ivec4(index[0].get<int>(), index[1].get<int>(), index[2].get<int>(), 0));
            }
            init(mesh);
        }

        ~RenderableObject()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderer->GetGBufferPass()->RemoveUnit(material.get(), renderID);
        }

        std::string ToJSON() override
        {
            std::string ret = "{\n\"type\":\"renderableObject\"";
            ret += ",\n\"material\": " + std::to_string(material->handle);
            ret += ",\n\"mesh\": " + std::to_string(renderUnit->mesh->handle);
            ret += "\n}";
            return ret;
        }

    private:
        vke_ds::id64_t renderID;

        void init(std::shared_ptr<const vke_render::Mesh> &mesh)
        {
            renderUnit = textureIndices.size() == 0 ? std::make_unique<vke_render::RenderUnit>(mesh, &gameObject->transform.model, sizeof(glm::mat4))
                                                    : std::make_unique<vke_render::RenderUnit>(mesh,
                                                                                               std::vector<vke_render::PushConstantInfo>{vke_render::PushConstantInfo(sizeof(glm::mat4), &gameObject->transform.model, 0),
                                                                                                                                         vke_render::PushConstantInfo(sizeof(glm::ivec4), textureIndices.data(), sizeof(glm::mat4))},
                                                                                               1);

            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderer->GetGBufferPass()->AddUnit(material, renderUnit.get());
        }
    };
}

#endif