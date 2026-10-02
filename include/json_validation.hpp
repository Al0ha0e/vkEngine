#pragma once

#include <common.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>

namespace vke_common::json_validation
{
    using JSON = nlohmann::json;

    inline bool Unsigned(const JSON &value, uint64_t maximum = UINT64_MAX)
    {
        return value.is_number_integer() &&
               (value.is_number_unsigned() || value.get<int64_t>() >= 0) && value.get<uint64_t>() <= maximum;
    }

    inline bool Number(const JSON &value)
    {
        return value.is_number() && std::isfinite(value.get<double>()) &&
               std::abs(value.get<double>()) <= std::numeric_limits<float>::max();
    }

    inline bool Vector(const JSON &value, size_t size)
    {
        return value.is_array() && value.size() == size && std::all_of(value.begin(), value.end(), Number);
    }

    // Primitive field checks only. Each Data type owns its schema and semantics.
    class Object
    {
    public:
        explicit Object(const JSON &value) : value(value)
        {
            if (!value.is_object()) result = std::unexpected("expected an object");
        }

        Object &Require(std::initializer_list<const char *> fields)
        {
            for (auto field : fields)
                if (result && !value.contains(field)) result = std::unexpected(std::string("missing ") + field);
            return *this;
        }

        Object &Numbers(std::initializer_list<const char *> fields) { return Check(fields, Number); }
        Object &Booleans(std::initializer_list<const char *> fields)
        {
            return Check(fields, [](const JSON &v) { return v.is_boolean(); });
        }
        Object &Strings(std::initializer_list<const char *> fields)
        {
            return Check(fields, [](const JSON &v) { return v.is_string(); });
        }
        Object &Unsigneds(std::initializer_list<const char *> fields, uint64_t maximum = UINT64_MAX)
        {
            return Check(fields, [maximum](const JSON &v) { return Unsigned(v, maximum); });
        }
        Object &Vectors(std::initializer_list<const char *> fields, size_t size)
        {
            return Check(fields, [size](const JSON &v) { return Vector(v, size); });
        }
        SceneResult<void> Result() const { return result; }

    private:
        const JSON &value;
        SceneResult<void> result;

        template <typename Predicate>
        Object &Check(std::initializer_list<const char *> fields, Predicate predicate)
        {
            for (auto field : fields)
                if (result && value.contains(field) && !predicate(value[field]))
                    result = std::unexpected(std::string("invalid ") + field);
            return *this;
        }
    };
}
