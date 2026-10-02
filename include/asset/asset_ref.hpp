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
        const std::shared_ptr<T> &Get() const { return asset; }
        bool IsResolved() const { return handle == 0 || asset != nullptr; }

        void SetHandle(AssetHandle value)
        {
            handle = value;
            asset.reset();
        }

        void Resolve(std::shared_ptr<T> value) { asset = std::move(value); }

        void SetAsset(std::shared_ptr<T> value)
        {
            handle = value ? value->handle : 0;
            asset = std::move(value);
        }

    private:
        AssetHandle handle = 0;
        std::shared_ptr<T> asset;
    };
}
