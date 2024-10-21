#include <asset.hpp>

namespace vke_common
{
    const std::string BuiltinAssetLUTPath = std::string(REL_DIR) + "/builtin_assets/builtin_assets.json";

    extern const std::vector<vke_render::Vertex> planeVertices = {
        vke_render::Vertex(glm::vec3(1, 0, 1), glm::vec3(0, 1, 0), glm::vec2(1, 1)),
        vke_render::Vertex(glm::vec3(-1, 0, 1), glm::vec3(0, 1, 0), glm::vec2(1, 0)),
        vke_render::Vertex(glm::vec3(-1, 0, -1), glm::vec3(0, 1, 0), glm::vec2(0, 0)),
        vke_render::Vertex(glm::vec3(1, 0, -1), glm::vec3(0, 1, 0), glm::vec2(0, 1))};

    extern const std::vector<uint32_t> planeIndices = {0, 1, 2, 0, 2, 3};
}