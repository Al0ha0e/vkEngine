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

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (RenderResourceManager::instance != nullptr)
                    delete RenderResourceManager::instance;
            }
        };
        static Deletor deletor;

    public:
        static RenderResourceManager *GetInstance()
        {
            if (instance == nullptr)
                instance = new RenderResourceManager();
            return instance;
        }

        static RenderResourceManager *Init()
        {
            instance = new RenderResourceManager();
            return instance;
        }

        static void Dispose() {}

        static VertFragShader *LoadVertFragShader(std::string vpth, std::string fpth)
        {
            auto &cache = instance->shaderCache;
            std::string id = vpth + "_" + fpth;
            auto it = cache.find(id);
            if (it != cache.end())
                return (VertFragShader *)it->second;

            auto vcode = readFile(vpth);
            auto fcode = readFile(fpth);
            VertFragShader *ret = new VertFragShader(vcode, fcode);
            cache[id] = ret;
            return ret;
        }

        static ComputeShader *LoadComputeShader(std::string pth)
        {
            auto &cache = instance->shaderCache;
            auto it = cache.find(pth);
            if (it != cache.end())
                return (ComputeShader *)it->second;

            auto code = readFile(pth);
            ComputeShader *ret = new ComputeShader(code);
            cache[pth] = ret;
            return ret;
        }

        static Texture2D *LoadTexture2D(std::string pth)
        {
            int texWidth, texHeight, texChannels;
            stbi_uc *pixels = stbi_load(pth.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if (!pixels)
            {
                throw std::runtime_error("failed to load texture image!");
            }

            return new Texture2D(pixels, texWidth, texHeight);
        }

        static Material *LoadMaterial(std::string pth)
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

            Texture2D *texture = LoadTexture2D("./resources/texture/texture.jpg");
            ret->textures.push_back(texture);
            ret->shader = LoadVertFragShader("./tests/shader/vert.spv", "./tests/shader/frag.spv");
            ret->bindingDescriptions = bindingDescriptions;
            ret->attributeDescriptions = attributeDescriptions;

            VkDescriptorSetLayoutBinding modelLayoutBinding{};
            modelLayoutBinding.binding = 0;
            modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            modelLayoutBinding.descriptorCount = 1;
            modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            modelLayoutBinding.pImmutableSamplers = nullptr; // Optional
            vke_render::DescriptorInfo descriptorInfo(modelLayoutBinding, sizeof(glm::mat4));
            ret->perUnitDescriptorInfos.push_back(descriptorInfo);

            VkDescriptorSetLayoutBinding textureLayoutBinding{};
            textureLayoutBinding.binding = 0;
            textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textureLayoutBinding.descriptorCount = 1;
            textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            textureLayoutBinding.pImmutableSamplers = nullptr; // Optional
            vke_render::DescriptorInfo textureDescriptorInfo(textureLayoutBinding, texture->textureImageView, texture->textureSampler);
            ret->commonDescriptorInfos.push_back(textureDescriptorInfo);

            return ret;
        }

        static Mesh *LoadMesh(std::string pth)
        {
            // TODO
            const std::vector<Vertex> vertices = {
                {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};
            std::vector<uint32_t> indices = {
                0, 1, 2, 2, 3, 0};
            Mesh *ret = new Mesh(vertices.size() * sizeof(Vertex), (void *)vertices.data(), indices);
            return ret;
        }

    private:
        std::map<std::string, Shader *> shaderCache;
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