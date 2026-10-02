#ifndef SCRIPT_COMPONENT_H
#define SCRIPT_COMPONENT_H

#include <json_validation.hpp>
#include <reflect/type_info.hpp>
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

        vke_common::SceneResult<vke_common::TypeInfoDataPtr> PrepareForInstantiation() const;

        // Call before constructing from JSON; asset checks happen in LoadAssets where applicable.
        static vke_common::SceneResult<void> ValidateJSON(const nlohmann::json &json)
        {
            using namespace vke_common::json_validation;
            auto result = Object(json).Require({"className", "data"}).Strings({"className"}).Result();
            if (!result) return result;
            if (json["className"].get<std::string>().empty() || !json["data"].is_object())
                return std::unexpected("script requires className and object data");
            return {};
        }

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
