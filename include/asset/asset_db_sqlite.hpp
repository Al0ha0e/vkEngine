#ifndef ASSET_DB_SQLITE_H
#define ASSET_DB_SQLITE_H

#include <asset/asset_db.hpp>
#include <set>

struct sqlite3;
struct sqlite3_stmt;

namespace vke_common
{

#define DBSQLITE_GET_ASSET_FUNC(tp, cache, tbl, deleted) \
    virtual tp *Get##tp(AssetHandle id) override         \
    {                                                    \
        auto it = cache.find(id);                        \
        if (it != cache.end())                           \
            return &it->second;                          \
        if (deleted.contains(id))                        \
            return nullptr;                              \
        tp asset;                                        \
        if (loadAssetFromDB(tbl, id, asset))             \
        {                                                \
            cache[id] = asset;                           \
            return &cache[id];                           \
        }                                                \
        return nullptr;                                  \
    }

#define DBSQLITE_SET_ASSET_FUNC(tp, cache, dirty, deleted) \
    virtual bool Set##tp(tp &asset) override               \
    {                                                      \
        cache[asset.id] = asset;                           \
        dirty.insert(asset.id);                            \
        deleted.erase(asset.id);                           \
        if (asset.id >= ids[tp::type])                     \
            ids[tp::type] = asset.id + 1;                  \
        return true;                                       \
    }

#define DBSQLITE_SYNC_ASSET_FUNC(tp, cache, dirty, deleted, tbl) \
    virtual bool Sync##tp(AssetHandle id) override               \
    {                                                            \
        if (deleted.contains(id))                                \
        {                                                        \
            if (!removeFromDB(tbl, id))                          \
                return false;                                    \
            deleted.erase(id);                                   \
            dirty.erase(id);                                     \
            return true;                                         \
        }                                                        \
        auto it = cache.find(id);                                \
        if (it == cache.end() ||                                 \
            !saveToDB(tbl, id, it->second.ToJSON().dump()))      \
            return false;                                        \
        dirty.erase(id);                                         \
        return true;                                             \
    }

#define DBSQLITE_CREATE_ASSET_FUNC(tp, cache, dirty, deleted) \
    virtual AssetHandle Create##tp(tp &asset) override        \
    {                                                         \
        AssetHandle newId = ids[tp::type]++;                  \
        asset.id = newId;                                     \
        cache[newId] = asset;                                 \
        dirty.insert(newId);                                  \
        deleted.erase(newId);                                 \
        return newId;                                         \
    }

#define DBSQLITE_REMOVE_ASSET_FUNC(tp, cache, dirty, deleted) \
    virtual bool Remove##tp(AssetHandle id) override          \
    {                                                         \
        cache.erase(id);                                      \
        dirty.erase(id);                                      \
        deleted.insert(id);                                   \
        return true;                                          \
    }

#define DBSQLITE_ITERATE_ASSET_FUNC(tp, cache, dirty, deleted, tbl)       \
    virtual void Iterate##tp(std::function<void(const tp &)> op) override \
    {                                                                     \
        if (!loadCleanAssetsFromDB(tbl, cache, dirty, deleted))           \
            return;                                                       \
        for (auto &kv : cache)                                            \
            op(kv.second);                                                \
    }

#define DBSQLITE_MARKDIRTY_ASSET_FUNC(tp, cache, ds)    \
    virtual void MarkDirty##tp(AssetHandle id) override \
    {                                                   \
        if (!cache.contains(id))                        \
            return;                                     \
        ds.insert(id);                                  \
    }

#define DBSQLITE_ASSET_OP_FUNCS(tp, cache, dirty, deleted, tbl) \
    DBSQLITE_GET_ASSET_FUNC(tp, cache, tbl, deleted)            \
    DBSQLITE_SET_ASSET_FUNC(tp, cache, dirty, deleted)          \
    DBSQLITE_SYNC_ASSET_FUNC(tp, cache, dirty, deleted, tbl)    \
    DBSQLITE_CREATE_ASSET_FUNC(tp, cache, dirty, deleted)       \
    DBSQLITE_REMOVE_ASSET_FUNC(tp, cache, dirty, deleted)       \
    DBSQLITE_ITERATE_ASSET_FUNC(tp, cache, dirty, deleted, tbl) \
    DBSQLITE_MARKDIRTY_ASSET_FUNC(tp, cache, dirty)

    class AssetDBSQLite : public AssetDBBase
    {
    public:
        AssetDBSQLite(const std::filesystem::path &dbPath, AssetHandle stid = 1);
        AssetDBSQLite(const AssetDBSQLite &) = delete;
        AssetDBSQLite &operator=(const AssetDBSQLite &) = delete;
        virtual ~AssetDBSQLite();

        virtual void Init() override;
        virtual bool ClearAll() override;
        virtual bool BulkLoad(const std::filesystem::path &pth) override;
        virtual bool SyncAll() override;

        DBSQLITE_ASSET_OP_FUNCS(TextureAsset, textureCache, dirtyTexture, deletedTexture, "texture_assets")
        DBSQLITE_ASSET_OP_FUNCS(MeshAsset, meshCache, dirtyMesh, deletedMesh, "mesh_assets")
        DBSQLITE_ASSET_OP_FUNCS(VFShaderAsset, vfShaderCache, dirtyVFShader, deletedVFShader, "vf_shader_assets")
        DBSQLITE_ASSET_OP_FUNCS(ComputeShaderAsset, computeShaderCache, dirtyComputeShader, deletedComputeShader, "compute_shader_assets")
        DBSQLITE_ASSET_OP_FUNCS(MaterialAsset, materialCache, dirtyMaterial, deletedMaterial, "material_assets")
        DBSQLITE_ASSET_OP_FUNCS(SkeletonAsset, skeletonCache, dirtySkeleton, deletedSkeleton, "skeleton_assets")
        DBSQLITE_ASSET_OP_FUNCS(AnimationAsset, animationCache, dirtyAnimation, deletedAnimation, "animation_assets")
        DBSQLITE_ASSET_OP_FUNCS(SceneAsset, sceneCache, dirtyScene, deletedScene, "scene_assets")
        DBSQLITE_ASSET_OP_FUNCS(FontAsset, fontCache, dirtyFont, deletedFont, "font_assets")
        DBSQLITE_ASSET_OP_FUNCS(AudioClipAsset, audioCache, dirtyAudio, deletedAudio, "audio_clip_assets")

    private:
        sqlite3 *db = nullptr;
        AssetHandle ids[ASSET_CNT_FLAG] = {};

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

        std::set<AssetHandle> dirtyTexture;
        std::set<AssetHandle> dirtyMesh;
        std::set<AssetHandle> dirtyVFShader;
        std::set<AssetHandle> dirtyComputeShader;
        std::set<AssetHandle> dirtyMaterial;
        std::set<AssetHandle> dirtySkeleton;
        std::set<AssetHandle> dirtyAnimation;
        std::set<AssetHandle> dirtyScene;
        std::set<AssetHandle> dirtyFont;
        std::set<AssetHandle> dirtyAudio;

        std::set<AssetHandle> deletedTexture;
        std::set<AssetHandle> deletedMesh;
        std::set<AssetHandle> deletedVFShader;
        std::set<AssetHandle> deletedComputeShader;
        std::set<AssetHandle> deletedMaterial;
        std::set<AssetHandle> deletedSkeleton;
        std::set<AssetHandle> deletedAnimation;
        std::set<AssetHandle> deletedScene;
        std::set<AssetHandle> deletedFont;
        std::set<AssetHandle> deletedAudio;

        bool execSQL(const std::string &sql);
        bool createTables();
        bool updateNextIds();

        bool saveToDB(const std::string &table, AssetHandle id, const std::string &jsonData);
        bool removeFromDB(const std::string &table, AssetHandle id);

        template <typename T>
        bool loadAssetFromDB(const std::string &table, AssetHandle id, T &outAsset);

        template <typename T>
        bool loadCleanAssetsFromDB(const std::string &table, std::map<AssetHandle, T> &cache,
                                   const std::set<AssetHandle> &dirty,
                                   const std::set<AssetHandle> &deleted);

        template <typename T>
        bool flushChanges(const std::string &table, std::map<AssetHandle, T> &cache,
                          const std::set<AssetHandle> &dirty,
                          const std::set<AssetHandle> &deleted);
    };

} // namespace vke_common

#endif // ASSET_DB_SQLITE_H
