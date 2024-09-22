#ifndef RENDER_RESOURCE_H
#define RENDER_RESOURCE_H

#include <render/shader.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/texture.hpp>
#include <nlohmann/json.hpp>

#include <physics.hpp>

#include <fstream>
#include <iostream>
#include <map>

namespace vke_common
{
    extern const std::string BuiltinPlanePath;
    extern const std::string BuiltinCubePath;
    extern const std::string BuiltinSpherePath;
    // extern const std::string BuiltinCapsulePath;
    extern const std::string BuiltinCylinderPath;
    extern const std::string BuiltinMonkeyPath;

    class ResourceManager
    {
    private:
        static ResourceManager *instance;
        ResourceManager() {};
        ~ResourceManager() {}
        ResourceManager(const ResourceManager &);
        ResourceManager &operator=(const ResourceManager);

    public:
        static ResourceManager *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("ResourceManager not initialized!");
            return instance;
        }

        static ResourceManager *Init();

        static void Dispose()
        {
            delete instance;
        }

        static nlohmann::json LoadJSON(const std::string &pth);

        static std::shared_ptr<vke_render::VertFragShader> LoadVertFragShader(const std::string &vpth, const std::string &fpth);

        static std::shared_ptr<vke_render::ComputeShader> LoadComputeShader(const std::string &pth);

        static std::shared_ptr<vke_render::Texture2D> LoadTexture2D(const std::string &pth);

        static std::shared_ptr<vke_render::Material> LoadMaterial(const std::string &pth);

        static std::shared_ptr<vke_render::Mesh> LoadMesh(const std::string &pth, const std::string &key);

        static std::shared_ptr<vke_render::Mesh> LoadMesh(const std::string &pth)
        {
            return LoadMesh(pth, pth);
        }

        static std::shared_ptr<vke_physics::PhysicsMaterial> LoadPhysicsMaterial(const std::string &pth);

    private:
        std::map<std::string, std::shared_ptr<vke_render::Shader>> shaderCache;
        std::map<std::string, std::shared_ptr<vke_render::Texture2D>> textureCache;
        std::map<std::string, std::shared_ptr<vke_render::Material>> materialCache;
        std::map<std::string, std::shared_ptr<vke_render::Mesh>> meshCache;
        std::map<std::string, std::shared_ptr<vke_physics::PhysicsMaterial>> physxMaterialCache;

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