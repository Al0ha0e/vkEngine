#include <common.hpp>
#include <render/mesh.hpp>
#include <tinygltf/tiny_gltf.h>
#include <nlohmann/json.hpp>

#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/motion_extractor.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/raw_track.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/track_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/track.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/memory/unique_ptr.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <string>
#include <system_error>
#include <vector>

// ---------------------------------------------------------------------------
// Command-line arguments
// ---------------------------------------------------------------------------
struct Args
{
    std::filesystem::path input;
    std::filesystem::path output;
    bool anim = false;
    bool animAll = false;
    bool rootMotion = false;
    std::string rootJointName;
};

static bool parseArgs(int argc, char **argv, Args &args)
{
    if (argc < 3)
        return false;

    args.input = argv[1];
    args.output = argv[2];

    for (int i = 3; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--anim")
            args.anim = true;
        else if (arg == "--all")
            args.animAll = true;
        else if (arg == "--root-motion")
            args.rootMotion = true;
        else if (arg == "--root-joint" && i + 1 < argc)
            args.rootJointName = argv[++i];
        else
            return false;
    }

    if (args.animAll)
        args.anim = true;

    return true;
}

static void usage()
{
    VKE_LOG_ERROR(
        "Usage: gltf_skin_conv <input.glb> <output_dir> [options]\n"
        "  Always extracts: <stem>.mesh, <stem>_skeleton.ozz\n"
        "  Options:\n"
        "    --anim               Extract first animation\n"
        "    --all                Extract all animations (implies --anim)\n"
        "    --root-motion        Extract root motion tracks\n"
        "    --root-joint <name>  Root joint name for root-motion extraction")
}

// ---------------------------------------------------------------------------
// GLTF scene validation
// ---------------------------------------------------------------------------
static bool checkAttribute(const char *name,
                           const tinygltf::Model &model,
                           const tinygltf::Primitive &prim,
                           int type, int ctype)
{
    auto it = prim.attributes.find(name);
    if (it == prim.attributes.end())
        return true;
    VKE_LOG_INFO("CHECK ATTR {}", name)
    const tinygltf::Accessor &accessor = model.accessors[it->second];
    return accessor.type == type && accessor.componentType == ctype;
}

static bool checkMesh(const tinygltf::Model &model, const tinygltf::Node &node)
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

static bool checkSkin(const tinygltf::Model &model, int meshID)
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

static bool checkSkinNode(const tinygltf::Model &model, int nodeID, std::unordered_set<int> &visited)
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

static bool checkNode(const tinygltf::Model &model, int nodeID, std::unordered_set<int> &visited)
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

static bool checkScene(const tinygltf::Model &model, const tinygltf::Scene &scene)
{
    VKE_LOG_INFO("CHECK SCENE {} NODE CNT {}", scene.name, scene.nodes.size())
    std::unordered_set<int> visited;
    for (int nodeID : scene.nodes)
        if (visited.find(nodeID) == visited.end() && !checkNode(model, nodeID, visited))
            return false;
    return true;
}

// ---------------------------------------------------------------------------
// GLTF buffer helpers
// ---------------------------------------------------------------------------
template <typename IT, typename OT, size_t OSTRIDE>
static void copyGLTFBuffer(const tinygltf::Model &model,
                           uint32_t accessorIdx,
                           vke_render::CPUBuffer<> &obuffer,
                           size_t offset)
{
    auto &accessor = model.accessors[accessorIdx];
    auto &view = model.bufferViews[accessor.bufferView];
    auto &buffer = model.buffers[view.buffer];
    std::byte *obufferp = obuffer.data + offset;

    const unsigned char *bufferp = buffer.data.data() + view.byteOffset + accessor.byteOffset;
    uint32_t cnt = accessor.count;
    uint32_t componentCnt = tinygltf::GetNumComponentsInType(uint32_t(accessor.type));

    size_t stride = view.byteStride == 0
                        ? tinygltf::GetComponentSizeInBytes(uint32_t(accessor.componentType)) * componentCnt
                        : view.byteStride;
    for (uint32_t i = 0; i < cnt; ++i)
    {
        for (uint32_t j = 0; j < componentCnt; ++j)
            *(((OT *)obufferp) + j) = *(((IT *)bufferp) + j);
        obufferp += OSTRIDE;
        bufferp += stride;
    }
}

template <typename IT, typename OT>
static void copyAttribute(const tinygltf::Model &model,
                          const tinygltf::Primitive &prim,
                          const char *name,
                          vke_render::CPUBuffer<> &obuffer,
                          size_t offset)
{
    VKE_LOG_INFO("CPOY ATTR {}", name)
    auto it = prim.attributes.find(name);
    if (it != prim.attributes.end())
        copyGLTFBuffer<IT, OT, sizeof(vke_render::SkinVertex)>(model, it->second, obuffer, offset);
}

// ---------------------------------------------------------------------------
// Skinned-mesh processing
// ---------------------------------------------------------------------------
static void processSkinNode(const tinygltf::Model &model, const tinygltf::Node &node,
                            const std::string &opth,
                            std::map<std::string, uint32_t> &nameMap)
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
            meshInfo.indexSize = tinygltf::GetComponentSizeInBytes(uint32_t(accessor.componentType)) * accessor.count;
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

        VKE_LOG_INFO("VOFF {} VSIZ {} IOFF {} ISIZ {}",
                     meshInfo.vertexOffset, meshInfo.vertexSize,
                     meshInfo.indexOffset, meshInfo.indexSize)
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
        VKE_LOG_INFO("VO {} VS {} VC {} IO {} IS {} IC {}",
                     info.vertexOffset, info.vertexSize, info.vertexCnt,
                     info.indexOffset, info.indexSize, info.indexCnt)

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

// ---------------------------------------------------------------------------
// OZZ skeleton helpers
// ---------------------------------------------------------------------------
static bool convertNodeToJoint(
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
            return false;
    }

    return true;
}

static ozz::unique_ptr<ozz::animation::Skeleton> buildOzzSkeleton(
    const tinygltf::Model &model, const tinygltf::Node &node)
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

static void saveOzzSkeleton(const std::filesystem::path &output,
                            const ozz::animation::Skeleton &skeleton)
{
    ozz::io::File file(output.string().c_str(), "wb");
    VKE_FATAL_IF(!file.opened(), "Failed to open output skeleton file {}", output.string())
    ozz::io::OArchive archive(&file);
    archive << skeleton;
    VKE_LOG_INFO("Wrote skeleton {}", output.string())
}

static std::unordered_map<std::string, int> buildJointMap(
    const ozz::animation::Skeleton &skeleton)
{
    std::unordered_map<std::string, int> joints;
    const auto names = skeleton.joint_names();
    for (int i = 0; i < names.size(); ++i)
        joints.emplace(std::string(names[i]), i);
    return joints;
}

// ---------------------------------------------------------------------------
// Animation extraction
// ---------------------------------------------------------------------------
static std::string sanitizeFileName(const std::string &name, size_t fallbackIndex)
{
    std::string sanitized;
    sanitized.reserve(name.size());

    for (char c : name)
    {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || c == '-' || c == '_')
            sanitized.push_back(c);
        else if (c == ' ' || c == '.')
            sanitized.push_back('_');
    }

    if (sanitized.empty())
        sanitized = "animation_" + std::to_string(fallbackIndex);
    return sanitized;
}

static std::filesystem::path outputPathForAnimation(
    const std::filesystem::path &outputDirectory,
    const tinygltf::Animation &animation,
    size_t animationIndex,
    std::unordered_map<std::string, size_t> &nameCounts)
{
    const std::string baseName = sanitizeFileName(animation.name, animationIndex);
    size_t &count = nameCounts[baseName];
    const std::string fileName = count == 0
                                     ? baseName + ".anim"
                                     : baseName + "_" + std::to_string(count) + ".anim";
    ++count;
    return outputDirectory / fileName;
}

static const unsigned char *accessorData(const tinygltf::Model &model,
                                         int accessorIndex, size_t &stride)
{
    const tinygltf::Accessor &accessor = model.accessors[accessorIndex];
    VKE_FATAL_IF(accessor.sparse.isSparse, "Sparse animation accessors are not supported yet.")
    VKE_FATAL_IF(accessor.bufferView < 0, "Animation accessor has no buffer view.")

    const tinygltf::BufferView &view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer &buffer = model.buffers[view.buffer];
    const int componentCount = tinygltf::GetNumComponentsInType(accessor.type);
    const int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    stride = view.byteStride == 0 ? componentCount * componentSize : view.byteStride;
    return buffer.data.data() + view.byteOffset + accessor.byteOffset;
}

static std::vector<float> readFloatAccessor(const tinygltf::Model &model,
                                            int accessorIndex, int expectedType)
{
    const tinygltf::Accessor &accessor = model.accessors[accessorIndex];
    VKE_FATAL_IF(accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT, "Animation accessor must contain floats.")
    VKE_FATAL_IF(accessor.type != expectedType, "Animation accessor has unexpected component count.")

    const int componentCount = tinygltf::GetNumComponentsInType(accessor.type);
    size_t stride = 0;
    const unsigned char *data = accessorData(model, accessorIndex, stride);

    std::vector<float> out(accessor.count * componentCount);
    for (size_t i = 0; i < accessor.count; ++i)
    {
        const float *src = reinterpret_cast<const float *>(data + i * stride);
        for (int c = 0; c < componentCount; ++c)
            out[i * componentCount + c] = src[c];
    }
    return out;
}

static ozz::animation::offline::RawAnimation buildRawAnimation(
    const tinygltf::Model &model,
    const tinygltf::Animation &gltfAnimation,
    const ozz::animation::Skeleton &skeleton,
    const std::unordered_map<std::string, int> &joints)
{
    ozz::animation::offline::RawAnimation raw;
    raw.name = gltfAnimation.name.c_str();
    raw.tracks.resize(skeleton.num_joints());
    raw.duration = 0.0f;

    for (const tinygltf::AnimationChannel &channel : gltfAnimation.channels)
    {
        const tinygltf::Node &targetNode = model.nodes[channel.target_node];
        auto jointIt = joints.find(targetNode.name);
        if (jointIt == joints.end())
        {
            VKE_LOG_INFO("Skipping animation channel for non-skeleton node {}", targetNode.name)
            continue;
        }

        const tinygltf::AnimationSampler &sampler = gltfAnimation.samplers[channel.sampler];
        const std::vector<float> times = readFloatAccessor(model, sampler.input, TINYGLTF_TYPE_SCALAR);
        if (!times.empty())
            raw.duration = std::max(raw.duration, times.back());
        ozz::animation::offline::RawAnimation::JointTrack &track = raw.tracks[jointIt->second];

        if (channel.target_path == "translation")
        {
            const std::vector<float> values = readFloatAccessor(model, sampler.output, TINYGLTF_TYPE_VEC3);
            VKE_FATAL_IF(values.size() != times.size() * 3, "Translation key count mismatch.")
            for (size_t i = 0; i < times.size(); ++i)
                track.translations.push_back({times[i], {values[i * 3 + 0], values[i * 3 + 1], values[i * 3 + 2]}});
        }
        else if (channel.target_path == "rotation")
        {
            const std::vector<float> values = readFloatAccessor(model, sampler.output, TINYGLTF_TYPE_VEC4);
            VKE_FATAL_IF(values.size() != times.size() * 4, "Rotation key count mismatch.")
            for (size_t i = 0; i < times.size(); ++i)
                track.rotations.push_back({times[i], {values[i * 4 + 0], values[i * 4 + 1], values[i * 4 + 2], values[i * 4 + 3]}});
        }
        else if (channel.target_path == "scale")
        {
            const std::vector<float> values = readFloatAccessor(model, sampler.output, TINYGLTF_TYPE_VEC3);
            VKE_FATAL_IF(values.size() != times.size() * 3, "Scale key count mismatch.")
            for (size_t i = 0; i < times.size(); ++i)
                track.scales.push_back({times[i], {values[i * 3 + 0], values[i * 3 + 1], values[i * 3 + 2]}});
        }
        else
        {
            VKE_LOG_INFO("Skipping unsupported animation target path {}", channel.target_path)
        }
    }

    VKE_FATAL_IF(raw.duration <= 0.0f, "Animation duration must be positive.")
    VKE_FATAL_IF(!raw.Validate(), "Raw animation failed validation.")
    return raw;
}

static int rootJointIndex(const ozz::animation::Skeleton &skeleton,
                          const std::string &rootJointName)
{
    if (rootJointName.empty())
        return 0;

    const auto names = skeleton.joint_names();
    for (int i = 0; i < names.size(); ++i)
    {
        if (rootJointName == names[i])
            return i;
    }
    VKE_FATAL("Root joint {} does not exist in skeleton.", rootJointName)
}

static void saveAnimation(
    const std::filesystem::path &output,
    const ozz::animation::offline::RawAnimation &raw,
    const ozz::animation::Skeleton &skeleton,
    bool rootMotion,
    int rootJoint)
{
    ozz::animation::offline::RawAnimation rawForBuild;
    const ozz::animation::offline::RawAnimation *animationInput = &raw;
    ozz::animation::offline::RawFloat3Track rawMotionPosition;
    ozz::animation::offline::RawQuaternionTrack rawMotionRotation;

    if (rootMotion)
    {
        ozz::animation::offline::MotionExtractor extractor;
        extractor.root_joint = rootJoint;
        VKE_FATAL_IF(!extractor(raw, skeleton, &rawMotionPosition, &rawMotionRotation, &rawForBuild),
                     "Failed to extract root motion.")
        animationInput = &rawForBuild;
    }

    ozz::animation::offline::AnimationBuilder animationBuilder;
    ozz::unique_ptr<ozz::animation::Animation> animation = animationBuilder(*animationInput);
    VKE_FATAL_IF(!animation, "Failed to build runtime animation.")

    ozz::io::File file(output.string().c_str(), "wb");
    VKE_FATAL_IF(!file.opened(), "Failed to open output animation file {}", output.string())
    ozz::io::OArchive archive(&file);
    archive << *animation;

    if (rootMotion)
    {
        ozz::animation::offline::TrackBuilder trackBuilder;
        ozz::unique_ptr<ozz::animation::Float3Track> motionPosition = trackBuilder(rawMotionPosition);
        ozz::unique_ptr<ozz::animation::QuaternionTrack> motionRotation = trackBuilder(rawMotionRotation);
        VKE_FATAL_IF(!motionPosition || !motionRotation, "Failed to build root motion tracks.")
        archive << *motionPosition;
        archive << *motionRotation;
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char **argv)
{
    Args args;
    if (!parseArgs(argc, argv, args))
    {
        usage();
        return -1;
    }

    VKE_LOG_INFO("PTH {} ODIR {}", args.input.string(), args.output.string())

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    if (!loader.LoadBinaryFromFile(&model, &err, &warn, args.input.string().c_str()))
    {
        VKE_LOG_ERROR("Failed to load GLTF {}: {}", args.input.string(), err)
        return -1;
    }
    if (!warn.empty())
        VKE_LOG_INFO("GLTF warning: {}", warn)

    VKE_FATAL_IF(!checkScene(model, model.scenes[0]), "Invalid gltf format!")
    VKE_LOG_INFO("GLTF CHECK OK")

    auto &root = model.nodes[model.scenes[0].nodes[0]];

    // Output paths
    std::filesystem::path meshPath = args.output / args.input.stem();
    meshPath += ".mesh";
    std::filesystem::path skeletonPath = args.output / (args.input.stem().string() + "_skeleton.ozz");

    // Build and save skeleton
    ozz::unique_ptr<ozz::animation::Skeleton> skeleton = buildOzzSkeleton(model, root);
    VKE_FATAL_IF(!skeleton, "Failed to build skeleton.")
    saveOzzSkeleton(skeletonPath, *skeleton);

    // Build joint name → index map
    const auto &names = skeleton->joint_names();
    std::map<std::string, uint32_t> nameMap;
    for (int i = 0; i < names.size(); ++i)
        nameMap[std::string(names[i])] = i;

    for (auto &[k, v] : nameMap)
        VKE_LOG_INFO("K {} V {}", k, v)

    // Process skinned mesh
    const tinygltf::Node &node = model.nodes[root.children[0]];
    processSkinNode(model, node, meshPath.string(), nameMap);
    VKE_LOG_INFO("Wrote mesh {}", meshPath.string())

    // Extract animations (optional)
    if (args.anim)
    {
        VKE_FATAL_IF(model.animations.empty(), "GLTF has no animations.")
        const auto joints = buildJointMap(*skeleton);
        const int rootJnt = rootJointIndex(*skeleton, args.rootJointName);

        if (args.animAll)
        {
            std::error_code ec;
            std::filesystem::create_directories(args.output, ec);
            VKE_FATAL_IF(ec, "Failed to create output directory {}: {}", args.output.string(), ec.message())

            std::unordered_map<std::string, size_t> nameCounts;
            for (size_t i = 0; i < model.animations.size(); ++i)
            {
                const tinygltf::Animation &gltfAnimation = model.animations[i];
                const ozz::animation::offline::RawAnimation raw =
                    buildRawAnimation(model, gltfAnimation, *skeleton, joints);
                const std::filesystem::path output =
                    outputPathForAnimation(args.output, gltfAnimation, i, nameCounts);
                saveAnimation(output, raw, *skeleton, args.rootMotion, rootJnt);
                VKE_LOG_INFO("Wrote animation {} rootMotion={}", output.string(), args.rootMotion)
            }
        }
        else
        {
            const ozz::animation::offline::RawAnimation raw =
                buildRawAnimation(model, model.animations[0], *skeleton, joints);
            const std::string animName = sanitizeFileName(model.animations[0].name, 0);
            const std::filesystem::path animPath = args.output / (animName + ".anim");
            saveAnimation(animPath, raw, *skeleton, args.rootMotion, rootJnt);
            VKE_LOG_INFO("Wrote animation {} rootMotion={}", animPath.string(), args.rootMotion)
        }
    }

    return 0;
}
