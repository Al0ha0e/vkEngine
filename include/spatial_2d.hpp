#ifndef SPATIAL_2D_H
#define SPATIAL_2D_H

#include <ds/id_allocator.hpp>
#include <glm/vec2.hpp>
#include <limits>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace vke_common
{
    struct AABB2D
    {
        glm::vec2 min{0.0f};
        glm::vec2 max{0.0f};

        AABB2D() = default;
        AABB2D(const glm::vec2 &min, const glm::vec2 &max) : min(min), max(max) {}

        bool Contains(const glm::vec2 &point) const;
        bool Overlaps(const AABB2D &other) const;
    };

    struct Spatial2DUnit
    {
        static constexpr vke_ds::id32_t INVALID_LAYER = std::numeric_limits<vke_ds::id32_t>::max();

        vke_ds::id32_t id = 0;
        AABB2D bounds;
        float zIndex = 0.0f;
        vke_ds::id32_t layer = INVALID_LAYER;
    };

    class Spatial2DLayer
    {
    public:
        explicit Spatial2DLayer(vke_ds::id32_t id) : id(id) {}

        const vke_ds::id32_t id;

        bool CanInsert(const Spatial2DUnit &unit) const;
        void Insert(Spatial2DUnit &unit);
        void Remove(vke_ds::id32_t unitID);
        bool Empty() const { return units.empty(); }

        std::vector<vke_ds::id32_t> Query(const glm::vec2 &point) const;
        std::vector<vke_ds::id32_t> Query(const AABB2D &bounds) const;

        const std::unordered_map<vke_ds::id32_t, Spatial2DUnit *> &GetUnits() const { return units; }

    private:
        std::unordered_map<vke_ds::id32_t, Spatial2DUnit *> units;
    };

    class Spatial2DLayerManager
    {
    public:
        static Spatial2DLayerManager *GetInstance();
        static Spatial2DLayerManager *Init();
        static void Dispose();

        vke_ds::id32_t CreateUnit(const AABB2D &bounds, float zIndex);
        bool ReinsertUnit(vke_ds::id32_t unitID, const AABB2D &bounds, float zIndex);
        void RemoveUnit(vke_ds::id32_t unitID);

        Spatial2DUnit *GetUnit(vke_ds::id32_t unitID);
        const Spatial2DUnit *GetUnit(vke_ds::id32_t unitID) const;
        const Spatial2DLayer *GetLayer(vke_ds::id32_t layerID) const;

        std::vector<vke_ds::id32_t> Query(const glm::vec2 &point) const;
        std::vector<vke_ds::id32_t> Query(const AABB2D &bounds) const;
        const std::vector<vke_ds::id32_t> &GetLayerOrder() const { return layerOrder; }

    private:
        static Spatial2DLayerManager *instance;

        Spatial2DLayerManager() = default;
        Spatial2DLayerManager(const Spatial2DLayerManager &) = delete;
        Spatial2DLayerManager &operator=(const Spatial2DLayerManager &) = delete;

        vke_ds::NaiveIDAllocator<vke_ds::id32_t> unitAllocator;
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> layerAllocator;
        std::map<vke_ds::id32_t, Spatial2DUnit> units;
        std::map<vke_ds::id32_t, Spatial2DLayer> layers;
        std::vector<vke_ds::id32_t> layerOrder;

        vke_ds::id32_t insertUnit(Spatial2DUnit &unit);
        void removeFromLayer(Spatial2DUnit &unit);
    };
}

#endif
