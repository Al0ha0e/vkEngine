#include <spatial_2d.hpp>
#include <algorithm>

namespace vke_common
{
    Spatial2DLayerManager *Spatial2DLayerManager::instance = nullptr;

    Spatial2DLayerManager *Spatial2DLayerManager::GetInstance()
    {
        return instance;
    }

    Spatial2DLayerManager *Spatial2DLayerManager::Init()
    {
        if (instance == nullptr)
            instance = new Spatial2DLayerManager();
        return instance;
    }

    void Spatial2DLayerManager::Dispose()
    {
        delete instance;
        instance = nullptr;
    }

    bool AABB2D::Contains(const glm::vec2 &point) const
    {
        return point.x >= min.x && point.y >= min.y && point.x <= max.x && point.y <= max.y;
    }

    bool AABB2D::Overlaps(const AABB2D &other) const
    {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y;
    }

    bool Spatial2DLayer::CanInsert(const Spatial2DUnit &unit) const
    {
        for (const auto &entry : units)
        {
            const Spatial2DUnit &other = *entry.second;
            if (other.zIndex != unit.zIndex && other.bounds.Overlaps(unit.bounds))
                return false;
        }
        return true;
    }

    void Spatial2DLayer::Insert(Spatial2DUnit &unit)
    {
        units[unit.id] = &unit;
        unit.layer = id;
    }

    void Spatial2DLayer::Remove(vke_ds::id32_t unitID)
    {
        units.erase(unitID);
    }

    std::vector<vke_ds::id32_t> Spatial2DLayer::Query(const glm::vec2 &point) const
    {
        std::vector<vke_ds::id32_t> result;
        for (const auto &entry : units)
            if (entry.second->bounds.Contains(point))
                result.push_back(entry.first);
        return result;
    }

    std::vector<vke_ds::id32_t> Spatial2DLayer::Query(const AABB2D &bounds) const
    {
        std::vector<vke_ds::id32_t> result;
        for (const auto &entry : units)
            if (entry.second->bounds.Overlaps(bounds))
                result.push_back(entry.first);
        return result;
    }

    vke_ds::id32_t Spatial2DLayerManager::CreateUnit(const AABB2D &bounds, float zIndex)
    {
        const vke_ds::id32_t id = unitAllocator.Alloc();
        auto [it, inserted] = units.emplace(id, Spatial2DUnit{id, bounds, zIndex, Spatial2DUnit::INVALID_LAYER});
        insertUnit(it->second);
        return id;
    }

    bool Spatial2DLayerManager::ReinsertUnit(vke_ds::id32_t unitID, const AABB2D &bounds, float zIndex)
    {
        auto it = units.find(unitID);
        if (it == units.end())
            return false;

        Spatial2DUnit &unit = it->second;
        removeFromLayer(unit);
        unit.bounds = bounds;
        unit.zIndex = zIndex;
        insertUnit(unit);
        return true;
    }

    void Spatial2DLayerManager::RemoveUnit(vke_ds::id32_t unitID)
    {
        auto it = units.find(unitID);
        if (it == units.end())
            return;
        removeFromLayer(it->second);
        units.erase(it);
    }

    Spatial2DUnit *Spatial2DLayerManager::GetUnit(vke_ds::id32_t unitID)
    {
        auto it = units.find(unitID);
        return it == units.end() ? nullptr : &it->second;
    }

    const Spatial2DUnit *Spatial2DLayerManager::GetUnit(vke_ds::id32_t unitID) const
    {
        auto it = units.find(unitID);
        return it == units.end() ? nullptr : &it->second;
    }

    const Spatial2DLayer *Spatial2DLayerManager::GetLayer(vke_ds::id32_t layerID) const
    {
        auto it = layers.find(layerID);
        return it == layers.end() ? nullptr : &it->second;
    }

    std::vector<vke_ds::id32_t> Spatial2DLayerManager::Query(const glm::vec2 &point) const
    {
        std::vector<vke_ds::id32_t> result;
        for (auto it = layerOrder.rbegin(); it != layerOrder.rend(); ++it)
        {
            std::vector<vke_ds::id32_t> hits = layers.at(*it).Query(point);
            result.insert(result.end(), hits.begin(), hits.end());
        }
        return result;
    }

    std::vector<vke_ds::id32_t> Spatial2DLayerManager::Query(const AABB2D &bounds) const
    {
        std::vector<vke_ds::id32_t> result;
        for (auto it = layerOrder.rbegin(); it != layerOrder.rend(); ++it)
        {
            std::vector<vke_ds::id32_t> hits = layers.at(*it).Query(bounds);
            result.insert(result.end(), hits.begin(), hits.end());
        }
        return result;
    }

    vke_ds::id32_t Spatial2DLayerManager::insertUnit(Spatial2DUnit &unit)
    {
        size_t firstLayer = 0;
        size_t lastLayer = layerOrder.size();

        for (size_t i = 0; i < layerOrder.size(); ++i)
        {
            const Spatial2DLayer &layer = layers.at(layerOrder[i]);
            for (const auto &entry : layer.GetUnits())
            {
                const Spatial2DUnit &other = *entry.second;
                if (!other.bounds.Overlaps(unit.bounds))
                    continue;
                if (other.zIndex < unit.zIndex)
                    firstLayer = std::max(firstLayer, i + 1);
                else if (other.zIndex > unit.zIndex)
                    lastLayer = std::min(lastLayer, i);
            }
        }

        if (firstLayer > lastLayer)
            firstLayer = lastLayer;

        for (size_t i = firstLayer; i < layerOrder.size() && i <= lastLayer; ++i)
        {
            Spatial2DLayer &layer = layers.at(layerOrder[i]);
            if (layer.CanInsert(unit))
            {
                layer.Insert(unit);
                return layer.id;
            }
        }

        const vke_ds::id32_t layerID = layerAllocator.Alloc();
        auto [it, inserted] = layers.emplace(layerID, Spatial2DLayer(layerID));
        it->second.Insert(unit);
        layerOrder.insert(layerOrder.begin() + static_cast<std::ptrdiff_t>(firstLayer), layerID);
        return layerID;
    }

    void Spatial2DLayerManager::removeFromLayer(Spatial2DUnit &unit)
    {
        if (unit.layer == Spatial2DUnit::INVALID_LAYER)
            return;

        auto layerIt = layers.find(unit.layer);
        if (layerIt != layers.end())
        {
            layerIt->second.Remove(unit.id);
            if (layerIt->second.Empty())
            {
                layerOrder.erase(std::remove(layerOrder.begin(), layerOrder.end(), unit.layer), layerOrder.end());
                layers.erase(layerIt);
            }
        }
        unit.layer = Spatial2DUnit::INVALID_LAYER;
    }
}
