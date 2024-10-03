#include <asset.hpp>

namespace vke_common
{
    const std::string BuiltinSkyboxTexPath = std::string(REL_DIR) + "/builtin_assets/texture/skybox.png";
    const std::string BuiltinCubePath = std::string(REL_DIR) + "/builtin_assets/mesh/cube.obj";
    const std::string BuiltinSpherePath = std::string(REL_DIR) + "/builtin_assets/mesh/sphere.obj";
    // const std::string BuiltinCapsulePath("@cp");
    const std::string BuiltinCylinderPath = std::string(REL_DIR) + "/builtin_assets/mesh/cylinder.obj";
    const std::string BuiltinMonkeyPath = std::string(REL_DIR) + "/builtin_assets/mesh/monkey.obj";
    const std::string BuiltinSkyVertShaderPath = std::string(REL_DIR) + "/builtin_assets/shader/skyvert.spv";
    const std::string BuiltinSkyFragShaderPath = std::string(REL_DIR) + "/builtin_assets/shader/skyfrag.spv";

    extern const std::vector<vke_render::Vertex> planeVertices = {
        vke_render::Vertex(glm::vec3(1, 0, 1), glm::vec3(0, 1, 0), glm::vec2(1, 1)),
        vke_render::Vertex(glm::vec3(-1, 0, 1), glm::vec3(0, 1, 0), glm::vec2(1, 0)),
        vke_render::Vertex(glm::vec3(-1, 0, -1), glm::vec3(0, 1, 0), glm::vec2(0, 0)),
        vke_render::Vertex(glm::vec3(1, 0, -1), glm::vec3(0, 1, 0), glm::vec2(0, 1))};

    extern const std::vector<uint32_t> planeIndices = {0, 1, 2, 0, 2, 3};
}