#ifndef RENDER_RESOURCE_H
#define RENDER_RESOURCE_H

#include <render/shader.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/texture.hpp>
#include <fstream>
#include <iostream>
#include <map>

namespace vke_render
{
    class RenderResourceManager
    {
    private:
        static RenderResourceManager *instance;
        RenderResourceManager() {}
        ~RenderResourceManager() {}
        RenderResourceManager(const RenderResourceManager &);
        RenderResourceManager &operator=(const RenderResourceManager);

    public:
        static RenderResourceManager *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("RenderResourceManager not initialized!");
            return instance;
        }

        static RenderResourceManager *Init()
        {
            instance = new RenderResourceManager();
            return instance;
        }

        static void Dispose()
        {
            delete instance;
        }

        static std::shared_ptr<std::string> LoadJSON(const std::string &pth)
        {
            auto content = readFile(pth);
            std::shared_ptr<std::string> ret = std::make_shared<std::string>(content.data(), content.size());
            return ret;
        }

        static std::shared_ptr<VertFragShader> LoadVertFragShader(const std::string &vpth, const std::string &fpth)
        {
            auto &cache = instance->shaderCache;
            std::string id = vpth + "_" + fpth;
            auto it = cache.find(id);
            if (it != cache.end())
                return std::static_pointer_cast<VertFragShader>(it->second);

            auto vcode = readFile(vpth);
            auto fcode = readFile(fpth);
            std::shared_ptr<VertFragShader> ret = std::make_shared<VertFragShader>(vcode, fcode);
            cache[id] = ret;
            return ret;
        }

        static std::shared_ptr<ComputeShader> LoadComputeShader(const std::string &pth)
        {
            auto &cache = instance->shaderCache;
            auto it = cache.find(pth);
            if (it != cache.end())
                return std::static_pointer_cast<ComputeShader>(it->second);

            auto code = readFile(pth);
            std::shared_ptr<ComputeShader> ret = std::make_shared<ComputeShader>(code);
            cache[pth] = ret;
            return ret;
        }

        static std::shared_ptr<Texture2D> LoadTexture2D(const std::string &pth)
        {
            int texWidth, texHeight, texChannels;
            stbi_uc *pixels = stbi_load(pth.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if (!pixels)
            {
                throw std::runtime_error("failed to load texture image!");
            }

            return std::make_shared<Texture2D>(pixels, texWidth, texHeight);
        }

        static std::shared_ptr<Material> LoadMaterial(const std::string &pth)
        {
            // TODO
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            std::vector<VkVertexInputBindingDescription> bindingDescriptions;
            bindingDescriptions.push_back(bindingDescription);

            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            Material *ret = new Material;

            std::shared_ptr<Texture2D> texture = LoadTexture2D("./resources/texture/texture.jpg");
            ret->textures.push_back(texture);
            ret->shader = LoadVertFragShader("./tests/shader/vert.spv", "./tests/shader/frag.spv");
            ret->bindingDescriptions = bindingDescriptions;
            ret->attributeDescriptions = attributeDescriptions;

            VkDescriptorSetLayoutBinding modelLayoutBinding{};
            InitDescriptorSetLayoutBinding(modelLayoutBinding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

            vke_render::DescriptorInfo descriptorInfo(modelLayoutBinding, sizeof(glm::mat4));
            ret->perUnitDescriptorInfos.push_back(std::move(descriptorInfo));

            VkDescriptorSetLayoutBinding textureLayoutBinding{};
            InitDescriptorSetLayoutBinding(textureLayoutBinding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                           VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

            vke_render::DescriptorInfo textureDescriptorInfo(textureLayoutBinding, texture->textureImageView, texture->textureSampler);
            ret->commonDescriptorInfos.push_back(std::move(textureDescriptorInfo));

            return std::shared_ptr<Material>(ret);
        }

        static std::shared_ptr<Mesh> LoadMesh(const std::string &pth)
        {
            // TODO
            const std::vector<Vertex> vertices = {
                {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};
            std::vector<uint32_t> indices = {
                0, 1, 2, 2, 3, 0};
            std::shared_ptr<Mesh> ret = std::make_shared<Mesh>(vertices.size() * sizeof(Vertex), (void *)vertices.data(), indices);
            return ret;
        }

    private:
        std::map<std::string, std::shared_ptr<Shader>> shaderCache;
        std::map<std::string, Material *> materialCache;

        static std::vector<char> readFile(const std::string &filename)
        {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open())
            {
                throw std::runtime_error("failed to open file!");
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();

            return buffer;
        }
    };
}

#endif