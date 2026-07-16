#include <asset/asset_db_json.hpp>

namespace vke_common
{
    void AssetDBJSON::ClearAll()
    {
        textureCache.clear();
        meshCache.clear();
        vfShaderCache.clear();
        computeShaderCache.clear();
        materialCache.clear();
        skeletonCache.clear();
        animationCache.clear();
        sceneCache.clear();
        fontCache.clear();
        audioCache.clear();
        for (auto &id : ids)
            id = stID;
    }

#define LOAD_LUT_CASE(tpid, tp, assets)          \
    case tpid:                                   \
        assets[id] = tp(id, asset);              \
        ids[type] = std::max(ids[type], id + 1); \
        break;

    void AssetDBJSON::bulkLoad(const std::filesystem::path &pth)
    {
        const nlohmann::json &json = LoadJSON(pth.string());
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
                LOAD_LUT_CASE(ASSET_SKELETON, SkeletonAsset, skeletonCache)
                LOAD_LUT_CASE(ASSET_ANIMATION, AnimationAsset, animationCache)
                LOAD_LUT_CASE(ASSET_SCENE, SceneAsset, sceneCache)
                LOAD_LUT_CASE(ASSET_FONT, FontAsset, fontCache)
                LOAD_LUT_CASE(ASSET_AUDIO_CLIP, AudioClipAsset, audioCache)
            default:
                break;
            }
        }
    }

#define ASSET_TO_JSON(cache)                \
    for (auto &kv : cache)                  \
        if (kv.first >= CUSTOM_ASSET_ID_ST) \
            ret += "\n" + kv.second.ToJSON() + ",";

    void AssetDBJSON::saveAll(const std::filesystem::path &pth)
    {
        std::string ret = "[ ";

        ASSET_TO_JSON(textureCache)
        ASSET_TO_JSON(meshCache)
        ASSET_TO_JSON(vfShaderCache)
        ASSET_TO_JSON(computeShaderCache)
        ASSET_TO_JSON(materialCache)
        ASSET_TO_JSON(skeletonCache)
        ASSET_TO_JSON(animationCache)
        ASSET_TO_JSON(fontCache)
        ASSET_TO_JSON(audioCache)
        for (auto &kv : sceneCache)
            ret += "\n" + kv.second.ToJSON() + ",";

        ret[ret.length() - 1] = ' ';
        ret += "]";

        std::ofstream ofs(pth);
        ofs << ret;
        ofs.close();
    }
}