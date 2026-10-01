#pragma once

#include <common.hpp>
#include <memory>

namespace vke_common
{
    // Serialized identity and optional resolved ownership. Access never performs I/O.
    template <typename T>
    class AssetRef
    {
    public:
        AssetRef() = default;
        explicit AssetRef(AssetHandle handle) : handle(handle) {}

        AssetHandle Handle() const { return handle; }
        const std::shared_ptr<T> &Get() const { return resource; }
        bool IsResolved() const { return handle == 0 || resource != nullptr; }

        void SetHandle(AssetHandle value)
        {
            handle = value;
            resource.reset();
        }

        void Resolve(std::shared_ptr<T> value) { resource = std::move(value); }

        void SetResource(std::shared_ptr<T> value)
        {
            handle = value ? value->handle : 0;
            resource = std::move(value);
        }

    private:
        AssetHandle handle = 0;
        std::shared_ptr<T> resource;
    };
}
