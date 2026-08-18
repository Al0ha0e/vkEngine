#include <reflect/type_info.hpp>
#include <reflect/value_view.hpp>

#include <cmath>
#include <fstream>
#include <optional>

using namespace vke_common;

namespace
{
    int TestJsonRoundTrip()
    {
        auto positionType = TypeInfo::CreateVector(TypeInfo::Float32(), 3, "position");
        auto valuesType = TypeInfo::CreateArray(TypeInfo::Int32(), "values");
        if (!positionType || !valuesType)
            return 1;

        auto rootType = TypeInfo::CreateStruct(
            "example",
            {
                FieldInfo{"id", TypeInfo::Int64()},
                FieldInfo{"name", TypeInfo::String()},
                FieldInfo{"position", *positionType},
                FieldInfo{"values", *valuesType},
            });
        if (!rootType)
            return 2;

        const nlohmann::json json = {
            {"id", 1234567890123},
            {"name", "abc"},
            {"position", {1.5, -2.0, 3.25}},
            {"values", {7, 8, 9}},
        };

        auto encoded = (*rootType)->EncodeBinaryFromJson(json);
        if (!encoded || (*encoded)->size() != 48)
            return 3;

        std::optional<ValueView> savedName;
        {
            auto root = ValueView::Parse(*rootType, std::move(*encoded));
            if (!root || root->Size() != 48)
                return 4;

            auto id = root->Field("id");
            auto name = root->Field("name");
            auto position = root->Field("position");
            auto values = root->Field("values");
            if (!id || !name || !position || !values)
                return 5;

            auto idValue = id->AsInt64();
            auto xView = position->Component(0);
            auto zView = position->Component(2);
            auto count = values->Count();
            auto secondView = values->Element(1);
            if (!idValue || *idValue != 1234567890123 || !xView || !zView ||
                !count || *count != 3 || !secondView)
                return 6;

            auto x = xView->AsFloat32();
            auto z = zView->AsFloat32();
            auto second = secondView->AsInt32();
            if (!x || !z || !second || std::abs(*x - 1.5f) > 0.0001f ||
                std::abs(*z - 3.25f) > 0.0001f || *second != 8)
                return 7;

            savedName.emplace(*name);
        }

        auto name = savedName->AsString();
        if (!name || *name != "abc")
            return 8;

        nlohmann::json missingField = json;
        missingField.erase("name");
        auto missingResult = (*rootType)->EncodeBinaryFromJson(missingField);
        if (missingResult || missingResult.error().code != TypeInfoJsonErrorCode::MissingField ||
            missingResult.error().path != "$.name")
            return 9;

        nlohmann::json unexpectedField = json;
        unexpectedField["extra"] = true;
        auto unexpectedResult = (*rootType)->EncodeBinaryFromJson(unexpectedField);
        if (unexpectedResult ||
            unexpectedResult.error().code != TypeInfoJsonErrorCode::UnexpectedField ||
            unexpectedResult.error().path != "$.extra")
            return 10;

        return 0;
    }

    int TestTypeInfoFromJson()
    {
        const nlohmann::json json = {
            {"kind", "struct"},
            {"name", "Example.Script"},
            {"fields", {
                {
                    {"name", "position"},
                    {"type", {
                        {"kind", "vector"},
                        {"name", "Example.Vector3"},
                        {"scalarType", {{"kind", "float32"}}},
                        {"componentCount", 3},
                    }},
                },
                {
                    {"name", "values"},
                    {"type", {
                        {"kind", "array"},
                        {"elementType", {{"kind", "int32"}}},
                    }},
                },
            }},
        };

        auto type = TypeInfo::FromJson(json);
        if (!type || (*type)->Name() != "Example.Script" ||
            (*type)->Kind() != TypeKind::Struct || (*type)->Alignment() != 4)
            return 1;

        const auto *structure = (*type)->GetIf<StructTypeInfo>();
        if (!structure || structure->fields.size() != 2 ||
            structure->fields[0].name != "position" ||
            structure->fields[1].name != "values")
            return 2;

        const auto *vector = structure->fields[0].type->GetIf<VectorTypeInfo>();
        const auto *array = structure->fields[1].type->GetIf<ArrayTypeInfo>();
        if (!vector || vector->componentCount != 3 ||
            vector->scalarType != TypeInfo::Float32() ||
            vector->scalarType->Name() != "float32" ||
            !array || array->elementType != TypeInfo::Int32() ||
            structure->fields[1].type->Name() != "array<int32>")
            return 3;

        nlohmann::json missingKind = {{"name", "broken"}};
        auto missing = TypeInfo::FromJson(missingKind);
        if (missing || missing.error().code != TypeInfoJsonErrorCode::MissingField ||
            missing.error().path != "$.kind")
            return 4;

        nlohmann::json badNestedKind = json;
        badNestedKind["fields"][1]["type"]["elementType"]["kind"] = "uint32";
        auto invalid = TypeInfo::FromJson(badNestedKind);
        if (invalid || invalid.error().code != TypeInfoJsonErrorCode::InvalidType ||
            invalid.error().path != "$.fields[1].type.elementType.kind")
            return 5;

        nlohmann::json extraField = {{"kind", "float32"}, {"name", "float"}};
        auto unexpected = TypeInfo::FromJson(extraField);
        if (unexpected ||
            unexpected.error().code != TypeInfoJsonErrorCode::UnexpectedField ||
            unexpected.error().path != "$.name")
            return 6;

        nlohmann::json duplicateFields = {
            {"kind", "struct"},
            {"name", "duplicate"},
            {"fields", {
                nlohmann::json::object({
                    {"name", "value"},
                    {"type", nlohmann::json::object({{"kind", "byte"}})},
                }),
                nlohmann::json::object({
                    {"name", "value"},
                    {"type", nlohmann::json::object({{"kind", "byte"}})},
                }),
            }},
        };
        auto duplicate = TypeInfo::FromJson(duplicateFields);
        if (duplicate || duplicate.error().code != TypeInfoJsonErrorCode::InvalidType ||
            duplicate.error().path != "$")
            return 7;

        return 0;
    }

    int TestExportedTypeInfoFile(const char *path)
    {
        std::ifstream input(path);
        if (!input)
            return 1;
        nlohmann::json document = nlohmann::json::parse(input, nullptr, false);
        if (document.is_discarded() || !document.is_object())
            return 2;
        const auto types = document.find("types");
        if (types == document.end() || !types->is_array() || types->empty())
            return 3;
        for (const auto &json : *types)
        {
            auto type = TypeInfo::FromJson(json);
            if (!type || (*type)->Kind() != TypeKind::Struct)
                return 4;
        }
        return 0;
    }
}

int main(int argc, char **argv)
{
    if (int result = TestJsonRoundTrip())
        return result;
    if (int result = TestTypeInfoFromJson())
        return 100 + result;
    if (argc == 2)
    {
        if (int result = TestExportedTypeInfoFile(argv[1]))
            return 200 + result;
    }
    return 0;
}
