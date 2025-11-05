#include <logger.hpp>
#include <render/mesh.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <fstream>

static void processNode(aiNode *node, const aiScene *scene,
                        std::vector<vke_render::Vertex> &vertices, std::vector<uint32_t> &indices, std::vector<vke_render::MeshInfo> &infos);

int main(int argc, char **argv)
{
    vke_common::Logger::Init();

    if (argc != 3)
        return -1;

    std::string pth(argv[1]);
    std::string opth(argv[2]);

    VKE_LOG_INFO("PTH {} OPTH {}", pth, opth)

    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(pth,
                                             aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenNormals | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        VKE_LOG_ERROR("ERROR::ASSIMP::{}", importer.GetErrorString())
        return -1;
    }

    std::vector<vke_render::Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<vke_render::MeshInfo> infos;

    processNode(scene->mRootNode, scene, vertices, indices, infos);

    VKE_LOG_INFO("VCNT {} ICNT {} INFOCNT {}", vertices.size(), indices.size(), infos.size())
    for (auto &info : infos)
        VKE_LOG_INFO("VO {} VS {} IO {} IS {} IC {}", info.vertexOffset, info.vertexSize, info.indexOffset, info.indexSize, info.indexCnt)

    std::ofstream outFile(opth, std::ios::binary);
    vke_render::Mesh::MeshDataToBinary(outFile, infos, vertices, indices);

    return 0;
}

static void processMesh(aiMesh *mesh, const aiScene *scene,
                        std::vector<vke_render::Vertex> &vertices, std::vector<uint32_t> &indices,
                        std::vector<vke_render::MeshInfo> &infos)
{
    vke_render::MeshInfo info;
    info.vertexOffset = vertices.size() * sizeof(vke_render::Vertex);
    info.vertexSize = mesh->mNumVertices * sizeof(vke_render::Vertex);
    info.indexOffset = indices.size() * sizeof(uint32_t);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        glm::vec3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        glm::vec3 T = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        glm::vec3 B = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        glm::vec3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        float tw = (glm::dot(glm::cross(normal, T), B) < 0.0f) ? -1.0f : +1.0f;
        glm::vec4 tangent(T, tw);
        glm::vec2 texcoord(0, 0);
        if (mesh->mTextureCoords[0])
            texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

        vertices.emplace_back(pos, normal, tangent, texcoord);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    info.indexSize = indices.size() * sizeof(uint32_t) - info.indexOffset;
    info.indexCnt = info.indexSize / sizeof(uint32_t);

    infos.push_back(info);
}

static void processNode(aiNode *node, const aiScene *scene,
                        std::vector<vke_render::Vertex> &vertices, std::vector<uint32_t> &indices,
                        std::vector<vke_render::MeshInfo> &infos)
{
    for (int i = 0; i < node->mNumMeshes; i++)
        processMesh(scene->mMeshes[node->mMeshes[i]], scene, vertices, indices, infos);

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        processNode(node->mChildren[i], scene, vertices, indices, infos);
}