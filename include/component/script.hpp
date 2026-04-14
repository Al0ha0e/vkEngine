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
    class ScriptState
    {
    public:
        std::string className;
        nlohmann::json serializedData;

        ScriptState() : className(), serializedData() {}

        ScriptState(std::string className)
            : className(std::move(className)), serializedData() {}

        ScriptState(const ScriptState &) = default;
        ScriptState &operator=(const ScriptState &) = default;

        ScriptState(ScriptState &&ano) noexcept
            : className(std::move(ano.className)),
              serializedData(std::move(ano.serializedData)) {}

        ScriptState &operator=(ScriptState &&ano) noexcept
        {
            if (this == &ano)
                return *this;
            className = std::move(ano.className);
            serializedData = std::move(ano.serializedData);
            return *this;
        }

        ScriptState(const nlohmann::json &json)
            : className(json["className"].get<std::string>()),
              serializedData(json["data"]) {}

        nlohmann::json ToJSON() const
        {
            return {{"type", "script"},
                    {"className", className},
                    {"data", serializedData}};
        }

        std::string ToCSharp(entt::entity entity) const
        {
            nlohmann::json json = {
                {"entity", (uint32_t)entity},
                {"className", className},
                {"data", serializedData}};
            return json.dump();
        }

        bool operator==(const ScriptState &ano) const
        {
            return className == ano.className;
        }

        bool operator!=(const ScriptState &ano) const
        {
            return !(*this == ano);
        }
    };
}

#endif