#ifndef ASSET_H
#define ASSET_H

#include <render/shader.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/texture.hpp>
#include <nlohmann/json.hpp>

#include <physics/physics.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>

namespace vke_common
{
    extern const std::string BuiltinAssetLUTPath;

    const AssetHandle CUSTOM_ASSET_ID_ST = 1024;

    const AssetHandle BUILTIN_TEXTURE_DEFAULT_ID = 1;
    const AssetHandle BUILTIN_TEXTURE_TLUT_ID = 2;
    const AssetHandle BUILTIN_TEXTURE_SLUT_ID = 3;

    const AssetHandle BUILTIN_MESH_PLANE_ID = 1;
    const AssetHandle BUILTIN_MESH_CUBE_ID = 2;
    const AssetHandle BUILTIN_MESH_SPHERE_ID = 3;
    const AssetHandle BUILTIN_MESH_CYLINDER_ID = 4;
    const AssetHandle BUILTIN_MESH_MONKEY_ID = 5;

    const AssetHandle BUILTIN_VFSHADER_DEFAULT_ID = 1;
    const AssetHandle BUILTIN_VFSHADER_SKYBOX_ID = 2;

    const AssetHandle BUILTIN_COMPUTE_SHADER_SKYLUT_ID = 1;

    const AssetHandle BUILTIN_MATERIAL_DEFAULT_ID = 1;
    const AssetHandle BUILTIN_MATERIAL_SKYBOX_ID = 2;

    enum AssetType
    {
        ASSET_TEXTURE,
        ASSET_MESH,
        ASSET_VF_SHADER,
        ASSET_COMPUTE_SHADER,
        ASSET_MATERIAL,
        ASSET_SCENE,
        ASSET_CNT_FLAG
    };

    const std::string AssetTypeToName[] = {"Texture", "Mesh", "VFShader", "ComputeShader", "Material", "PhysicalMaterial", "Scene"};

    template <AssetType TID, typename T, typename VT>
    class Asset
    {
    public:
        static const AssetType type = TID;
        AssetHandle id;
        std::string name;
        std::string path;
        std::shared_ptr<VT> val;

        Asset() : id(0), val(nullptr) {}

        Asset(AssetHandle id, nlohmann::json &json)
            : id(id), name(json["name"]), path(json["path"]), val(nullptr) {}
        Asset(AssetHandle id, const std::string &nm, const std::string &pth)
            : id(id), name(nm), path(pth), val(nullptr) {}

        Asset<TID, T, VT> &operator=(const Asset<TID, T, VT> &ano)
        {
            id = ano.id;
            name = ano.name;
            path = ano.path;
            val = ano.val;
            return *this;
        }

        std::string ToJSON()
        {
            std::string ret = "{\"type\": " + std::to_string(type) + ", ";
            ret += "\"id\": " + std::to_string(id) + ", ";
            ret += "\"name\": \"" + name + "\", ";
            ret += "\"path\": \"" + path + "\"";
            ret += static_cast<T *>(this)->toJSON() + " }\n";
            return ret;
        }

        std::string toJSON() { return ""; }
    };

#define DEFAULT_CONSTRUCTOR(type) \
    type(AssetHandle id, nlohmann::json &json) : Asset(id, json) {}

#define DEFAULT_CONSTRUCTOR2(type) \
    type(AssetHandle id, const std::string &nm, const std::string &pth) : Asset(id, nm, pth) {}

#define LEAF_ASSET_TYPE(type, typeid, valtype)       \
    class type : public Asset<typeid, type, valtype> \
    {                                                \
    public:                                          \
        type() {}                                    \
        DEFAULT_CONSTRUCTOR(type)                    \
        DEFAULT_CONSTRUCTOR2(type)                   \
    };

    LEAF_ASSET_TYPE(MeshAsset, ASSET_MESH, vke_render::Mesh)
    LEAF_ASSET_TYPE(ComputeShaderAsset, ASSET_COMPUTE_SHADER, vke_render::ShaderModuleSet)
    LEAF_ASSET_TYPE(SceneAsset, ASSET_SCENE, int);

    class TextureAsset : public Asset<ASSET_TEXTURE, TextureAsset, vke_render::Texture2D>
    {
    public:
        VkFormat format;
        VkImageUsageFlags usage;
        VkImageLayout layout;
        VkFilter minFilter;
        VkFilter magFilter;
        VkSamplerAddressMode addressMode;
        bool anisotropyEnable;

        TextureAsset() {}

        TextureAsset(AssetHandle id, nlohmann::json &json) : Asset(id, json)
        {
            format = json.contains("format") ? json["format"] : VK_FORMAT_R8G8B8A8_SRGB;
            usage = json.contains("usage") ? json["usage"] : VK_IMAGE_USAGE_SAMPLED_BIT;
            layout = json.contains("layout") ? json["layout"] : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            minFilter = json.contains("minFilter") ? json["minFilter"] : VK_FILTER_LINEAR;
            magFilter = json.contains("magFilter") ? json["magFilter"] : VK_FILTER_LINEAR;
            addressMode = json.contains("addressMode") ? json["addressMode"] : VK_SAMPLER_ADDRESS_MODE_REPEAT;
            anisotropyEnable = json.contains("anisotropy") ? json["anisotropy"] : true;
        }

        TextureAsset(AssetHandle id, const std::string &nm, const std::string &pth)
            : Asset(id, nm, pth),
              format(VK_FORMAT_R8G8B8A8_SRGB), usage(VK_IMAGE_USAGE_SAMPLED_BIT), layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
              minFilter(VK_FILTER_LINEAR), magFilter(VK_FILTER_LINEAR), addressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT), anisotropyEnable(VK_TRUE) {}

        std::string toJSON()
        {
            std::string ret = ", \"format\": " + std::to_string(format) +
                              ", \"usage\": " + std::to_string(usage) +
                              ", \"layout\": " + std::to_string(layout) +
                              ", \"minFilter\": " + std::to_string(minFilter) +
                              ", \"magFilter\": " + std::to_string(magFilter) +
                              ", \"addressMode\": " + std::to_string(addressMode) +
                              ", \"anisotropy\": " + std::to_string(anisotropyEnable);
            return ret;
        }
    };

    class VFShaderAsset : public Asset<ASSET_VF_SHADER, VFShaderAsset, vke_render::ShaderModuleSet>
    {
    public:
        std::string fragPath;

        VFShaderAsset() {}

        VFShaderAsset(AssetHandle id, nlohmann::json &json)
            : fragPath(json["fragPath"]), Asset(id, json) {}

        DEFAULT_CONSTRUCTOR2(VFShaderAsset)

        VFShaderAsset(AssetHandle id, const std::string &nm, const std::string &pth, const std::string &fragpth)
            : fragPath(fragpth), Asset(id, nm, pth) {}

        std::string toJSON()
        {
            std::string ret = ", \"fragPath\": \"" + fragPath + "\"";
            return ret;
        }
    };

    class MaterialAsset : public Asset<ASSET_MATERIAL, MaterialAsset, vke_render::Material>
    {
    public:
        AssetHandle shader;
        std::vector<AssetHandle> textures;

        MaterialAsset() {}

        MaterialAsset(AssetHandle id, nlohmann::json &json)
            : shader(json["shader"]), Asset(id, json)
        {
            auto &texs = json["textures"];
            for (auto &tex : texs)
                textures.push_back(tex);
        }

        DEFAULT_CONSTRUCTOR2(MaterialAsset)

        std::string toJSON()
        {
            std::string ret = ", \"shader\": " + std::to_string(shader);
            ret += ", \"textures\": [ ";
            for (auto texture : textures)
                ret += std::to_string(texture) + ",";
            ret[ret.length() - 1] = ']';
            return ret;
        }
    };

#define GET_ASSET_FUNC(tp, cache)            \
    static tp *Get##tp(AssetHandle id)       \
    {                                        \
        auto &it = instance->cache.find(id); \
        if (it == instance->cache.end())     \
            return nullptr;                  \
        return &(it->second);                \
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
        AssetManager() {};
        ~AssetManager() {}
        AssetManager(const AssetManager &);
        AssetManager &operator=(const AssetManager);

    public:
        AssetHandle ids[ASSET_CNT_FLAG];
        std::map<AssetHandle, TextureAsset> textureCache;
        std::map<AssetHandle, MeshAsset> meshCache;
        std::map<AssetHandle, VFShaderAsset> vfShaderCache;
        std::map<AssetHandle, ComputeShaderAsset> computeShaderCache;
        std::map<AssetHandle, MaterialAsset> materialCache;
        std::map<AssetHandle, SceneAsset> sceneCache;

        static AssetManager *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("AssetManager not initialized!");
            return instance;
        }

        static AssetManager *Init();

        static void Dispose()
        {
            delete instance;
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

        static std::shared_ptr<vke_render::Texture2D> LoadTexture2D(const AssetHandle hdl);
        static std::shared_ptr<vke_render::Mesh> LoadMesh(const AssetHandle hdl);
        static std::shared_ptr<vke_render::ShaderModuleSet> LoadVertFragShader(const AssetHandle hdl);
        static std::shared_ptr<vke_render::ShaderModuleSet> LoadComputeShader(const AssetHandle hdl);
        static std::shared_ptr<vke_render::Material> LoadMaterial(const AssetHandle hdl);

        ASSET_OP_FUNCS(TextureAsset, textureCache)
        ASSET_OP_FUNCS(MeshAsset, meshCache)
        ASSET_OP_FUNCS(VFShaderAsset, vfShaderCache)
        ASSET_OP_FUNCS(ComputeShaderAsset, computeShaderCache)
        ASSET_OP_FUNCS(MaterialAsset, materialCache)
        ASSET_OP_FUNCS(SceneAsset, sceneCache)

    private:
        void clearAssetLUT();
        void loadAssetLUT(const std::string &pth);
        void saveAssetLUT(const std::string &pth);
        void loadBuiltinAssets();
    };
}

#endif