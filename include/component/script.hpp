#ifndef SCRIPT_COMPONENT_H
#define SCRIPT_COMPONENT_H

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <nlohmann/json.hpp>
#include <ds/id_allocator.hpp>

namespace vke_component
{
    struct ScriptStateData
    {
        std::string className;
        nlohmann::json serializedData;

        ScriptStateData() = default;
        ScriptStateData(const nlohmann::json &json)
            : className(json["className"].get<std::string>()), serializedData(json["data"]) {}
        nlohmann::json ToJSON() const
        {
            return {{"type", "script"}, {"className", className}, {"data", serializedData}};
        }

    };
}

#endif
