#include <common.hpp>
#include <render/mesh.hpp>
#include <tinygltf/tiny_gltf.h>
#include <nlohmann/json.hpp>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/memory/unique_ptr.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <unordered_set>

bool checkScene(const tinygltf::Model &model, const tinygltf::Scene &scene);
void processSkinNode(const tinygltf::Model &model, const tinygltf::Node &node, const std::string &opth, std::map<std::string, uint32_t> &nameMap);
ozz::unique_ptr<ozz::animation::Skeleton> buildOzzSkeleton(const tinygltf::Model &model, const tinygltf::Node &node);

int main(int argc, char **argv)
{
    if (argc != 3)
        return -1;

    std::filesystem::path pth(argv[1]);
    std::filesystem::path odir(argv[2]);
    std::filesystem::path opth = odir / pth.stem();
    opth += std::string(".mesh");

    VKE_LOG_INFO("PTH {} ODIR {}", pth.string(), odir.string())

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    loader.LoadBinaryFromFile(&model, &err, &warn, pth.string().c_str());

    VKE_FATAL_IF(!checkScene(model, model.scenes[0]), "Invalid gltf format!")
    VKE_LOG_INFO("GLTF CHECK OK")

    auto &root = model.nodes[model.scenes[0].nodes[0]];

    const tinygltf::Node &node = model.nodes[root.children[0]];

    std::map<std::string, uint32_t> nameMap;
    ozz::unique_ptr<ozz::animation::Skeleton> skeleton = buildOzzSkeleton(model, root);

    const auto &names = skeleton->joint_names();

    for (int i = 0; i < names.size(); ++i)
        nameMap[std::string(names[i])] = i;

    for (auto &[k, v] : nameMap)
        VKE_LOG_INFO("K {} V {}", k, v)

    processSkinNode(model, node, opth.string(), nameMap);

    return 0;
}

bool checkAttribute(const char *name,
                    const tinygltf::Model &model,
                    const tinygltf::Primitive &prim,
                    const int type, const int ctype)
{
    auto it = prim.attributes.find(name);
    if (it == prim.attributes.end())
        return true;
    VKE_LOG_INFO("CHECK ATTR {}", name)
    const tinygltf::Accessor &accessor = model.accessors[it->second];
    if (accessor.type != type || accessor.componentType != ctype)
        return false;
    return true;
}

bool checkMesh(const tinygltf::Model &model, const tinygltf::Node &node)
{
    if (node.mesh == -1)
        return false;

    auto &mesh = model.meshes[node.mesh];
    for (auto &prim : mesh.primitives)
    {
        if (prim.attributes.size() == 0)
            return false;
        int cnt = model.accessors[prim.attributes.begin()->second].count;
        for (auto &[name, idx] : prim.attributes)
            if (model.accessors[idx].count != cnt)
                return false;

        if (!checkAttribute("NORMAL", model, prim, TINYGLTF_TYPE_VEC3, TINYGLTF_PARAMETER_TYPE_FLOAT) ||
            !checkAttribute("POSITION", model, prim, TINYGLTF_TYPE_VEC3, TINYGLTF_PARAMETER_TYPE_FLOAT) ||
            !checkAttribute("TANGENT", model, prim, TINYGLTF_TYPE_VEC4, TINYGLTF_PARAMETER_TYPE_FLOAT) ||
            !checkAttribute("TEXCOORD_0", model, prim, TINYGLTF_TYPE_VEC2, TINYGLTF_PARAMETER_TYPE_FLOAT))
            return false;

        if (prim.indices == -1)
            return false;
        const tinygltf::Accessor &accessor = model.accessors[prim.indices];
        if (accessor.type != TINYGLTF_TYPE_SCALAR)
            return false;
        if (accessor.componentType != TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT &&
            accessor.componentType != TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
            return false;
    }
    return true;
}

bool checkSkin(const tinygltf::Model &model, const int meshID)
{
    auto &mesh = model.meshes[meshID];
    VKE_LOG_INFO("CHECK SKIN {} NAME {} PRIM_CNT {}", meshID, mesh.name, mesh.primitives.size())
    for (auto &prim : mesh.primitives)
    {
        if (prim.attributes.size() == 0)
            return false;
        int cnt = model.accessors[prim.attributes.begin()->second].count;
        for (auto &[name, idx] : prim.attributes)
            if (model.accessors[idx].count != cnt)
                return false;

        if (!checkAttribute("NORMAL", model, prim, TINYGLTF_TYPE_VEC3, TINYGLTF_PARAMETER_TYPE_FLOAT) ||
            !checkAttribute("POSITION", model, prim, TINYGLTF_TYPE_VEC3, TINYGLTF_PARAMETER_TYPE_FLOAT) ||
            !checkAttribute("TANGENT", model, prim, TINYGLTF_TYPE_VEC4, TINYGLTF_PARAMETER_TYPE_FLOAT) ||
            !checkAttribute("TEXCOORD_0", model, prim, TINYGLTF_TYPE_VEC2, TINYGLTF_PARAMETER_TYPE_FLOAT) ||
            !checkAttribute("JOINTS_0", model, prim, TINYGLTF_TYPE_VEC4, TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) ||
            !checkAttribute("WEIGHTS_0", model, prim, TINYGLTF_TYPE_VEC4, TINYGLTF_PARAMETER_TYPE_FLOAT))
            return false;

        if (prim.indices == -1)
            return false;
        const tinygltf::Accessor &accessor = model.accessors[prim.indices];
        if (accessor.type != TINYGLTF_TYPE_SCALAR)
            return false;
        if (accessor.componentType != TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT &&
            accessor.componentType != TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
            return false;
    }
    return true;
}

bool checkSkinNode(const tinygltf::Model &model, const int nodeID, std::unordered_set<int> &visited)
{
    visited.insert(nodeID);
    const tinygltf::Node &node = model.nodes[nodeID];
    VKE_LOG_INFO("CHECK SKINnode ID {} NAME {} MESH {}", nodeID, node.name, node.mesh)
    if (node.mesh == -1 || !checkSkin(model, node.mesh))
        return false;
    auto &skin = model.skins[node.skin];
    for (int jointID : skin.joints)
        visited.insert(jointID);
    return true;
}

bool checkNode(const tinygltf::Model &model, const int nodeID, std::unordered_set<int> &visited)
{
    visited.insert(nodeID);
    const tinygltf::Node &node = model.nodes[nodeID];
    VKE_LOG_INFO("CHECK NODE ID {} NAME {} MESH {} CHILDREN CNT {}", nodeID, node.name, node.mesh, node.children.size())
    if (node.mesh == -1)
    {
        if (node.children.size() == 2 && model.nodes[node.children[0]].skin != -1 && !checkSkinNode(model, node.children[0], visited))
            return false;
    }
    else
    {
        if (!checkMesh(model, node))
            return false;
    }

    for (int child : node.children)
        if (visited.find(child) == visited.end() && !checkNode(model, child, visited))
            return false;
    return true;
}

bool checkScene(const tinygltf::Model &model, const tinygltf::Scene &scene)
{
    VKE_LOG_INFO("CHECK SCENE {} NODE CNT {}", scene.name, scene.nodes.size())
    std::unordered_set<int> visited;
    for (int nodeID : scene.nodes)
        if (visited.find(nodeID) == visited.end() && !checkNode(model, nodeID, visited))
            return false;
    return true;
}

template <typename IT, typename OT, size_t OSTRIDE>
void copyGLTFBuffer(const tinygltf::Model &model,
                    const uint32_t accessorIdx,
                    vke_render::CPUBuffer<> &obuffer,
                    const size_t offset)
{
    auto &accessor = model.accessors[accessorIdx];
    auto &view = model.bufferViews[accessor.bufferView];
    auto &buffer = model.buffers[view.buffer];
    std::byte *obufferp = obuffer.data + offset;

    const unsigned char *bufferp = buffer.data.data() + view.byteOffset + accessor.byteOffset;
    const uint32_t cnt = accessor.count;
    const uint32_t componentCnt = tinygltf::GetNumComponentsInType((uint32_t)(accessor.type));

    size_t stride = view.byteStride == 0
                        ? tinygltf::GetComponentSizeInBytes((uint32_t)(accessor.componentType)) * componentCnt
                        : view.byteStride;
    for (int i = 0; i < cnt; ++i)
    {
        for (int j = 0; j < componentCnt; ++j)
            *(((OT *)obufferp) + j) = *(((IT *)bufferp) + j);
        obufferp += OSTRIDE;
        bufferp += stride;
    }
}

template <typename IT, typename OT>
void copyAttribute(const tinygltf::Model &model,
                   const tinygltf::Primitive &prim,
                   const char *name,
                   vke_render::CPUBuffer<> &obuffer,
                   const size_t offset)
{
    VKE_LOG_INFO("CPOY ATTR {}", name)
    auto it = prim.attributes.find(name);
    if (it != prim.attributes.end())
        copyGLTFBuffer<IT, OT, sizeof(vke_render::SkinVertex)>(model, it->second, obuffer, offset);
}

void processSkinNode(const tinygltf::Model &model, const tinygltf::Node &node, const std::string &opth, std::map<std::string, uint32_t> &nameMap)
{
    auto &mesh = model.meshes[node.mesh];
    size_t vertexSumSize = 0, indexSumSize = 0;
    std::vector<vke_render::MeshInfo> meshInfos(mesh.primitives.size());
    std::map<uint32_t, uint32_t> indexFirstPrim;
    int i = 0;
    for (auto &prim : mesh.primitives)
    {
        vke_render::MeshInfo &meshInfo = meshInfos[i];
        meshInfo.vertexOffset = vertexSumSize;
        meshInfo.vertexCnt = model.accessors[prim.attributes.begin()->second].count;
        meshInfo.vertexSize = sizeof(vke_render::SkinVertex) * meshInfo.vertexCnt;
        vertexSumSize += meshInfo.vertexSize;

        auto it = indexFirstPrim.find(prim.indices);

        if (it == indexFirstPrim.end())
        {
            const tinygltf::Accessor &accessor = model.accessors[prim.indices];
            meshInfo.indexCnt = accessor.count;
            meshInfo.indexOffset = indexSumSize;
            meshInfo.indexSize = tinygltf::GetComponentSizeInBytes((uint32_t)(accessor.componentType)) * accessor.count;
            indexSumSize += meshInfo.indexSize;
            indexFirstPrim[prim.indices] = i;
        }
        else
        {
            vke_render::MeshInfo &prevInfo = meshInfos[it->second];
            meshInfo.indexOffset = prevInfo.indexOffset;
            meshInfo.indexSize = prevInfo.indexSize;
            meshInfo.indexCnt = prevInfo.indexCnt;
        }

        VKE_LOG_INFO("VOFF {} VSIZ {} IOFF {} ISIZ {}", meshInfo.vertexOffset, meshInfo.vertexSize, meshInfo.indexOffset, meshInfo.indexSize)
        ++i;
    }

    vke_render::CPUBuffer<> vertices(vertexSumSize);
    vke_render::CPUBuffer<> indices(indexSumSize);

    i = 0;
    for (auto &prim : mesh.primitives)
    {
        VKE_LOG_INFO("DO COPY {}", i)
        vke_render::MeshInfo &meshInfo = meshInfos[i++];
        copyAttribute<float, float>(model, prim, "POSITION", vertices, meshInfo.vertexOffset);
        copyAttribute<float, float>(model, prim, "NORMAL", vertices, meshInfo.vertexOffset + 3 * sizeof(float));
        copyAttribute<float, float>(model, prim, "TANGENT", vertices, meshInfo.vertexOffset + 6 * sizeof(float));
        copyAttribute<float, float>(model, prim, "TEXCOORD_0", vertices, meshInfo.vertexOffset + 10 * sizeof(float));
        copyAttribute<float, float>(model, prim, "WEIGHTS_0", vertices, meshInfo.vertexOffset + 12 * sizeof(float));
        copyAttribute<uint8_t, uint32_t>(model, prim, "JOINTS_0", vertices, meshInfo.vertexOffset + 16 * sizeof(float));
        if (i > 1 && meshInfo.indexOffset < meshInfos[i - 2].indexOffset)
            continue;
        VKE_LOG_INFO("COPY INDEX0 {}", meshInfo.indexOffset)
        if (meshInfo.indexSize / meshInfo.indexCnt == 2)
            copyGLTFBuffer<uint16_t, uint16_t, sizeof(uint16_t)>(model, prim.indices, indices, meshInfo.indexOffset);
        else
            copyGLTFBuffer<uint32_t, uint32_t, sizeof(uint32_t)>(model, prim.indices, indices, meshInfo.indexOffset);
        VKE_LOG_INFO("COPY INDEX1 {}", meshInfo.indexOffset)
    }
    for (auto &info : meshInfos)
        VKE_LOG_INFO("VO {} VS {} VC {} IO {} IS {} IC {}", info.vertexOffset, info.vertexSize, info.vertexCnt, info.indexOffset, info.indexSize, info.indexCnt)

    const auto &skin = model.skins[node.skin];
    const tinygltf::Accessor &accessor = model.accessors[skin.inverseBindMatrices];
    const auto &view = model.bufferViews[accessor.bufferView];

    size_t skinSize = accessor.count * sizeof(glm::mat4);
    vke_render::CPUBuffer<> ibmBuffer(skinSize);
    copyGLTFBuffer<float, float, sizeof(glm::mat4)>(
        model, skin.inverseBindMatrices, ibmBuffer, 0);

    std::ofstream outFile(opth, std::ios::binary);

    std::vector<int> joints;

    for (auto &jointID : skin.joints)
        joints.push_back(nameMap[model.nodes[jointID].name]);

    vke_render::Mesh::MeshDataToBinary(outFile, meshInfos, vertices, indices, ibmBuffer, joints);
}

bool convertNodeToJoint(
    const tinygltf::Model &model,
    const tinygltf::Node &node,
    ozz::animation::offline::RawSkeleton::Joint *joint)
{
    joint->name = node.name.c_str();
    joint->children.resize(node.children.size());

    for (size_t i = 0; i < node.children.size(); ++i)
    {
        const tinygltf::Node &childnode = model.nodes[node.children[i]];
        ozz::animation::offline::RawSkeleton::Joint &childjoint =
            joint->children[i];

        if (!convertNodeToJoint(model, childnode, &childjoint))
        {
            return false;
        }
    }

    return true;
}

ozz::unique_ptr<ozz::animation::Skeleton> buildOzzSkeleton(const tinygltf::Model &model, const tinygltf::Node &node)
{
    ozz::animation::offline::RawSkeleton rawSkeleton;
    rawSkeleton.roots.resize(1);
    ozz::animation::offline::RawSkeleton::Joint &rootjoint = rawSkeleton.roots[0];

    if (!convertNodeToJoint(model, node, &rootjoint))
        return nullptr;

    if (!rawSkeleton.Validate())
    {
        VKE_LOG_ERROR("Output skeleton failed validation. This is likely an implementation issue.")
        return nullptr;
    }

    ozz::animation::offline::SkeletonBuilder builder;
    ozz::unique_ptr<ozz::animation::Skeleton> skeleton = builder(rawSkeleton);

    if (!skeleton)
    {
        VKE_LOG_ERROR("Failed to build runtime skeleton.")
        return nullptr;
    }

    return skeleton;
}