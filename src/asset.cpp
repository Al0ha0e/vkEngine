#include <asset.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>

namespace vke_common
{
    AssetManager *AssetManager::instance = nullptr;

    AssetManager *AssetManager::Init()
    {
        instance = new AssetManager();
        for (int i = 0; i < ASSET_CNT_FLAG; i++)
            instance->ids[i] = CUSTOM_ASSET_ID_ST;
        instance->ids[ASSET_SCENE] = 1;
        instance->loadBuiltinAssets();
        return instance;
    }

    AssetHandle AssetManager::AllocateAssetID(AssetType type)
    {
        return instance->ids[type]++;
    }

    void AssetManager::clearAssetLUT()
    {
        // TODO
    }

#define LOAD_LUT_CASE(tpid, tp, assets)          \
    case tpid:                                   \
        assets[id] = tp(id, asset);              \
        ids[type] = std::max(ids[type], id + 1); \
        break;

    void AssetManager::loadAssetLUT(const std::string &pth)
    {
        const nlohmann::json &json = LoadJSON(pth);
        for (const auto &asset : json)
        {
            AssetType type = asset["type"];
            AssetHandle id = asset["id"];
            switch (type)
            {
                LOAD_LUT_CASE(ASSET_TEXTURE, TextureAsset, textureCache)
                LOAD_LUT_CASE(ASSET_MESH, MeshAsset, meshCache)
                LOAD_LUT_CASE(ASSET_VF_SHADER, VFShaderAsset, vfShaderCache)
                LOAD_LUT_CASE(ASSET_COMPUTE_SHADER, ComputeShaderAsset, computeShaderCache)
                LOAD_LUT_CASE(ASSET_MATERIAL, MaterialAsset, materialCache)
                LOAD_LUT_CASE(ASSET_SCENE, SceneAsset, sceneCache)
            default:
                break;
            }
        }
    }

#define ASSET_TO_JSON(cache)                \
    for (auto &kv : cache)                  \
        if (kv.first >= CUSTOM_ASSET_ID_ST) \
            ret += "\n" + kv.second.ToJSON() + ",";

    void AssetManager::saveAssetLUT(const std::string &pth)
    {
        std::string ret = "[ ";

        ASSET_TO_JSON(textureCache)
        ASSET_TO_JSON(meshCache)
        ASSET_TO_JSON(vfShaderCache)
        ASSET_TO_JSON(computeShaderCache)
        ASSET_TO_JSON(materialCache)
        for (auto &kv : sceneCache)
            ret += "\n" + kv.second.ToJSON() + ",";

        ret[ret.length() - 1] = ' ';
        ret += "]";

        std::ofstream ofs(pth);
        ofs << ret;
        ofs.close();
    }

    extern const std::vector<vke_render::Vertex> planeVertices;
    extern const std::vector<uint32_t> planeIndices;

#define LOAD_BUILTIN_ASSET(cache, load)                         \
    for (auto &kv : cache)                                      \
    {                                                           \
        kv.second.path = std::string(REL_DIR) + kv.second.path; \
        load(kv.first);                                         \
    }

    void AssetManager::loadBuiltinAssets()
    {
        loadAssetLUT(BuiltinAssetLUTPath);

        LOAD_BUILTIN_ASSET(textureCache, LoadTexture2D)
        LOAD_BUILTIN_ASSET(meshCache, LoadMesh)
        for (auto &kv : vfShaderCache)
        {
            kv.second.path = std::string(REL_DIR) + kv.second.path;
            kv.second.fragPath = std::string(REL_DIR) + kv.second.fragPath;
            LoadVertFragShader(kv.first);
        }
        LOAD_BUILTIN_ASSET(materialCache, LoadMaterial)

        MeshAsset plane(BUILTIN_MESH_PLANE_ID, "Plane", "");
        plane.val = std::make_shared<vke_render::Mesh>(BUILTIN_MESH_PLANE_ID, planeVertices, planeIndices);
        meshCache[BUILTIN_MESH_PLANE_ID] = plane;
    }
};