#ifndef RDOBJECT_H
#define RDOBJECT_H

#include <render/render.hpp>
#include <render/buffer.hpp>
#include <asset.hpp>
#include <component/transform.hpp>

namespace vke_component
{
    class RenderableObject
    {
    public:
        std::shared_ptr<vke_render::Material> material;
        std::vector<glm::ivec4> textureIndices;
        std::unique_ptr<vke_render::RenderUnit> renderUnit;

        RenderableObject(
            const vke_common::Transform &transform,
            std::shared_ptr<vke_render::Material> &mat,
            std::shared_ptr<const vke_render::Mesh> &mesh)
            : material(mat)
        {
            init(transform, mesh);
        }

        RenderableObject(const vke_common::Transform &transform, const nlohmann::json &json)
        {
            material = vke_common::AssetManager::LoadMaterial(json["material"]);
            std::shared_ptr<const vke_render::Mesh> mesh = vke_common::AssetManager::LoadMesh(json["mesh"]);
            if (json.contains("textureIndices"))
            {
                auto &indices = json["textureIndices"];
                for (auto &index : indices)
                    textureIndices.push_back(glm::ivec4(index[0].get<int>(), index[1].get<int>(), index[2].get<int>(), 0));
            }
            init(transform, mesh);
        }

        ~RenderableObject()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderer->GetGBufferPass()->RemoveUnit(material.get(), renderID);
        }

        nlohmann::json ToJSON()
        {
            nlohmann::json ret = {
                {"type", "renderableObject"},
                {"material", material->handle},
                {"mesh", renderUnit->mesh->handle}};
            return ret;
        }

    private:
        vke_ds::id64_t renderID;

        void init(const vke_common::Transform &transform, std::shared_ptr<const vke_render::Mesh> &mesh)
        {
            renderUnit = textureIndices.size() == 0 ? std::make_unique<vke_render::RenderUnit>(mesh, &transform.model, sizeof(glm::mat4))
                                                    : std::make_unique<vke_render::RenderUnit>(mesh,
                                                                                               std::vector<vke_render::PushConstantInfo>{vke_render::PushConstantInfo(sizeof(glm::mat4), &transform.model, 0),
                                                                                                                                         vke_render::PushConstantInfo(sizeof(glm::ivec4), textureIndices.data(), sizeof(glm::mat4))},
                                                                                               1);

            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            renderer->GetGBufferPass()->AddUnit(material, renderUnit.get());
        }
    };
}

#endif