#ifndef ASSET_H
#define ASSET_H

#include <render/shader.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/texture.hpp>
#include <nlohmann/json.hpp>

#include <physics.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>

namespace vke_common
{
    extern const std::string BuiltinSkyboxTexPath;
    extern const std::string BuiltinCubePath;
    extern const std::string BuiltinSpherePath;
    extern const std::string BuiltinCylinderPath;
    extern const std::string BuiltinMonkeyPath;
    extern const std::string BuiltinSkyVertShaderPath;
    extern const std::string BuiltinSkyFragShaderPath;

    using AssetHandle = uint64_t;

    const AssetHandle CUSTOM_ASSET_ID_ST = 1024;

    const AssetHandle BUILTIN_TEXTURE_SKYBOX_ID = 1;

    const AssetHandle BUILTIN_MESH_PLANE_ID = 1;
    const AssetHandle BUILTIN_MESH_CUBE_ID = 2;
    const AssetHandle BUILTIN_MESH_SPHERE_ID = 3;
    const AssetHandle BUILTIN_MESH_CYLINDER_ID = 4;
    const AssetHandle BUILTIN_MESH_MONKEY_ID = 5;

    const AssetHandle BUILTIN_VFSHADER_SKYBOX_ID = 1;

    enum AssetType
    {
        ASSET_TEXTURE,
        ASSET_MESH,
        ASSET_VF_SHADER,
        ASSET_COMPUTE_SHADER,
        ASSET_MATERIAL,
        ASSET_PHYSICS_MATERIAL,
        ASSET_SCENE,
        ASSET_CNT_FLAG
    };

    const std::string AssetTypeToName[] = {"Texture", "Mesh", "VFShader", "ComputeShader", "Material", "PhysicalMaterial", "Scene"};

    template <typename T, typename VT>
    class Asset
    {
    public:
        AssetType type;
        AssetHandle id;
        std::string name;
        std::string path;
        std::shared_ptr<VT> val;

        Asset() : type(ASSET_CNT_FLAG), id(0), val(nullptr) {}

        Asset(AssetType tp, AssetHandle id, nlohmann::json &json)
            : type(tp), id(id), name(json["name"]), path(json["path"]), val(nullptr) {}
        Asset(AssetType tp, AssetHandle id, const std::string &nm, const std::string &pth)
            : type(tp), id(id), name(nm), path(pth), val(nullptr) {}

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

#define DEFAULT_CONSTRUCTOR(type, typecode) \
    type(AssetHandle id, nlohmann::json &json) : Asset(typecode, id, json) {}

#define DEFAULT_CONSTRUCTOR2(type, typecode) \
    type(AssetHandle id, const std::string &nm, const std::string &pth) : Asset(typecode, id, nm, pth) {}

#define LEAF_ASSET_TYPE(type, typecode, valtype) \
    class type : public Asset<type, valtype>     \
    {                                            \
    public:                                      \
        type() {}                                \
        DEFAULT_CONSTRUCTOR(type, typecode)      \
        DEFAULT_CONSTRUCTOR2(type, typecode)     \
    };

    LEAF_ASSET_TYPE(TextureAsset, ASSET_TEXTURE, vke_render::Texture2D)
    LEAF_ASSET_TYPE(MeshAsset, ASSET_MESH, vke_render::Mesh)
    LEAF_ASSET_TYPE(ComputeShaderAsset, ASSET_COMPUTE_SHADER, vke_render::ComputeShader)
    LEAF_ASSET_TYPE(PhysicsMaterialAsset, ASSET_PHYSICS_MATERIAL, vke_physics::PhysicsMaterial)
    LEAF_ASSET_TYPE(SceneAsset, ASSET_SCENE, int);

    class VFShaderAsset : public Asset<VFShaderAsset, vke_render::VertFragShader>
    {
    public:
        std::string fragPath;

        VFShaderAsset() {}

        VFShaderAsset(AssetHandle id, nlohmann::json &json)
            : fragPath(json["fragPath"]), Asset(ASSET_VF_SHADER, id, json) {}

        DEFAULT_CONSTRUCTOR2(VFShaderAsset, ASSET_VF_SHADER)

        VFShaderAsset(AssetHandle id, const std::string &nm, const std::string &pth, const std::string &fragpth)
            : fragPath(fragpth), Asset(ASSET_VF_SHADER, id, nm, pth) {}

        std::string toJSON()
        {
            std::string ret = ", \"fragPath\": " + fragPath;
            return ret;
        }
    };

    class MaterialAsset : public Asset<MaterialAsset, vke_render::Material>
    {
    public:
        AssetHandle shader;
        std::vector<AssetHandle> textures;

        MaterialAsset() {}

        MaterialAsset(AssetHandle id, nlohmann::json &json)
            : shader(json["shader"]), Asset(ASSET_MATERIAL, id, json)
        {
            auto &texs = json["textures"];
            for (auto &tex : texs)
                textures.push_back(tex);
        }

        DEFAULT_CONSTRUCTOR2(MaterialAsset, ASSET_MATERIAL)

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

    class AssetManager
    {
    private:
        static AssetManager *instance;
        AssetManager() {};
        ~AssetManager() {}
        AssetManager(const AssetManager &);
        AssetManager &operator=(const AssetManager);

    public:
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

        static void LoadAssetLUT(const std::string &pth) { instance->loadAssetLUT(pth); }

        static void ReadFile(const std::string &filename, std::vector<char> &buffer);

        static nlohmann::json LoadJSON(const std::string &pth);

        static std::shared_ptr<vke_render::Texture2D> LoadTexture2D(const AssetHandle hdl);
        static std::shared_ptr<vke_render::Mesh> LoadMesh(const AssetHandle hdl);
        static std::shared_ptr<vke_render::VertFragShader> LoadVertFragShader(const AssetHandle hdl);
        static std::shared_ptr<vke_render::ComputeShader> LoadComputeShader(const AssetHandle hdl);
        static std::shared_ptr<vke_render::Material> LoadMaterial(const AssetHandle hdl);
        static std::shared_ptr<vke_physics::PhysicsMaterial> LoadPhysicsMaterial(const AssetHandle hdl);

    private:
        AssetHandle ids[ASSET_CNT_FLAG];
        std::map<AssetHandle, TextureAsset> textureCache;
        std::map<AssetHandle, MeshAsset> meshCache;
        std::map<AssetHandle, VFShaderAsset> vfShaderCache;
        std::map<AssetHandle, ComputeShaderAsset> computeShaderCache;
        std::map<AssetHandle, MaterialAsset> materialCache;
        std::map<AssetHandle, PhysicsMaterialAsset> physicsMaterialCache;
        std::map<AssetHandle, SceneAsset> sceneCache;

        void loadAssetLUT(const std::string &pth);
        void loadBuiltinAssets();
    };
}

#endif