#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <asset/asset_db_json.hpp>
#include <filesystem>

namespace vke_common
{
#define AM_GET_FUNC(tp) \
    static tp *Get##tp(AssetHandle id);
#define AM_ITERATE_FUNC(tp) \
    static void Iterate##tp(std::function<void(const tp &)> op);

#define AM_OP_FUNCS(tp) \
    AM_GET_FUNC(tp)     \
    AM_ITERATE_FUNC(tp)

    class AssetManager
    {
    private:
        static AssetManager *instance;
        AssetManager(const std::filesystem::path &prefix)
            : ftLibrary(nullptr), pathPrefix(prefix), builtinAssets(std::make_unique<AssetDBJSON>(BuiltinAssetLUTPath)) {};
        ~AssetManager()
        {
            assetDB.reset();
            builtinAssets.reset();
            if (ftLibrary != nullptr)
            {
                FT_Done_FreeType(ftLibrary);
                ftLibrary = nullptr;
            }
        }
        AssetManager(const AssetManager &);
        AssetManager &operator=(const AssetManager);

    public:
        FT_Library ftLibrary;

        static AssetManager *GetInstance()
        {
            VKE_FATAL_IF(instance == nullptr, "AssetManager not initialized!")
            return instance;
        }

        static AssetManager *Init(std::unique_ptr<AssetDBBase> &&db, const std::filesystem::path &prefix);

        static void Dispose()
        {
            delete instance;
            instance = nullptr;
        }

        static bool BulkLoad(const std::filesystem::path &pth, bool builtIn = false)
        {
            if (builtIn)
                return instance->builtinAssets->BulkLoad(pth);
            return instance->assetDB->BulkLoad(pth);
        }

        static AssetDBBase *GetAssetDB() { return instance->assetDB.get(); }
        static AssetDBBase *GetBuiltinAssets() { return instance->builtinAssets.get(); }

        AM_OP_FUNCS(TextureAsset)
        AM_OP_FUNCS(MeshAsset)
        AM_OP_FUNCS(VFShaderAsset)
        AM_OP_FUNCS(ComputeShaderAsset)
        AM_OP_FUNCS(MaterialAsset)
        AM_OP_FUNCS(SkeletonAsset)
        AM_OP_FUNCS(AnimationAsset)
        AM_OP_FUNCS(SceneAsset)
        AM_OP_FUNCS(FontAsset)
        AM_OP_FUNCS(AudioClipAsset)

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

    private:
        std::filesystem::path pathPrefix;
        std::unique_ptr<AssetDBJSON> builtinAssets;
        std::unique_ptr<AssetDBBase> assetDB;

        void initBuiltinAssets();
    };
}

#undef AM_GET_FUNC
#undef AM_ITERATE_FUNC
#undef AM_OP_FUNCS

#endif
