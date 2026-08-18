#ifndef VKE_REFLECT_REFLECT_H
#define VKE_REFLECT_REFLECT_H

#define REFLECT_STRUCT(name) struct name
#define REFLECT_CLASS(name) class name
#define REFLECT_FIELD(type, name, ...) type name

#define LOAD_JSON void loadJSON(const nlohmann::json &json)

#endif
