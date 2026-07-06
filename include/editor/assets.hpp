#ifndef EDITOR_ASSETS_H
#define EDITOR_ASSETS_H

#include <asset.hpp>
#include <map>
#include <string>
#include <vector>

namespace vke_editor
{
    enum class AssetBrowserMode
    {
        ByType,
        ByDirectory
    };

    struct AssetTreeEntry
    {
        vke_common::AssetType type;
        vke_common::AssetHandle id;
        std::string name;
        std::string path;
    };

    struct AssetTreeNode
    {
        std::string name;
        std::map<std::string, AssetTreeNode> children;
        std::vector<AssetTreeEntry> assets;
    };
}

#endif
