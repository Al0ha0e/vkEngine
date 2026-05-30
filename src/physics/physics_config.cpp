#include <physics/physics_config.hpp>
#include <common.hpp>
#include <algorithm>

namespace vke_physics
{
    static uint32_t ReadLayerID(const nlohmann::json &json)
    {
        if (json.is_number_unsigned() || json.is_number_integer())
            return json.get<uint32_t>();

        return json.value("id", 0u);
    }

    PhysicsConfig::PhysicsConfig()
    {
        resetDefault();
    }

    PhysicsConfig::PhysicsConfig(const nlohmann::json &json)
    {
        resetDefault();
        LoadJSON(json);
    }

    void PhysicsConfig::resetDefault()
    {
        maxBodies = 65536 * 4;
        numBodyMutexes = 0;
        maxBodyPairs = 65536 * 4;
        maxContactConstraints = 10240;
        collisionSteps = 1;
        tempAllocatorSize = 10 * 1024 * 1024;
        stepTime = 1.0f / 60.0f;
        gravity = JPH::Vec3(0, -9.8f, 0);

        objectLayerCount = 2;
        broadPhaseLayerCount = 2;
        objectLayerNames.fill("");
        broadPhaseLayerNames.fill("");

        for (uint32_t i = 0; i < MaxObjectLayers; ++i)
        {
            objectToBroadPhase[i] = JPH::BroadPhaseLayer(0);
            for (uint32_t j = 0; j < MaxObjectLayers; ++j)
                objectLayerPairs[i][j] = false;
            for (uint32_t j = 0; j < MaxBroadPhaseLayers; ++j)
                objectVsBroadPhaseLayerPairs[i][j] = false;
        }

        objectLayerNames[DefaultObjectLayers::NON_MOVING] = "NON_MOVING";
        objectLayerNames[DefaultObjectLayers::MOVING] = "MOVING";
        broadPhaseLayerNames[DefaultBroadPhaseLayers::NON_MOVING.GetValue()] = "NON_MOVING";
        broadPhaseLayerNames[DefaultBroadPhaseLayers::MOVING.GetValue()] = "MOVING";
        objectToBroadPhase[DefaultObjectLayers::NON_MOVING] = DefaultBroadPhaseLayers::NON_MOVING;
        objectToBroadPhase[DefaultObjectLayers::MOVING] = DefaultBroadPhaseLayers::MOVING;
        SetObjectLayerCollision(DefaultObjectLayers::NON_MOVING, DefaultObjectLayers::MOVING, true);
        SetObjectLayerCollision(DefaultObjectLayers::MOVING, DefaultObjectLayers::MOVING, true);
        rebuildObjectVsBroadPhaseLayerPairs();
    }

    JPH::ObjectLayer PhysicsConfig::AddObjectLayer(const std::string &name, JPH::BroadPhaseLayer broadPhaseLayer)
    {
        VKE_FATAL_IF(objectLayerCount >= MaxObjectLayers, "Jolt object layer count cannot exceed {}", MaxObjectLayers)
        VKE_FATAL_IF(broadPhaseLayer.GetValue() >= broadPhaseLayerCount, "Invalid broad phase layer {}", broadPhaseLayer.GetValue())
        const JPH::ObjectLayer layer = static_cast<JPH::ObjectLayer>(objectLayerCount++);
        objectLayerNames[layer] = name;
        objectToBroadPhase[layer] = broadPhaseLayer;
        rebuildObjectVsBroadPhaseLayerPairs();
        return layer;
    }

    JPH::BroadPhaseLayer PhysicsConfig::AddBroadPhaseLayer(const std::string &name)
    {
        VKE_FATAL_IF(broadPhaseLayerCount >= MaxBroadPhaseLayers, "Jolt broad phase layer count cannot exceed {}", MaxBroadPhaseLayers)
        const JPH::BroadPhaseLayer layer(static_cast<JPH::BroadPhaseLayer::Type>(broadPhaseLayerCount++));
        broadPhaseLayerNames[layer.GetValue()] = name;
        return layer;
    }

    void PhysicsConfig::SetObjectLayerBroadPhaseLayer(JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer)
    {
        VKE_FATAL_IF(objectLayer >= objectLayerCount, "Invalid object layer {}", objectLayer)
        VKE_FATAL_IF(broadPhaseLayer.GetValue() >= broadPhaseLayerCount, "Invalid broad phase layer {}", broadPhaseLayer.GetValue())
        objectToBroadPhase[objectLayer] = broadPhaseLayer;
        rebuildObjectVsBroadPhaseLayerPairs();
    }

    void PhysicsConfig::SetObjectLayerCollision(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2, bool shouldCollide)
    {
        VKE_FATAL_IF(layer1 >= objectLayerCount || layer2 >= objectLayerCount, "Invalid object layer pair {}, {}", layer1, layer2)
        objectLayerPairs[layer1][layer2] = shouldCollide;
        objectLayerPairs[layer2][layer1] = shouldCollide;
        rebuildObjectVsBroadPhaseLayerPairs();
    }

    void PhysicsConfig::SetObjectVsBroadPhaseLayerCollision(JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer, bool shouldCollide)
    {
        VKE_FATAL_IF(objectLayer >= objectLayerCount, "Invalid object layer {}", objectLayer)
        VKE_FATAL_IF(broadPhaseLayer.GetValue() >= broadPhaseLayerCount, "Invalid broad phase layer {}", broadPhaseLayer.GetValue())
        objectVsBroadPhaseLayerPairs[objectLayer][broadPhaseLayer.GetValue()] = shouldCollide;
    }

    void PhysicsConfig::rebuildObjectVsBroadPhaseLayerPairs()
    {
        for (uint32_t objectLayer = 0; objectLayer < MaxObjectLayers; ++objectLayer)
            for (uint32_t broadPhaseLayer = 0; broadPhaseLayer < MaxBroadPhaseLayers; ++broadPhaseLayer)
                objectVsBroadPhaseLayerPairs[objectLayer][broadPhaseLayer] = false;

        for (uint32_t objectLayer = 0; objectLayer < objectLayerCount; ++objectLayer)
        {
            for (uint32_t otherLayer = 0; otherLayer < objectLayerCount; ++otherLayer)
            {
                if (!objectLayerPairs[objectLayer][otherLayer])
                    continue;

                const uint32_t broadPhaseLayer = objectToBroadPhase[otherLayer].GetValue();
                if (broadPhaseLayer < broadPhaseLayerCount)
                    objectVsBroadPhaseLayerPairs[objectLayer][broadPhaseLayer] = true;
            }
        }
    }

    void PhysicsConfig::LoadJSON(const nlohmann::json &json)
    {
        resetDefault();

        if (json.is_null())
            return;

        maxBodies = json.value("maxBodies", maxBodies);
        numBodyMutexes = json.value("numBodyMutexes", numBodyMutexes);
        maxBodyPairs = json.value("maxBodyPairs", maxBodyPairs);
        maxContactConstraints = json.value("maxContactConstraints", maxContactConstraints);
        collisionSteps = json.value("collisionSteps", collisionSteps);
        tempAllocatorSize = json.value("tempAllocatorSize", tempAllocatorSize);
        stepTime = json.value("stepTime", stepTime);

        if (json.contains("gravity"))
        {
            const auto &gravityJson = json["gravity"];
            if (gravityJson.is_array())
            {
                VKE_FATAL_IF(gravityJson.size() < 3, "PhysicsConfig gravity array must contain 3 values")
                gravity = JPH::Vec3(gravityJson.at(0).get<float>(), gravityJson.at(1).get<float>(), gravityJson.at(2).get<float>());
            }
            else
            {
                gravity = JPH::Vec3(gravityJson.value("x", gravity.GetX()), gravityJson.value("y", gravity.GetY()), gravityJson.value("z", gravity.GetZ()));
            }
        }

        if (json.contains("objectLayers"))
        {
            objectLayerCount = 0;
            uint32_t nextID = 0;
            for (const auto &layerJson : json["objectLayers"])
            {
                const uint32_t id = layerJson.is_string() ? nextID : ReadLayerID(layerJson);
                VKE_FATAL_IF(id >= MaxObjectLayers, "Jolt object layer id {} exceeds max {}", id, MaxObjectLayers)
                objectLayerCount = std::max(objectLayerCount, id + 1);
                objectLayerNames[id] = layerJson.is_string() ? layerJson.get<std::string>() : layerJson.value("name", std::to_string(id));
                nextID = id + 1;
            }
        }

        if (json.contains("broadPhaseLayers"))
        {
            broadPhaseLayerCount = 0;
            uint32_t nextID = 0;
            for (const auto &layerJson : json["broadPhaseLayers"])
            {
                const uint32_t id = layerJson.is_string() ? nextID : ReadLayerID(layerJson);
                VKE_FATAL_IF(id >= MaxBroadPhaseLayers, "Jolt broad phase layer id {} exceeds max {}", id, MaxBroadPhaseLayers)
                broadPhaseLayerCount = std::max(broadPhaseLayerCount, id + 1);
                broadPhaseLayerNames[id] = layerJson.is_string() ? layerJson.get<std::string>() : layerJson.value("name", std::to_string(id));
                nextID = id + 1;
            }
        }

        for (uint32_t i = 0; i < MaxObjectLayers; ++i)
            objectToBroadPhase[i] = JPH::BroadPhaseLayer(0);

        if (json.contains("objectToBroadPhase"))
        {
            for (const auto &mapping : json["objectToBroadPhase"])
            {
                uint32_t objectLayer = 0;
                uint32_t broadPhaseLayer = 0;
                if (mapping.is_array())
                {
                    objectLayer = mapping.at(0).get<uint32_t>();
                    broadPhaseLayer = mapping.at(1).get<uint32_t>();
                }
                else
                {
                    objectLayer = mapping.value("objectLayer", mapping.value("object", 0u));
                    broadPhaseLayer = mapping.value("broadPhaseLayer", mapping.value("broadPhase", 0u));
                }
                VKE_FATAL_IF(objectLayer >= objectLayerCount || broadPhaseLayer >= broadPhaseLayerCount, "Invalid object to broad phase mapping {} -> {}", objectLayer, broadPhaseLayer)
                objectToBroadPhase[objectLayer] = JPH::BroadPhaseLayer(static_cast<JPH::BroadPhaseLayer::Type>(broadPhaseLayer));
            }
        }

        for (uint32_t i = 0; i < MaxObjectLayers; ++i)
            for (uint32_t j = 0; j < MaxObjectLayers; ++j)
                objectLayerPairs[i][j] = false;

        if (json.contains("objectLayerPairs"))
        {
            for (const auto &pairJson : json["objectLayerPairs"])
            {
                uint32_t layer1 = 0;
                uint32_t layer2 = 0;
                bool shouldCollide = true;
                if (pairJson.is_array())
                {
                    layer1 = pairJson.at(0).get<uint32_t>();
                    layer2 = pairJson.at(1).get<uint32_t>();
                    shouldCollide = pairJson.size() < 3 || pairJson.at(2).get<bool>();
                }
                else
                {
                    layer1 = pairJson.value("layer1", pairJson.value("a", 0u));
                    layer2 = pairJson.value("layer2", pairJson.value("b", 0u));
                    shouldCollide = pairJson.value("collide", true);
                }
                SetObjectLayerCollision(static_cast<JPH::ObjectLayer>(layer1), static_cast<JPH::ObjectLayer>(layer2), shouldCollide);
            }
        }

        rebuildObjectVsBroadPhaseLayerPairs();

        if (json.contains("objectVsBroadPhaseLayerPairs"))
        {
            for (const auto &pairJson : json["objectVsBroadPhaseLayerPairs"])
            {
                uint32_t objectLayer = 0;
                uint32_t broadPhaseLayer = 0;
                bool shouldCollide = true;
                if (pairJson.is_array())
                {
                    objectLayer = pairJson.at(0).get<uint32_t>();
                    broadPhaseLayer = pairJson.at(1).get<uint32_t>();
                    shouldCollide = pairJson.size() < 3 || pairJson.at(2).get<bool>();
                }
                else
                {
                    objectLayer = pairJson.value("objectLayer", pairJson.value("object", 0u));
                    broadPhaseLayer = pairJson.value("broadPhaseLayer", pairJson.value("broadPhase", 0u));
                    shouldCollide = pairJson.value("collide", true);
                }
                SetObjectVsBroadPhaseLayerCollision(static_cast<JPH::ObjectLayer>(objectLayer), JPH::BroadPhaseLayer(static_cast<JPH::BroadPhaseLayer::Type>(broadPhaseLayer)), shouldCollide);
            }
        }
    }
}
