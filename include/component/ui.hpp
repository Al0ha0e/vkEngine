#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include <component/transform.hpp>
#include <render/render.hpp>
#include <limits>
#include <memory>
#include <vector>
#include <utility>

namespace vke_render
{
    class Layered2DRenderUnit;
}

namespace vke_component
{
    class UIComponent
    {
    public:
        static constexpr vke_ds::id32_t INVALID_ID = std::numeric_limits<vke_ds::id32_t>::max();

        explicit UIComponent(const vke_common::Transform &transform,
                             std::shared_ptr<vke_render::Material> componentMaterial)
            : material(std::move(componentMaterial))
        {
            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();

            if (material == nullptr)
                material = renderer->GetDefaultMaterial();

            id = renderer->AllocateUnit(material, transform.model, quadIDs);
            renderUnit = renderer->GetUnit(id);
        }

        virtual ~UIComponent() {}

        UIComponent(const UIComponent &) = delete;
        UIComponent &operator=(const UIComponent &) = delete;
        UIComponent(UIComponent &&other) noexcept
            : material(std::move(other.material)),
              quadIDs(std::exchange(other.quadIDs, {})),
              id(std::exchange(other.id, INVALID_ID)),
              renderUnit(std::exchange(other.renderUnit, nullptr)) {}

        UIComponent &operator=(UIComponent &&other) noexcept
        {
            if (this != &other)
            {
                Unload();
                material = std::move(other.material);
                quadIDs = std::exchange(other.quadIDs, {});
                id = std::exchange(other.id, INVALID_ID);
                renderUnit = std::exchange(other.renderUnit, nullptr);
            }
            return *this;
        }

        void Unload()
        {
            if (!isLoaded())
                return;
            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();
            renderer->DestroyUnit(id);
            vke_render::Renderer::GetQuadManager()->Release(quadIDs);
            quadIDs.clear();

            id = INVALID_ID;
            renderUnit = nullptr;
        }

        void OnTransformed(const vke_common::Transform &newTransform)
        {
            if (isLoaded())
                renderUnit->modelMatrix = newTransform.model;
        }

        vke_ds::id32_t GetID() const { return id; }
        vke_render::Layered2DRenderUnit *GetRenderUnit() const { return renderUnit; }
        const std::shared_ptr<vke_render::Material> &GetMaterial() const { return material; }

    protected:
        bool setGeometry(std::vector<vke_render::QuadInstanceGPU> newQuads)
        {
            if (!setQuads(std::move(newQuads)))
                return false;

            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();
            return renderer->UpdateUnitQuadIDs(id, quadIDs);
        }

        void setQuadColor(const glm::vec4 &color)
        {
            vke_render::QuadManager *quadManager = vke_render::Renderer::GetQuadManager();
            for (vke_render::QuadID quadID : quadIDs)
                quadManager->UpdateColor(quadID, color);
        }

        bool isLoaded() const { return id != INVALID_ID; }

        std::shared_ptr<vke_render::Material> material;
        std::vector<vke_render::QuadID> quadIDs;

    private:
        vke_ds::id32_t id = INVALID_ID;
        vke_render::Layered2DRenderUnit *renderUnit = nullptr;

        bool setQuads(std::vector<vke_render::QuadInstanceGPU> newQuads)
        {
            vke_render::QuadManager *quadManager = vke_render::Renderer::GetQuadManager();
            if (!quadManager->CanAllocate(newQuads.size(), quadIDs.size()))
                return false;

            quadManager->Release(quadIDs);
            std::vector<vke_render::QuadID> newQuadIDs;
            quadManager->Allocate(newQuads, newQuadIDs);
            quadIDs = std::move(newQuadIDs);
            return true;
        }
    };
}

#endif
