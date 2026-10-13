#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include <component/transform.hpp>
#include <render/render.hpp>
#include <spatial_2d.hpp>
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
            : modelMatrix(transform.model), material(std::move(componentMaterial))
        {
            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();

            if (material == nullptr)
                material = renderer->GetDefaultMaterial();

            vke_common::Spatial2DLayerManager *spatialManager = vke_common::Spatial2DLayerManager::GetInstance();

            id = spatialManager->CreateUnit(getWorldBounds(), getZIndex());
            const vke_common::Spatial2DUnit *spatialUnit = spatialManager->GetUnit(id);
            renderer->AllocateUnit(id, material, modelMatrix, quadIDs);
            renderer->AddUnitToLayer(id, spatialUnit->layer);

            renderUnit = renderer->GetUnit(id);
            renderer->SetLayerOrder(spatialManager->GetLayerOrder());
        }

        virtual ~UIComponent() {}

        UIComponent(const UIComponent &) = delete;
        UIComponent &operator=(const UIComponent &) = delete;
        UIComponent(UIComponent &&other) noexcept
            : modelMatrix(other.modelMatrix),
              material(std::move(other.material)),
              quadIDs(std::exchange(other.quadIDs, {})),
              id(std::exchange(other.id, INVALID_ID)),
              localBounds(other.localBounds),
              renderUnit(std::exchange(other.renderUnit, nullptr)) {}

        UIComponent &operator=(UIComponent &&other) noexcept
        {
            if (this != &other)
            {
                Unload();
                modelMatrix = other.modelMatrix;
                material = std::move(other.material);
                quadIDs = std::exchange(other.quadIDs, {});
                id = std::exchange(other.id, INVALID_ID);
                localBounds = other.localBounds;
                renderUnit = std::exchange(other.renderUnit, nullptr);
            }
            return *this;
        }

        void Unload()
        {
            if (!IsLoaded())
                return;
            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();
            renderer->DestroyUnit(id);
            vke_render::Renderer::GetQuadManager()->Release(quadIDs);
            quadIDs.clear();
            vke_common::Spatial2DLayerManager *spatialManager = vke_common::Spatial2DLayerManager::GetInstance();
            spatialManager->RemoveUnit(id);
            renderer->SetLayerOrder(spatialManager->GetLayerOrder());

            id = INVALID_ID;
            renderUnit = nullptr;
        }

        void OnTransformed(const vke_common::Transform &newTransform)
        {
            modelMatrix = newTransform.model;
            if (IsLoaded())
            {
                renderUnit->modelMatrix = modelMatrix;
                syncSpatialLayer();
            }
        }

        vke_ds::id32_t GetID() const { return id; }
        const vke_common::AABB2D &GetLocalBounds() const { return localBounds; }
        vke_render::Layered2DRenderUnit *GetRenderUnit() const { return renderUnit; }
        const std::shared_ptr<vke_render::Material> &GetMaterial() const { return material; }

    protected:
        bool SetGeometry(std::vector<vke_render::QuadInstanceGPU> newQuads,
                         const vke_common::AABB2D &newLocalBounds)
        {
            if (!setQuads(std::move(newQuads)))
                return false;

            vke_render::Layered2DRenderer *renderer = vke_render::Renderer::GetLayered2DRenderer();
            if (!renderer->UpdateUnitQuadIDs(id, quadIDs))
                return false;
            localBounds = newLocalBounds;
            syncSpatialLayer();
            return true;
        }

        void SetQuadColor(const glm::vec4 &color)
        {
            vke_render::QuadManager *quadManager = vke_render::Renderer::GetQuadManager();
            for (vke_render::QuadID quadID : quadIDs)
                quadManager->UpdateColor(quadID, color);
        }

        bool IsLoaded() const { return id != INVALID_ID; }

        glm::mat4 modelMatrix;
        std::shared_ptr<vke_render::Material> material;
        std::vector<vke_render::QuadID> quadIDs;

    private:
        vke_ds::id32_t id = INVALID_ID;
        vke_common::AABB2D localBounds;
        vke_render::Layered2DRenderUnit *renderUnit = nullptr;

        float getZIndex() const { return modelMatrix[3].z; }

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
                const glm::vec2 point = glm::vec2(modelMatrix * glm::vec4(corner, 0.0f, 1.0f));
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
