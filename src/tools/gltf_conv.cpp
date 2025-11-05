#include <logger.hpp>
#include <render/mesh.hpp>
#include <tinygltf/tiny_gltf.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>

static bool checkGLTFNode(const tinygltf::Model &model, const tinygltf::Node &node);
static void processGLTFNode(const tinygltf::Model &model, const tinygltf::Node &node, const std::string &opth);
static void processMaterial(const tinygltf::Model &model, const tinygltf::Node &node, nlohmann::json &assetLUT, nlohmann::json &scene, const std::filesystem::path &odir);

uint32_t maxMeshID = 2047;
uint32_t maxTextureID = 2047;
uint32_t maxMaterialID = 2047;

std::string gltfName;

static nlohmann::json loadJSON(const std::filesystem::path &pth)
{
    nlohmann::json ret;
    std::ifstream ifs(pth, std::ios::in);
    ifs >> ret;
    ifs.close();
    return ret;
}

static void saveJSON(const std::filesystem::path &pth, const nlohmann::json &json)
{
    std::ofstream ofs(pth, std::ios::out);
    ofs << json;
    ofs.close();
}

int main(int argc, char **argv)
{
    vke_common::Logger::Init();

    if (argc != 5)
        return -1;

    std::filesystem::path pth(argv[1]);
    std::filesystem::path odir(argv[2]);
    std::filesystem::path scenePth(argv[3]);
    std::filesystem::path assetPth(argv[4]);
    std::filesystem::path opth = odir / pth.stem();
    opth += std::string(".mesh");
    gltfName = pth.stem().string();

    VKE_LOG_INFO("PTH {} ODIR {} SPTH {} APTH {}", pth.string(), odir.string(), scenePth.string(), assetPth.string())

    nlohmann::json scene = loadJSON(scenePth);
    nlohmann::json assetLUT = loadJSON(assetPth);

    for (auto &asset : assetLUT)
    {
        switch (asset["type"].get<int>())
        {
        case 0: // texture
            maxTextureID = std::max(maxTextureID, asset["id"].get<uint32_t>());
            break;
        case 1: // mesh
            maxMeshID = std::max(maxMeshID, asset["id"].get<uint32_t>());
            break;
        case 4: // material
            maxMaterialID = std::max(maxMaterialID, asset["id"].get<uint32_t>());
            break;
        }
    }

    assetLUT.push_back({{"type", 1},
                        {"id", ++maxMeshID},
                        {"name", gltfName},
                        {"path", opth.string()}});

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    loader.LoadBinaryFromFile(&model, &err, &warn, pth.string().c_str());

    const tinygltf::Node &node = model.nodes[model.scenes[0].nodes[0]]; // TODO load scene
    if (!checkGLTFNode(model, node))
        throw std::runtime_error("invalid gltf format");
    processGLTFNode(model, node, opth.string());

    processMaterial(model, node, assetLUT, scene, odir);
    saveJSON(assetPth, assetLUT);
    saveJSON(scenePth, scene);
    return 0;
}

static void processMaterial(const tinygltf::Model &model, const tinygltf::Node &node, nlohmann::json &assetLUT, nlohmann::json &scene, const std::filesystem::path &odir)
{
    std::set<int> materialSet;
    std::map<int, uint32_t> textureMap;

    auto &mesh = model.meshes[node.mesh];
    for (auto &prim : mesh.primitives)
        materialSet.insert(prim.material);

    bool textureMissing = false;
    for (auto matID : materialSet)
    {
        // logMaterialInfo(model, matID);
        auto &mat = model.materials[matID];
        VKE_LOG_INFO("MAT ID {}", matID)
        auto &metallicRoughness = mat.pbrMetallicRoughness;
        auto &baseColorTexture = metallicRoughness.baseColorTexture;
        auto &metallicRoughnessTexture = metallicRoughness.metallicRoughnessTexture;
        auto &normalTexture = mat.normalTexture;
        // auto &occlusionTexture = mat.occlusionTexture;
        // auto &emissiveTexture = mat.emissiveTexture;

        if (baseColorTexture.index == -1 || metallicRoughnessTexture.index == -1 || normalTexture.index == -1)
            textureMissing = true;
        if (baseColorTexture.index != -1 && textureMap.find(baseColorTexture.index) == textureMap.end())
            textureMap[baseColorTexture.index] = 0;
        if (metallicRoughnessTexture.index != -1 && textureMap.find(metallicRoughnessTexture.index) == textureMap.end())
            textureMap[metallicRoughnessTexture.index] = 0;
        if (normalTexture.index != -1 && textureMap.find(normalTexture.index) == textureMap.end())
            textureMap[normalTexture.index] = 0;

        VKE_LOG_INFO("baseColorTexture {} {}", baseColorTexture.index, baseColorTexture.index == -1 ? "" : model.images[model.textures[baseColorTexture.index].source].uri)
        VKE_LOG_INFO("metallicRoughnessTexture {}", metallicRoughnessTexture.index)
        VKE_LOG_INFO("normalTexture {}", normalTexture.index)
        // VKE_LOG_INFO("occlusionTexture {}", occlusionTexture.index)
        // VKE_LOG_INFO("emissiveTexture {}", emissiveTexture.index)
    }

    auto texArr = nlohmann::json::array();
    if (textureMissing)
        texArr.push_back(1);
    uint32_t textureOffset = textureMissing ? 1 : 0;
    for (auto &[texID, texAssetID] : textureMap)
    {
        texAssetID = textureOffset++;
        std::filesystem::path oTexPath = odir / model.images[model.textures[texID].source].uri;

        assetLUT.push_back({{"type", 0},
                            {"id", ++maxTextureID},
                            {"name", gltfName + std::string("_Tex_") + std::to_string(maxTextureID)},
                            {"path", oTexPath.string()}});
        texArr.push_back(maxTextureID);
    }

    if (textureMissing)
        textureMap[-1] = 0;

    assetLUT.push_back({{"type", 4},
                        {"id", ++maxMaterialID},
                        {"name", gltfName + std::string("_Mat_") + std::to_string(maxMaterialID)},
                        {"path", ""},
                        {"shader", 3},
                        {"textures", texArr},
                        {"bindingInfos", nlohmann::json::array({{{"binding", 0},
                                                                 {"offset", 0},
                                                                 {"cnt", texArr.size()}}})}});

    uint32_t maxID = scene["maxid"];
    auto textureIndices = nlohmann::json::array();

    for (auto &prim : mesh.primitives)
    {
        auto &mat = model.materials[prim.material];
        auto &metallicRoughness = mat.pbrMetallicRoughness;
        auto &baseColorTexture = metallicRoughness.baseColorTexture;
        auto &metallicRoughnessTexture = metallicRoughness.metallicRoughnessTexture;
        auto &normalTexture = mat.normalTexture;

        textureIndices.push_back({textureMap[baseColorTexture.index],
                                  textureMap[metallicRoughnessTexture.index],
                                  textureMap[normalTexture.index]});
    }

    nlohmann::json gameObject = {
        {"id", maxID},
        {"static", true},
        {"name", gltfName},
        {"layer", 0},
        {"parent", 0},
        {"transform",
         {{"pos", nlohmann::json::array({0.0, 0.0, 0.0})},
          {"scl", nlohmann::json::array({1.0, 1.0, 1.0})},
          {"rot", nlohmann::json::array({0.0, 0.0, 0.0, 1.0})}}},
        {"components", nlohmann::json::array({{{"type", "renderableObject"},
                                               {"material", maxMaterialID},
                                               {"mesh", maxMeshID},
                                               {"textureIndices", textureIndices}}})},
        {"children", nlohmann::json::array()}};
    scene["maxid"] = maxID + 1;
    scene["objects"].push_back(gameObject);
}

static bool checkAttribute(const std::string &&name,
                           const tinygltf::Model &model,
                           const tinygltf::Primitive &prim,
                           const int type, const int ctype)
{
    auto it = prim.attributes.find(name);
    if (it == prim.attributes.end())
        return true;
    const tinygltf::Accessor &accessor = model.accessors[it->second];
    if (accessor.type != type || accessor.componentType != ctype)
        return false;
    return true;
}

static bool checkGLTFNode(const tinygltf::Model &model, const tinygltf::Node &node)
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

    for (auto &childID : node.children)
        if (!checkGLTFNode(model, model.nodes[childID]))
            return false;
    return true;
}

template <typename T, size_t OSTRIDE>
static void copyGLTFBuffer(const tinygltf::Model &model,
                           const uint32_t accessorIdx,
                           vke_render::CPUBuffer &obuffer,
                           const size_t offset)
{
    auto &accessor = model.accessors[accessorIdx];
    auto &view = model.bufferViews[accessor.bufferView];
    auto &buffer = model.buffers[view.buffer];
    unsigned char *obufferp = obuffer.data + offset;

    const unsigned char *bufferp = buffer.data.data() + view.byteOffset + accessor.byteOffset;
    const uint32_t cnt = accessor.count;
    const uint32_t componentCnt = tinygltf::GetNumComponentsInType((uint32_t)(accessor.type));

    size_t stride = view.byteStride == 0
                        ? tinygltf::GetComponentSizeInBytes((uint32_t)(accessor.componentType)) * componentCnt
                        : view.byteStride;
    for (int i = 0; i < cnt; ++i)
    {
        for (int j = 0; j < componentCnt; ++j)
            *(((T *)obufferp) + j) = *(((T *)bufferp) + j);
        obufferp += OSTRIDE;
        bufferp += stride;
    }
}

static void copyAttribute(const tinygltf::Model &model,
                          const tinygltf::Primitive &prim,
                          const std::string &&name,
                          vke_render::CPUBuffer &obuffer,
                          const size_t offset)
{
    VKE_LOG_INFO("CPOY ATTR {}", name)
    auto it = prim.attributes.find(name);
    if (it != prim.attributes.end())
        copyGLTFBuffer<float, sizeof(vke_render::Vertex)>(model, it->second, obuffer, offset);
}

static void processGLTFNode(const tinygltf::Model &model, const tinygltf::Node &node, const std::string &opth)
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

        meshInfo.vertexSize = sizeof(vke_render::Vertex) * model.accessors[prim.attributes.begin()->second].count;
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

    vke_render::CPUBuffer vertices(vertexSumSize);
    vke_render::CPUBuffer indices(indexSumSize);

    i = 0;
    for (auto &prim : mesh.primitives)
    {
        VKE_LOG_INFO("DO COPY {}", i)
        vke_render::MeshInfo &meshInfo = meshInfos[i++];
        copyAttribute(model, prim, "POSITION", vertices, meshInfo.vertexOffset);
        copyAttribute(model, prim, "NORMAL", vertices, meshInfo.vertexOffset + 3 * sizeof(float));
        copyAttribute(model, prim, "TANGENT", vertices, meshInfo.vertexOffset + 6 * sizeof(float));
        copyAttribute(model, prim, "TEXCOORD_0", vertices, meshInfo.vertexOffset + 10 * sizeof(float));
        if (i > 1 && meshInfo.indexOffset < meshInfos[i - 2].indexOffset)
            continue;
        VKE_LOG_INFO("COPY INDEX0 {}", meshInfo.indexOffset)
        if (meshInfo.indexSize / meshInfo.indexCnt == 2)
            copyGLTFBuffer<unsigned short, sizeof(unsigned short)>(model, prim.indices, indices, meshInfo.indexOffset);
        else
            copyGLTFBuffer<unsigned int, sizeof(unsigned int)>(model, prim.indices, indices, meshInfo.indexOffset);
        VKE_LOG_INFO("COPY INDEX1 {}", meshInfo.indexOffset)
    }

    for (auto &info : meshInfos)
        VKE_LOG_INFO("VO {} VS {} IO {} IS {} IC {}", info.vertexOffset, info.vertexSize, info.indexOffset, info.indexSize, info.indexCnt)

    std::ofstream outFile(opth, std::ios::binary);
    vke_render::Mesh::MeshDataToBinary(outFile, meshInfos, vertices, indices);
}