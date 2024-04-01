#include <resource.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <fstream>
#include <string>

namespace vke_common
{
    ResourceManager *ResourceManager::instance = nullptr;

    nlohmann::json ResourceManager::LoadJSON(const std::string &pth)
    {
        nlohmann::json ret;
        std::ifstream ifs(pth, std::ios::in);
        ifs >> ret;
        ifs.close();
        return ret;
    }

    std::shared_ptr<vke_render::VertFragShader> ResourceManager::LoadVertFragShader(const std::string &vpth, const std::string &fpth)
    {
        auto &cache = instance->shaderCache;
        std::string id = vpth + "_" + fpth;
        auto it = cache.find(id);
        if (it != cache.end())
            return std::static_pointer_cast<vke_render::VertFragShader>(it->second);

        auto vcode = readFile(vpth);
        auto fcode = readFile(fpth);
        std::shared_ptr<vke_render::VertFragShader> ret = std::make_shared<vke_render::VertFragShader>(vcode, fcode);
        cache[id] = ret;
        return ret;
    }

    std::shared_ptr<vke_render::ComputeShader> ResourceManager::LoadComputeShader(const std::string &pth)
    {
        auto &cache = instance->shaderCache;
        auto it = cache.find(pth);
        if (it != cache.end())
            return std::static_pointer_cast<vke_render::ComputeShader>(it->second);

        auto code = readFile(pth);
        std::shared_ptr<vke_render::ComputeShader> ret = std::make_shared<vke_render::ComputeShader>(code);
        cache[pth] = ret;
        return ret;
    }

    std::shared_ptr<vke_render::Texture2D> ResourceManager::LoadTexture2D(const std::string &pth)
    {
        auto &cache = instance->textureCache;
        auto it = cache.find(pth);
        if (it != cache.end())
            return it->second;

        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(pth.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels)
        {
            throw std::runtime_error("failed to load texture image!");
        }

        std::shared_ptr<vke_render::Texture2D> ret = std::make_shared<vke_render::Texture2D>(pixels, texWidth, texHeight);
        cache[pth] = ret;
        return ret;
    }

    std::shared_ptr<vke_render::Material> ResourceManager::LoadMaterial(const std::string &pth)
    {
        // TODO
        auto &cache = instance->materialCache;
        auto it = cache.find(pth);
        if (it != cache.end())
        {
            std::cout << pth << " hit\n";
            return it->second;
        }

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

        vke_render::Material *mat = new vke_render::Material;
        mat->path = pth;
        mat->bindingDescriptions = bindingDescriptions;
        mat->attributeDescriptions = attributeDescriptions;

        nlohmann::json &json(LoadJSON(pth));
        mat->shader = LoadVertFragShader(json["vert"], json["frag"]);

        auto &texs = json["textures"];
        int bindingID = 0;
        for (auto &tex : texs)
        {
            std::shared_ptr<vke_render::Texture2D> texture = LoadTexture2D(tex);
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

        std::shared_ptr<vke_render::Material> ret(mat);
        cache[pth] = ret;
        return ret;
    }

    static std::shared_ptr<vke_render::Mesh> processMesh(const std::string &pth, aiMesh *mesh, const aiScene *scene)
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
        return std::make_shared<vke_render::Mesh>(pth, vertices, indices);
    }

    static std::shared_ptr<vke_render::Mesh> processNode(const std::string &pth, aiNode *node, const aiScene *scene)
    {
        std::shared_ptr<vke_render::Mesh> ret = nullptr;

        if (node->mNumMeshes == 0)
        {
            ret = std::make_shared<vke_render::Mesh>(pth);
        }
        else if (node->mNumMeshes == 1)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[0]];
            ret = processMesh(pth, mesh, scene);
        }
        else
        {
            ret = std::make_shared<vke_render::Mesh>();
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
                ret->subMeshes.push_back(processMesh(pth, mesh, scene));
            }
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            auto submesh = processNode(pth, node->mChildren[i], scene);
            if (submesh != nullptr)
                ret->subMeshes.push_back(submesh);
        }
        return ret;
    }

    std::shared_ptr<vke_render::Mesh> ResourceManager::LoadMesh(const std::string &pth)
    {
        auto &cache = instance->meshCache;
        auto it = cache.find(pth);
        if (it != cache.end())
            return it->second;

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(pth, aiProcess_Triangulate | aiProcess_GenNormals);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << "\n";
            return nullptr;
        }
        std::shared_ptr<vke_render::Mesh> ret = processNode(pth, scene->mRootNode, scene);
        cache[pth] = ret;
        return ret;
    }
};