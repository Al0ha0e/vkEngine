#include <asset.hpp>
#include <logger.hpp>
#include <stb/stb_image.h>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>

namespace vke_common
{
    static inline void readFile(const std::string &filename, std::vector<char> &buffer)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        buffer.resize(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
    }

    void AssetManager::ReadFile(const std::string &filename, std::vector<char> &buffer)
    {
        readFile(filename, buffer);
    }

    static inline nlohmann::json loadJSON(const std::string &pth)
    {
        nlohmann::json ret;
        std::ifstream ifs(pth, std::ios::in);
        ifs >> ret;
        ifs.close();
        return ret;
    }

    nlohmann::json AssetManager::LoadJSON(const std::string &pth)
    {
        return loadJSON(pth);
    }

    template <typename AT>
    static inline AT &tryGetAsset(std::map<AssetHandle, AT> &assets, const AssetHandle hdl)
    {
        auto it = assets.find(hdl);
        if (it == assets.end())
            throw std::runtime_error("Asset Not Exist!\n");
        return it->second;
    }

    template <typename T, typename AT>
    static inline std::shared_ptr<T> loadFromCacheOrUpdate(
        std::map<AssetHandle, AT> &assets,
        const AssetHandle hdl,
        std::function<std::unique_ptr<T>(AT &)> &load)
    {
        auto &asset = tryGetAsset(assets, hdl);
        if (asset.val != nullptr)
            return asset.val;

        asset.val = load(asset);
        return asset.val;
    }

    static inline std::unique_ptr<vke_render::Texture2D> loadTexture2D(const AssetHandle hdl, const TextureAsset &asset)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(asset.path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels)
        {
            throw std::runtime_error("failed to load texture image!");
        }

        return std::make_unique<vke_render::Texture2D>(hdl, pixels, texWidth, texHeight,
                                                       asset.format, asset.usage, asset.layout,
                                                       asset.minFilter, asset.magFilter, asset.addressMode,
                                                       asset.anisotropyEnable);
    }

    std::unique_ptr<vke_render::Texture2D> AssetManager::LoadTexture2DUnique(const AssetHandle hdl)
    {
        auto &asset = tryGetAsset(instance->textureCache, hdl);
        return loadTexture2D(hdl, asset);
    }

    std::shared_ptr<vke_render::Texture2D> AssetManager::LoadTexture2D(const AssetHandle hdl)
    {
        std::function<std::unique_ptr<vke_render::Texture2D>(TextureAsset &)> op = [hdl](TextureAsset &asset)
        { return loadTexture2D(hdl, asset); };

        return loadFromCacheOrUpdate<vke_render::Texture2D>(instance->textureCache, hdl, op);
    }

    static inline std::unique_ptr<vke_render::Mesh> loadMesh(const AssetHandle hdl, const std::string &pth)
    {
        std::ifstream file(pth, std::ios::binary);
        return std::make_unique<vke_render::Mesh>(hdl, file);
    }

    std::unique_ptr<vke_render::Mesh> AssetManager::LoadMeshUnique(const AssetHandle hdl)
    {
        auto &asset = tryGetAsset(instance->meshCache, hdl);
        return loadMesh(hdl, asset.path);
    }

    std::shared_ptr<vke_render::Mesh> AssetManager::LoadMesh(const AssetHandle hdl)
    {
        std::function<std::unique_ptr<vke_render::Mesh>(MeshAsset &)> op = [hdl](MeshAsset &asset)
        { return loadMesh(hdl, asset.path); };

        return loadFromCacheOrUpdate<vke_render::Mesh>(instance->meshCache, hdl, op);
    }

    static inline std::unique_ptr<vke_render::ShaderModuleSet> loadVertFragShader(const AssetHandle hdl, const std::string &vpth, const std::string &fpth)
    {
        std::vector<char> vcode, fcode;
        readFile(vpth, vcode);
        readFile(fpth, fcode);
        return std::make_unique<vke_render::ShaderModuleSet>(vcode, fcode);
    }

    std::unique_ptr<vke_render::ShaderModuleSet> AssetManager::LoadVertFragShaderUnique(const AssetHandle hdl)
    {
        auto &asset = tryGetAsset(instance->vfShaderCache, hdl);
        return loadVertFragShader(hdl, asset.path, asset.fragPath);
    }

    std::shared_ptr<vke_render::ShaderModuleSet> AssetManager::LoadVertFragShader(const AssetHandle hdl)
    {
        std::function<std::unique_ptr<vke_render::ShaderModuleSet>(VFShaderAsset &)> op = [hdl](VFShaderAsset &asset)
        { return loadVertFragShader(hdl, asset.path, asset.fragPath); };

        return loadFromCacheOrUpdate<vke_render::ShaderModuleSet>(instance->vfShaderCache, hdl, op);
    }

    static inline std::unique_ptr<vke_render::ShaderModuleSet> loadComputeShader(const AssetHandle hdl, const std::string &pth)
    {
        std::vector<char> code;
        readFile(pth, code);
        return std::make_unique<vke_render::ShaderModuleSet>(code);
    }

    std::unique_ptr<vke_render::ShaderModuleSet> AssetManager::LoadComputeShaderUnique(const AssetHandle hdl)
    {
        auto &asset = tryGetAsset(instance->computeShaderCache, hdl);
        return loadComputeShader(hdl, asset.path);
    }

    std::shared_ptr<vke_render::ShaderModuleSet> AssetManager::LoadComputeShader(const AssetHandle hdl)
    {
        std::function<std::unique_ptr<vke_render::ShaderModuleSet>(ComputeShaderAsset &)> op = [hdl](ComputeShaderAsset &asset)
        { return loadComputeShader(hdl, asset.path); };

        return loadFromCacheOrUpdate<vke_render::ShaderModuleSet>(instance->computeShaderCache, hdl, op);
    }

    std::unique_ptr<vke_render::Material> loadMaterial(MaterialAsset &asset)
    {
        vke_render::Material *mat = new vke_render::Material(asset.id);

        mat->shader = AssetManager::LoadVertFragShader(asset.shader);
        int bindingID = 0;
        for (auto tex : asset.textures)
            mat->textures.push_back(AssetManager::LoadTexture2D(tex));
        mat->textureBindingInfos = asset.textureBindingInfos;

        return std::unique_ptr<vke_render::Material>(mat);
    }

    std::unique_ptr<vke_render::Material> AssetManager::LoadMaterialUnique(const AssetHandle hdl)
    {
        auto &asset = tryGetAsset(instance->materialCache, hdl);
        return loadMaterial(asset);
    }

    std::shared_ptr<vke_render::Material> AssetManager::LoadMaterial(const AssetHandle hdl)
    {
        std::function<std::unique_ptr<vke_render::Material>(MaterialAsset &)> op(loadMaterial);
        return loadFromCacheOrUpdate<vke_render::Material>(instance->materialCache, hdl, op);
    }
}