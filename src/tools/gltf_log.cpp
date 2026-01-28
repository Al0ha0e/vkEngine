#include <tinygltf/tiny_gltf.h>
#include <logger.hpp>
#include <iostream>
#include <filesystem>

void logNode(tinygltf::Model &model, tinygltf::Node &node, uint32_t id);

void logSkin(tinygltf::Model &model, uint32_t id);

void logMesh(tinygltf::Model &model, uint32_t id);

void logMaterial(tinygltf::Model &model, uint32_t id);

int main(int argc, char **argv)
{
    vke_common::Logger::Init();

    if (argc != 2)
        return -1;

    std::filesystem::path pth(argv[1]);

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    loader.LoadBinaryFromFile(&model, &err, &warn, pth.string().c_str());

    VKE_LOG_INFO("NODE CNT {} SCENE CNT {} SKIN CNT {}", model.nodes.size(), model.scenes.size(), model.skins.size());

    for (auto &scene : model.scenes)
    {
        VKE_LOG_INFO("SCENE {}", scene.name);
        for (int nodeID : scene.nodes)
            logNode(model, model.nodes[nodeID], nodeID);
    }

    return 0;
}

void logNode(tinygltf::Model &model, tinygltf::Node &node, uint32_t id)
{
    VKE_LOG_INFO(">>>> NODE ID {} {}", id, node.name)
    VKE_LOG_INFO("NODE SKIN {} {}", node.skin, node.skin == -1 ? "" : "@@@@@@@@@@")

    if (node.skin != -1)
        logSkin(model, node.skin);

    if (node.mesh != -1)
        logMesh(model, node.mesh);

    VKE_LOG_INFO("{} CHILDREN ST-------------", node.name)
    for (int child : node.children)
        logNode(model, model.nodes[child], child);
    VKE_LOG_INFO("{} CHILDREN EN-------------", node.name)
}

void logSkin(tinygltf::Model &model, uint32_t id)
{
    auto &skin = model.skins[id];
    VKE_LOG_INFO("SKIN ID {} {}", id, skin.name)
    VKE_LOG_INFO("INV_BIND_MAT {} SKELETON {} JOINT_CNT {}",
                 skin.inverseBindMatrices, skin.skeleton, skin.joints.size())
    auto &accessor = model.accessors[skin.inverseBindMatrices];
    auto &view = model.bufferViews[accessor.bufferView];
    auto &buffer = model.buffers[view.buffer];
    VKE_LOG_INFO("INV ACC byteOffset {} componentType {} count {} type {}", accessor.byteOffset, accessor.componentType, accessor.count, accessor.type)
    VKE_LOG_INFO("INV VIEW byteLength {} byteOffset {} byteStride {}", view.byteLength, view.byteOffset, view.byteStride)
    for (int i : skin.joints)
        VKE_LOG_INFO("JOINT ID {} NAME {}", i, model.nodes[i].name)
    std::cout << "\n";
}

void logMesh(tinygltf::Model &model, uint32_t id)
{
    auto &mesh = model.meshes[id];
    VKE_LOG_INFO("~~~~~~~~~~~MESH ID {} {}", id, mesh.name)
    VKE_LOG_INFO("PRIM_CNT {}", mesh.primitives.size());

    for (int i = 0; i < mesh.primitives.size(); ++i)
    {
        auto &prim = mesh.primitives[i];
        VKE_LOG_INFO("PRIM ID {} MATERIAL {} INDICES_ID {} ATTR_CNT {}",
                     i, prim.material, prim.indices, prim.attributes.size())
        if (prim.material != -1)
            logMaterial(model, prim.material);
        for (auto &[k, v] : prim.attributes)
        {
            auto &accessor = model.accessors[v];
            VKE_LOG_INFO("ATTR NAME {} V {} CTYPE {} TYPE {}",
                         k, v, accessor.componentType, accessor.type)
        }
    }
}

#define LOG_TEXTURE(model, prefix, texture)                                                                              \
    {                                                                                                                    \
        if (texture.index == -1)                                                                                         \
            VKE_LOG_INFO("{} {}", prefix, texture.index)                                                                 \
        else                                                                                                             \
        {                                                                                                                \
            int source = model.textures[texture.index].source;                                                           \
            auto &image = model.images[source];                                                                          \
            VKE_LOG_INFO("{} {} SOURCE {} MIMETYPE {} URI {}", prefix, texture.index, source, image.mimeType, image.uri) \
        }                                                                                                                \
    }

void logMaterial(tinygltf::Model &model, uint32_t id)
{
    auto material = model.materials[id];
    VKE_LOG_INFO("---------------- LOG MATERIAL ST -----------------------")
    VKE_LOG_INFO("~~~~~~~~~~~MATERIAL ID {} {}", id, material.name)
    auto &pbrMetallicRoughness = material.pbrMetallicRoughness;
    LOG_TEXTURE(model, "~~~~~~~~~~~ BASECOLOR", pbrMetallicRoughness.baseColorTexture)
    LOG_TEXTURE(model, "~~~~~~~~~~~ MR", pbrMetallicRoughness.metallicRoughnessTexture)
    LOG_TEXTURE(model, "~~~~~~~~~~~ NORMAL {}", material.normalTexture)
    LOG_TEXTURE(model, "~~~~~~~~~~~ OCCLU {}", material.occlusionTexture)
    LOG_TEXTURE(model, "~~~~~~~~~~~ EMISSIVE {}", material.emissiveTexture)
    VKE_LOG_INFO("---------------- LOG MATERIAL EN -----------------------")
}