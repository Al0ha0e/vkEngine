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
        instance->loadBuiltinAssets();
        return instance;
    }

#define LOAD_LUT_CASE(tpid, tp, assets)          \
    case tpid:                                   \
        assets[id] = tp(id, asset);              \
        ids[type] = std::max(ids[type], id + 1); \
        break;

    void AssetManager::loadAssetLUT(const std::string &pth)
    {
        nlohmann::json &json = LoadJSON(pth);
        for (auto &asset : json)
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
                LOAD_LUT_CASE(ASSET_PHYSICS_MATERIAL, PhysicsMaterialAsset, physicsMaterialCache)
                LOAD_LUT_CASE(ASSET_SCENE, SceneAsset, sceneCache)
            default:
                break;
            }
        }
    }

    extern const std::vector<vke_render::Vertex> planeVertices;
    extern const std::vector<uint32_t> planeIndices;

    void AssetManager::loadBuiltinAssets()
    {
        textureCache[BUILTIN_TEXTURE_SKYBOX_ID] = TextureAsset(BUILTIN_TEXTURE_SKYBOX_ID, "SkyboxTex", BuiltinSkyboxTexPath);
        auto &plane = MeshAsset(BUILTIN_MESH_PLANE_ID, "Plane", "");
        plane.val = std::make_shared<vke_render::Mesh>("", planeVertices, planeIndices);
        meshCache[BUILTIN_MESH_PLANE_ID] = plane;
        meshCache[BUILTIN_MESH_CUBE_ID] = MeshAsset(BUILTIN_MESH_CUBE_ID, "Cube", BuiltinCubePath);
        meshCache[BUILTIN_MESH_SPHERE_ID] = MeshAsset(BUILTIN_MESH_SPHERE_ID, "Sphere", BuiltinSpherePath);
        meshCache[BUILTIN_MESH_CYLINDER_ID] = MeshAsset(BUILTIN_MESH_CYLINDER_ID, "Cylinder", BuiltinCylinderPath);
        meshCache[BUILTIN_MESH_MONKEY_ID] = MeshAsset(BUILTIN_MESH_MONKEY_ID, "Monkey", BuiltinMonkeyPath);
        vfShaderCache[BUILTIN_VFSHADER_SKYBOX_ID] = VFShaderAsset(BUILTIN_VFSHADER_SKYBOX_ID, "SkyboxShader",
                                                                  BuiltinSkyVertShaderPath,
                                                                  BuiltinSkyFragShaderPath);

        LoadTexture2D(BUILTIN_TEXTURE_SKYBOX_ID);
        for (int i = 2; i < 6; i++)
            LoadMesh(i);
        LoadVertFragShader(BUILTIN_VFSHADER_SKYBOX_ID);
    }
};