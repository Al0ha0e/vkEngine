#include <resource.hpp>

namespace vke_common
{
    const std::string BuiltinPlanePath("@p");
    const std::string BuiltinCubePath("@c");
    const std::string BuiltinSpherePath("@s");
    // const std::string BuiltinCapsulePath("@cp");
    const std::string BuiltinCylinderPath("@cy");
    const std::string BuiltinMonkeyPath("@m");

    extern const std::vector<vke_render::Vertex> planeVertices = {
        vke_render::Vertex(glm::vec3(1, 0, 1), glm::vec3(0, 1, 0), glm::vec2(1, 1)),
        vke_render::Vertex(glm::vec3(-1, 0, 1), glm::vec3(0, 1, 0), glm::vec2(1, 0)),
        vke_render::Vertex(glm::vec3(-1, 0, -1), glm::vec3(0, 1, 0), glm::vec2(0, 0)),
        vke_render::Vertex(glm::vec3(1, 0, -1), glm::vec3(0, 1, 0), glm::vec2(0, 1))};

    extern const std::vector<uint32_t> planeIndices = {0, 1, 2, 0, 2, 3};
}