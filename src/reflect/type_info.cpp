#include <reflect/type_info.hpp>

#include <algorithm>
#include <unordered_set>

namespace vke_common
{
    const TypeInfoPtr &TypeInfo::Byte() noexcept
    {
        static const TypeInfoPtr type(new TypeInfo("byte", 1, ByteTypeInfo{}));
        return type;
    }

    const TypeInfoPtr &TypeInfo::Int32() noexcept
    {
        static const TypeInfoPtr type(new TypeInfo("int32", 4, Int32TypeInfo{}));
        return type;
    }

    const TypeInfoPtr &TypeInfo::Int64() noexcept
    {
        static const TypeInfoPtr type(new TypeInfo("int64", 8, Int64TypeInfo{}));
        return type;
    }

    const TypeInfoPtr &TypeInfo::Float32() noexcept
    {
        static const TypeInfoPtr type(new TypeInfo("float32", 4, Float32TypeInfo{}));
        return type;
    }

    const TypeInfoPtr &TypeInfo::Float64() noexcept
    {
        static const TypeInfoPtr type(new TypeInfo("float64", 8, Float64TypeInfo{}));
        return type;
    }

    const TypeInfoPtr &TypeInfo::String() noexcept
    {
        static const TypeInfoPtr type(new TypeInfo("string", 4, StringTypeInfo{}));
        return type;
    }

    TypeInfoResult TypeInfo::CreateVector(TypeInfoPtr scalarType, uint32_t componentCount,
                                          std::string name)
    {
        if (!scalarType)
            return std::unexpected(TypeInfoError::NullReferencedType);
        if (!isVectorScalar(scalarType->Kind()))
            return std::unexpected(TypeInfoError::InvalidScalarType);
        if (componentCount < 2 || componentCount > 4)
            return std::unexpected(TypeInfoError::InvalidComponentCount);

        if (name.empty())
            name = "vec<" + scalarType->Name() + ", " + std::to_string(componentCount) + ">";
        if (!isValidName(name))
            return std::unexpected(TypeInfoError::InvalidTypeName);

        const uint32_t alignment = scalarType->Alignment();
        VectorTypeInfo vectorInfo{
            .scalarType = std::move(scalarType),
            .componentCount = componentCount,
        };
        return TypeInfoPtr(new TypeInfo(std::move(name), alignment, std::move(vectorInfo)));
    }

    TypeInfoResult TypeInfo::CreateArray(TypeInfoPtr elementType, std::string name)
    {
        if (!elementType)
            return std::unexpected(TypeInfoError::NullReferencedType);

        if (name.empty())
            name = "array<" + elementType->Name() + ">";
        if (!isValidName(name))
            return std::unexpected(TypeInfoError::InvalidTypeName);

        const uint32_t alignment = std::max(4u, elementType->Alignment());
        ArrayTypeInfo arrayInfo{
            .elementType = std::move(elementType),
        };
        return TypeInfoPtr(new TypeInfo(std::move(name), alignment, std::move(arrayInfo)));
    }

    TypeInfoResult TypeInfo::CreateStruct(std::string name, std::vector<FieldInfo> fields)
    {
        if (!isValidName(name))
            return std::unexpected(TypeInfoError::InvalidTypeName);

        std::unordered_set<std::string> fieldNames;
        fieldNames.reserve(fields.size());

        uint32_t alignment = 1;
        for (const auto &field : fields)
        {
            if (!isValidName(field.name))
                return std::unexpected(TypeInfoError::InvalidFieldName);
            if (!field.type)
                return std::unexpected(TypeInfoError::NullReferencedType);
            if (!fieldNames.emplace(field.name).second)
                return std::unexpected(TypeInfoError::DuplicateFieldName);

            alignment = std::max(alignment, field.type->Alignment());
        }

        StructTypeInfo structInfo{.fields = std::move(fields)};
        return TypeInfoPtr(new TypeInfo(std::move(name), alignment, std::move(structInfo)));
    }
}
