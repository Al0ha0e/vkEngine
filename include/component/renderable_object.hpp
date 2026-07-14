#ifndef RDOBJECT_H
#define RDOBJECT_H

#include <render/render.hpp>
#include <render/buffer.hpp>
#include <asset/asset_manager.hpp>
#include <component/transform.hpp>

namespace vke_component
{
    class RenderableObject
    {
    public:
        std::shared_ptr<vke_render::Material> material;
        std::vector<glm::ivec4> textureIndices;
        std::unique_ptr<vke_render::RenderUnit> renderUnit;
        std::unique_ptr<vke_render::RenderUnit> shadowRenderUnit;
        bool castsShadow;

        RenderableObject(
            const vke_common::Transform &transform,
            std::shared_ptr<vke_render::Material> &mat,
            std::shared_ptr<const vke_render::Mesh> &mesh)
            : material(mat), castsShadow(true), renderID(0), shadowRenderID(0)
        {
            init(transform, mesh);
        }

        RenderableObject(const vke_common::Transform &transform, const nlohmann::json &json)
            : castsShadow(json.contains("castsShadow") ? json["castsShadow"].get<bool>() : true), renderID(0), shadowRenderID(0)
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

        ~RenderableObject() {}

        void LoadToEngine()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            if (material->renderMode == vke_render::MaterialRenderMode::BLEND_MODE)
                renderID = renderer->GetTransparentPass()->AddUnit(material, renderUnit.get());
            else
                renderID = renderer->GetGBufferPass()->AddUnit(material, renderUnit.get());
            if (castsShadow && material->renderMode != vke_render::MaterialRenderMode::BLEND_MODE)
            {
                vke_render::ShadowPass *shadowPass = renderer->GetShadowPass();
                if (shadowPass != nullptr)
                    shadowRenderID = shadowPass->AddUnit(shadowRenderUnit.get());
            }
        }

        void UnloadFromEngine()
        {
            unloadFromEngine(material, castsShadow);
        }

        void SetMaterial(std::shared_ptr<vke_render::Material> &mat)
        {
            if (mat == nullptr || mat == material)
                return;

            const bool loaded = renderID != 0;
            if (loaded)
                unloadFromEngine(material, castsShadow);
            material = mat;
            if (loaded)
                LoadToEngine();
        }

        void SetCastShadow(bool castShadow)
        {
            if (castShadow == castsShadow)
                return;

            castsShadow = castShadow;
            if (renderID == 0 || material == nullptr ||
                material->renderMode == vke_render::MaterialRenderMode::BLEND_MODE)
                return;

            vke_render::ShadowPass *shadowPass = vke_render::Renderer::GetInstance()->GetShadowPass();
            if (shadowPass == nullptr)
                return;

            if (castsShadow && shadowRenderID == 0)
                shadowRenderID = shadowPass->AddUnit(shadowRenderUnit.get());
            else if (!castsShadow && shadowRenderID != 0)
            {
                shadowPass->RemoveUnit(shadowRenderID);
                shadowRenderID = 0;
            }
        }

        void SetMesh(std::shared_ptr<const vke_render::Mesh> &mesh)
        {
            if (renderUnit != nullptr)
                renderUnit->mesh = mesh;
            if (shadowRenderUnit != nullptr)
                shadowRenderUnit->mesh = mesh;
        }

        nlohmann::json ToJSON()
        {
            nlohmann::json ret = {
                {"type", "renderableObject"},
                {"material", material->handle},
                {"mesh", renderUnit->mesh->handle},
                {"castsShadow", castsShadow}};
            return ret;
        }

    private:
        vke_ds::id64_t renderID;
        vke_ds::id64_t shadowRenderID;

        void unloadFromEngine(std::shared_ptr<vke_render::Material> &mat, bool castShadow)
        {
            if (renderID == 0 || mat == nullptr)
                return;

            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            if (mat->renderMode == vke_render::MaterialRenderMode::BLEND_MODE)
                renderer->GetTransparentPass()->RemoveUnit(renderID);
            else
                renderer->GetGBufferPass()->RemoveUnit(mat.get(), renderID);
            renderID = 0;

            if (castShadow && mat->renderMode != vke_render::MaterialRenderMode::BLEND_MODE && shadowRenderID != 0)
            {
                vke_render::ShadowPass *shadowPass = renderer->GetShadowPass();
                if (shadowPass != nullptr)
                    shadowPass->RemoveUnit(shadowRenderID);
                shadowRenderID = 0;
            }
        }

        void init(const vke_common::Transform &transform, std::shared_ptr<const vke_render::Mesh> &mesh)
        {
            renderUnit = textureIndices.size() == 0 ? std::make_unique<vke_render::RenderUnit>(mesh, &transform.model, static_cast<uint32_t>(sizeof(glm::mat4)))
                                                    : std::make_unique<vke_render::RenderUnit>(mesh,
                                                                                               std::vector<vke_render::PushConstantInfo>{vke_render::PushConstantInfo(static_cast<uint32_t>(sizeof(glm::mat4)), &transform.model, true, 0),
                                                                                                                                         vke_render::PushConstantInfo(static_cast<uint32_t>(sizeof(glm::ivec4)), textureIndices.data(), false, static_cast<uint32_t>(sizeof(glm::mat4)))},
                                                                                               1);
            shadowRenderUnit = std::make_unique<vke_render::RenderUnit>(mesh, &transform.model, static_cast<uint32_t>(sizeof(glm::mat4)));
        }
    };
}

#endif
