#ifndef RENDER_RESOURCE_H
#define RENDER_RESOURCE_H

#include <render/shader.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
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

        static Shader *LoadShader(std::string vpth, std::string fpth)
        {
            auto &cache = instance->shaderCache;
            std::string id = vpth + "_" + fpth;
            auto it = cache.find(id);
            if (it != cache.end())
                return it->second;

            auto vcode = readFile(vpth);
            auto fcode = readFile(fpth);
            Shader *ret = new Shader(vcode, fcode);
            cache[id] = ret;
            return ret;
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

            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            Material *ret = new Material;
            ret->shader = LoadShader("./tests/shader/vert.spv", "./tests/shader/frag.spv");
            ret->bindingDescriptions = bindingDescriptions;
            ret->attributeDescriptions = attributeDescriptions;

            VkDescriptorSetLayoutBinding modelLayoutBinding;
            modelLayoutBinding.binding = 0;
            modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            modelLayoutBinding.descriptorCount = 1;
            modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            modelLayoutBinding.pImmutableSamplers = nullptr; // Optional

            vke_render::DescriptorInfo descriptorInfo(modelLayoutBinding, sizeof(glm::mat4));
            ret->perUnitDescriptorInfos.push_back(descriptorInfo);

            return ret;
        }

        static Mesh *LoadMesh(std::string pth)
        {
            // TODO
            const std::vector<Vertex> vertices = {
                {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
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