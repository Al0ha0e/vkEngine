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

    template <typename T, typename AT>
    static inline std::shared_ptr<T> loadFromCacheOrUpdate(
        std::map<AssetHandle, AT> &assets,
        const AssetHandle hdl,
        std::function<std::shared_ptr<T>(AT &)> &load)
    {
        auto it = assets.find(hdl);
        if (it == assets.end())
            throw std::runtime_error("Asset Not Exist!\n");

        auto &asset = it->second;
        if (asset.val != nullptr)
            return asset.val;

        auto ret = load(asset);
        asset.val = ret;
        return ret;
    }

    static inline std::shared_ptr<vke_render::Texture2D> loadTexture2D(const AssetHandle hdl, const std::string &pth)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(pth.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels)
        {
            throw std::runtime_error("failed to load texture image!");
        }

        return std::make_shared<vke_render::Texture2D>(hdl, pixels, texWidth, texHeight);
    }

    std::shared_ptr<vke_render::Texture2D> AssetManager::LoadTexture2D(const AssetHandle hdl)
    {
        std::function<std::shared_ptr<vke_render::Texture2D>(TextureAsset &)> op = [hdl](TextureAsset &asset)
        { return loadTexture2D(hdl, asset.path); };

        return loadFromCacheOrUpdate<vke_render::Texture2D>(instance->textureCache, hdl, op);
    }

    static std::shared_ptr<vke_render::Mesh> processNode(const AssetHandle hdl, aiNode *node, const aiScene *scene);

    static inline std::shared_ptr<vke_render::Mesh> loadMesh(const AssetHandle hdl, const std::string &pth)
    {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(pth, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_MakeLeftHanded);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << "\n";
            return nullptr;
        }
        return processNode(hdl, scene->mRootNode, scene);
    }

    std::shared_ptr<vke_render::Mesh> AssetManager::LoadMesh(const AssetHandle hdl)
    {
        std::function<std::shared_ptr<vke_render::Mesh>(MeshAsset &)> op = [hdl](MeshAsset &asset)
        { return loadMesh(hdl, asset.path); };

        return loadFromCacheOrUpdate<vke_render::Mesh>(instance->meshCache, hdl, op);
    }

    static inline std::shared_ptr<vke_render::VertFragShader> loadVertFragShader(const AssetHandle hdl, const std::string &vpth, const std::string &fpth)
    {
        std::vector<char> vcode, fcode;
        readFile(vpth, vcode);
        readFile(fpth, fcode);
        return std::make_shared<vke_render::VertFragShader>(hdl, vcode, fcode);
    }

    std::shared_ptr<vke_render::VertFragShader> AssetManager::LoadVertFragShader(const AssetHandle hdl)
    {
        std::function<std::shared_ptr<vke_render::VertFragShader>(VFShaderAsset &)> op = [hdl](VFShaderAsset &asset)
        { return loadVertFragShader(hdl, asset.path, asset.fragPath); };

        return loadFromCacheOrUpdate<vke_render::VertFragShader>(instance->vfShaderCache, hdl, op);
    }

    static inline std::shared_ptr<vke_render::ComputeShader> loadComputeShader(const AssetHandle hdl, const std::string &pth)
    {
        std::vector<char> code;
        readFile(pth, code);
        return std::make_shared<vke_render::ComputeShader>(hdl, code);
    }

    std::shared_ptr<vke_render::ComputeShader> AssetManager::LoadComputeShader(const AssetHandle hdl)
    {
        std::function<std::shared_ptr<vke_render::ComputeShader>(ComputeShaderAsset &)> op = [hdl](ComputeShaderAsset &asset)
        { return loadComputeShader(hdl, asset.path); };

        return loadFromCacheOrUpdate<vke_render::ComputeShader>(instance->computeShaderCache, hdl, op);
    }

    std::shared_ptr<vke_render::Material> loadMaterial(MaterialAsset &asset)
    {
        // TODO
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(vke_render::Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        bindingDescriptions.push_back(bindingDescription);

        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(vke_render::Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(vke_render::Vertex, normal);
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(vke_render::Vertex, texCoord);

        vke_render::Material *mat = new vke_render::Material(asset.id);
        mat->bindingDescriptions = bindingDescriptions;
        mat->attributeDescriptions = attributeDescriptions;

        mat->shader = AssetManager::LoadVertFragShader(asset.shader);

        int bindingID = 0;
        for (auto tex : asset.textures)
        {
            std::shared_ptr<vke_render::Texture2D> texture = AssetManager::LoadTexture2D(tex);
            mat->textures.push_back(texture);
            VkDescriptorSetLayoutBinding textureLayoutBinding{};
            vke_render::InitDescriptorSetLayoutBinding(textureLayoutBinding, bindingID++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                       VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

            vke_render::DescriptorInfo textureDescriptorInfo(textureLayoutBinding, texture->textureImageView, texture->textureSampler);
            mat->commonDescriptorInfos.push_back(std::move(textureDescriptorInfo));
        }

        VkDescriptorSetLayoutBinding modelLayoutBinding{};
        vke_render::InitDescriptorSetLayoutBinding(modelLayoutBinding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

        vke_render::DescriptorInfo descriptorInfo(modelLayoutBinding, sizeof(glm::mat4));
        mat->perUnitDescriptorInfos.push_back(std::move(descriptorInfo));

        return std::shared_ptr<vke_render::Material>(mat);
    }

    std::shared_ptr<vke_render::Material> AssetManager::LoadMaterial(const AssetHandle hdl)
    {
        std::function<std::shared_ptr<vke_render::Material>(MaterialAsset &)> op(loadMaterial);
        return loadFromCacheOrUpdate<vke_render::Material>(instance->materialCache, hdl, op);
    }

    static inline std::shared_ptr<vke_physics::PhysicsMaterial> loadPhysicsMaterial(const AssetHandle hdl, const std::string &pth)
    {
        nlohmann::json &json(loadJSON(pth));

        physx::PxMaterial *mat = vke_physics::PhysicsManager::GetInstance()->gPhysics->createMaterial(
            json["static_friction"], json["dynamic_friction"], json["restitution"]);

        return std::make_shared<vke_physics::PhysicsMaterial>(hdl, mat);
    }

    std::shared_ptr<vke_physics::PhysicsMaterial> AssetManager::LoadPhysicsMaterial(const AssetHandle hdl)
    {
        std::function<std::shared_ptr<vke_physics::PhysicsMaterial>(PhysicsMaterialAsset &)> op = [hdl](PhysicsMaterialAsset &asset)
        { return loadPhysicsMaterial(hdl, asset.path); };

        return loadFromCacheOrUpdate<vke_physics::PhysicsMaterial>(instance->physicsMaterialCache, hdl, op);
    }

    static std::shared_ptr<vke_render::Mesh> processMesh(const AssetHandle hdl, aiMesh *mesh, const aiScene *scene)
    {
        std::vector<vke_render::Vertex> vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            glm::vec3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            glm::vec3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            glm::vec2 texcoord(0, 0);
            if (mesh->mTextureCoords[0])
                texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

            vertices.emplace_back(pos, normal, texcoord);
        }
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        return std::make_shared<vke_render::Mesh>(hdl, vertices, indices);
    }

    static std::shared_ptr<vke_render::Mesh> processNode(const AssetHandle hdl, aiNode *node, const aiScene *scene)
    {
        std::shared_ptr<vke_render::Mesh> ret = nullptr;
        if (node->mNumMeshes == 0)
        {
            ret = std::make_shared<vke_render::Mesh>(hdl);
        }
        else if (node->mNumMeshes == 1)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[0]];
            ret = processMesh(hdl, mesh, scene);
        }
        else
        {
            ret = std::make_shared<vke_render::Mesh>();
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
                ret->subMeshes.push_back(processMesh(hdl, mesh, scene));
            }
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            auto submesh = processNode(hdl, node->mChildren[i], scene);
            if (submesh != nullptr)
                ret->subMeshes.push_back(submesh);
        }
        return ret;
    }

}