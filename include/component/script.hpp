// #ifndef SCRIPT_COMPONENT_H
// #define SCRIPT_COMPONENT_H

// #include <functional>
// #include <string>
// #include <unordered_map>
// #include <utility>
// #include <nlohmann/json.hpp>
// #include <ds/id_allocator.hpp>

// namespace vke_component
// {
//     class ScriptState
//     {
//     public:
//         vke_ds::id32_t id;
//         std::string className;
//         std::string serializedData;

//         ScriptState() : id(0), className(), serializedData() {}

//         ScriptState(vke_ds::id32_t id, std::string className, std::string serializedData)
//             : id(id), className(std::move(className)), serializedData(std::move(serializedData)) {}

//         ScriptState(const ScriptState &) = default;
//         ScriptState &operator=(const ScriptState &) = default;

//         ScriptState(ScriptState &&ano) noexcept
//             : id(ano.id),
//               className(std::move(ano.className)),
//               serializedData(std::move(ano.serializedData))
//         {
//             ano.id = 0;
//         }

//         ScriptState &operator=(ScriptState &&ano) noexcept
//         {
//             if (this == &ano)
//                 return *this;

//             id = ano.id;
//             className = std::move(ano.className);
//             serializedData = std::move(ano.serializedData);
//             ano.id = 0;
//             return *this;
//         }

//         ScriptState(vke_ds::id32_t id, const nlohmann::json &json)
//             : id(id),
//               className(json["className"].get<std::string>()),
//               serializedData(json["data"].dump()) {}

//         nlohmann::json ToJSON() const
//         {
//             nlohmann::json data = nlohmann::json::object();
//             if (!serializedData.empty())
//                 data = nlohmann::json::parse(serializedData);

//             return {
//                 {"type", "script"},
//                 {"id", id},
//                 {"className", className},
//                 {"data", std::move(data)}};
//         }

//         bool operator==(const ScriptState &ano) const
//         {
//             return id == ano.id && className == ano.className;
//         }

//         bool operator!=(const ScriptState &ano) const
//         {
//             return !(*this == ano);
//         }
//     };
// }

// #endif
