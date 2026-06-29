#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include <component/transform.hpp>
#include <render/render.hpp>
#include <spatial_2d.hpp>
#include <limits>
#include <memory>
#include <vector>

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
                             std::shared_ptr<vke_render::Material> material,
                             vke_render::CPUGlyphData *glyphData)
            : transform(&transform), material(std::move(material)), glyphData(glyphData) {}
        virtual ~UIComponent()
        {
            glyphData->Release(glyphIDs);
        }

        UIComponent(const UIComponent &) = delete;
        UIComponent &operator=(const UIComponent &) = delete;
        UIComponent(UIComponent &&) = delete;
        UIComponent &operator=(UIComponent &&) = delete;

        bool LoadToEngine()
        {
            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();

            if (material == nullptr)
                material = renderer->GetDefaultMaterial();

            vke_common::Spatial2DLayerManager *spatialManager = vke_common::Spatial2DLayerManager::GetInstance();

            id = spatialManager->CreateUnit(getWorldBounds(), getZIndex());
            const vke_common::Spatial2DUnit *spatialUnit = spatialManager->GetUnit(id);
            if (!renderer->AllocateUnit(id, material, &transform->model, glyphIDs) ||
                !renderer->AddUnitToLayer(id, spatialUnit->layer))
            {
                renderer->DestroyUnit(id);
                spatialManager->RemoveUnit(id);
                id = INVALID_ID;
                return false;
            }

            renderUnit = renderer->GetUnit(id);
            renderer->SetLayerOrder(spatialManager->GetLayerOrder());
            return true;
        }

        void UnloadFromEngine()
        {
            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();
            renderer->DestroyUnit(id);
            vke_common::Spatial2DLayerManager *spatialManager = vke_common::Spatial2DLayerManager::GetInstance();
            spatialManager->RemoveUnit(id);
            renderer->SetLayerOrder(spatialManager->GetLayerOrder());

            id = INVALID_ID;
            renderUnit = nullptr;
        }

        void OnTransformed(vke_common::Transform &newTransform)
        {
            transform = &newTransform;
            syncSpatialLayer();
        }

        vke_ds::id32_t GetID() const { return id; }
        const vke_common::AABB2D &GetLocalBounds() const { return localBounds; }
        vke_render::Layered2DRenderUnit *GetRenderUnit() const { return renderUnit; }
        const std::shared_ptr<vke_render::Material> &GetMaterial() const { return material; }

    protected:
        bool SetGeometry(std::vector<vke_render::GlyphInstanceGPU> newGlyphs,
                         const vke_common::AABB2D &newLocalBounds)
        {
            if (!setGlyphs(std::move(newGlyphs)))
                return false;
            if (!IsLoaded())
            {
                localBounds = newLocalBounds;
                return true;
            }

            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();
            if (!renderer->UpdateUnitGlyphIDs(id, glyphIDs))
                return false;
            localBounds = newLocalBounds;
            syncSpatialLayer();
            return true;
        }

        void SetGlyphColor(const glm::vec4 &color)
        {
            for (vke_render::GlyphID glyphID : glyphIDs)
                glyphData->UpdateColor(glyphID, color);
        }

        bool IsLoaded() const { return id != INVALID_ID; }

        const vke_common::Transform *transform;
        std::shared_ptr<vke_render::Material> material;
        vke_render::CPUGlyphData *glyphData;
        std::vector<vke_render::GlyphID> glyphIDs;

    private:
        vke_ds::id32_t id = INVALID_ID;
        vke_common::AABB2D localBounds;
        vke_render::Layered2DRenderUnit *renderUnit = nullptr;

        float getZIndex() const { return transform->model[3].z; }

        vke_common::AABB2D getWorldBounds() const
        {
            const glm::vec2 corners[] = {
                localBounds.min,
                glm::vec2(localBounds.max.x, localBounds.min.y),
                localBounds.max,
                glm::vec2(localBounds.min.x, localBounds.max.y)};

            glm::vec2 minimum(std::numeric_limits<float>::max());
            glm::vec2 maximum(std::numeric_limits<float>::lowest());
            for (const glm::vec2 &corner : corners)
            {
                const glm::vec2 point = glm::vec2(transform->model * glm::vec4(corner, 0.0f, 1.0f));
                minimum = glm::min(minimum, point);
                maximum = glm::max(maximum, point);
            }
            return vke_common::AABB2D(minimum, maximum);
        }

        void syncSpatialLayer()
        {
            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();
            vke_common::Spatial2DLayerManager *spatialManager = vke_common::Spatial2DLayerManager::GetInstance();

            const vke_common::Spatial2DUnit *oldUnit = spatialManager->GetUnit(id);
            const vke_ds::id32_t oldLayer = oldUnit->layer;
            spatialManager->ReinsertUnit(id, getWorldBounds(), getZIndex());
            const vke_common::Spatial2DUnit *newUnit = spatialManager->GetUnit(id);

            if (oldLayer != newUnit->layer)
            {
                renderer->RemoveUnitFromLayer(id, oldLayer);
                renderer->AddUnitToLayer(id, newUnit->layer);
            }
            renderer->SetLayerOrder(spatialManager->GetLayerOrder());
        }

        bool setGlyphs(std::vector<vke_render::GlyphInstanceGPU> newGlyphs)
        {
            if (!glyphData->CanAllocate(newGlyphs.size(), glyphIDs.size()))
                return false;

            glyphData->Release(glyphIDs);
            std::vector<vke_render::GlyphID> newGlyphIDs;
            glyphData->Allocate(newGlyphs, newGlyphIDs);
            glyphIDs = std::move(newGlyphIDs);
            return true;
        }
    };
}

#endif
