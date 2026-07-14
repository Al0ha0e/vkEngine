#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <asset/asset.hpp>

namespace vke_common
{
#define GET_ASSET_FUNC(tp, cache)           \
    static tp *Get##tp(AssetHandle id)      \
    {                                       \
        auto it = instance->cache.find(id); \
        if (it == instance->cache.end())    \
            return nullptr;                 \
        return &(it->second);               \
    }

#define SET_ASSET_FUNC(tp, cache)                  \
    static void Set##tp(AssetHandle id, tp &asset) \
    {                                              \
        instance->cache[id] = asset;               \
    }

#define CREATE_ASSET_FUNC(tp, cache)                                    \
    static AssetHandle Create##tp(std::string &name, std::string &path) \
    {                                                                   \
        AssetHandle id = AllocateAssetID(tp::type);                     \
        tp asset(id, name, path);                                       \
        Set##tp(id, asset);                                             \
        return id;                                                      \
    }

#define ASSET_OP_FUNCS(tp, cache) \
    GET_ASSET_FUNC(tp, cache)     \
    SET_ASSET_FUNC(tp, cache)     \
    CREATE_ASSET_FUNC(tp, cache)

    class AssetManager
    {
    private:
        static AssetManager *instance;
        AssetManager() : ftLibrary(nullptr) {};
        ~AssetManager()
        {
            audioCache.clear();
            fontCache.clear();
            if (ftLibrary != nullptr)
                FT_Done_FreeType(ftLibrary);
        }
        AssetManager(const AssetManager &);
        AssetManager &operator=(const AssetManager);

    public:
        AssetHandle ids[ASSET_CNT_FLAG];
        std::map<AssetHandle, TextureAsset> textureCache;
        std::map<AssetHandle, MeshAsset> meshCache;
        std::map<AssetHandle, VFShaderAsset> vfShaderCache;
        std::map<AssetHandle, ComputeShaderAsset> computeShaderCache;
        std::map<AssetHandle, MaterialAsset> materialCache;
        std::map<AssetHandle, SkeletonAsset> skeletonCache;
        std::map<AssetHandle, AnimationAsset> animationCache;
        std::map<AssetHandle, SceneAsset> sceneCache;
        std::map<AssetHandle, FontAsset> fontCache;
        std::map<AssetHandle, AudioClipAsset> audioCache;
        FT_Library ftLibrary;

        static AssetManager *GetInstance()
        {
            VKE_FATAL_IF(instance == nullptr, "AssetManager not initialized!")
            return instance;
        }

        static AssetManager *Init();

        static void Dispose()
        {
            delete instance;
            instance = nullptr;
        }

        static void ClearAssetLUT() { instance->clearAssetLUT(); }

        static void LoadAssetLUT(const std::string &pth) { instance->loadAssetLUT(pth); }

        static void SaveAssetLUT(const std::string &pth) { instance->saveAssetLUT(pth); }

        static void ReadFile(const std::string &filename, std::vector<char> &buffer);

        static nlohmann::json LoadJSON(const std::string &pth);

        static AssetHandle AllocateAssetID(AssetType type);

        static std::unique_ptr<vke_render::Texture2D> LoadTexture2DUnique(const AssetHandle hdl);
        static std::unique_ptr<vke_render::Mesh> LoadMeshUnique(const AssetHandle hdl);
        static std::unique_ptr<vke_render::ShaderModuleSet> LoadVertFragShaderUnique(const AssetHandle hdl);
        static std::unique_ptr<vke_render::ShaderModuleSet> LoadComputeShaderUnique(const AssetHandle hdl);
        static std::unique_ptr<vke_render::Material> LoadMaterialUnique(const AssetHandle hdl);
        static std::unique_ptr<vke_common::Skeleton> LoadSkeletonUnique(const AssetHandle hdl);
        static std::unique_ptr<vke_common::Animation> LoadAnimationUnique(const AssetHandle hdl);
        static std::unique_ptr<vke_common::Font> LoadFontUnique(const AssetHandle hdl);
        static std::unique_ptr<vke_audio::AudioClip> LoadAudioClipUnique(const AssetHandle hdl);

        static std::shared_ptr<vke_render::Texture2D> LoadTexture2D(const AssetHandle hdl);
        static std::shared_ptr<vke_render::Mesh> LoadMesh(const AssetHandle hdl);
        static std::shared_ptr<vke_render::ShaderModuleSet> LoadVertFragShader(const AssetHandle hdl);
        static std::shared_ptr<vke_render::ShaderModuleSet> LoadComputeShader(const AssetHandle hdl);
        static std::shared_ptr<vke_render::Material> LoadMaterial(const AssetHandle hdl);
        static std::shared_ptr<vke_common::Skeleton> LoadSkeleton(const AssetHandle hdl);
        static std::shared_ptr<vke_common::Animation> LoadAnimation(const AssetHandle hdl);
        static std::shared_ptr<vke_common::Font> LoadFont(const AssetHandle hdl);
        static std::shared_ptr<vke_audio::AudioClip> LoadAudioClip(const AssetHandle hdl);

        ASSET_OP_FUNCS(TextureAsset, textureCache)
        ASSET_OP_FUNCS(MeshAsset, meshCache)
        ASSET_OP_FUNCS(VFShaderAsset, vfShaderCache)
        ASSET_OP_FUNCS(ComputeShaderAsset, computeShaderCache)
        ASSET_OP_FUNCS(MaterialAsset, materialCache)
        ASSET_OP_FUNCS(SkeletonAsset, skeletonCache)
        ASSET_OP_FUNCS(AnimationAsset, animationCache)
        ASSET_OP_FUNCS(SceneAsset, sceneCache)
        ASSET_OP_FUNCS(FontAsset, fontCache)
        ASSET_OP_FUNCS(AudioClipAsset, audioCache)

    private:
        void clearAssetLUT();
        void loadAssetLUT(const std::string &pth);
        void saveAssetLUT(const std::string &pth);
        void loadBuiltinAssets();
    };
}

#endif