#include <component/canvas.hpp>
#include <script.hpp>

namespace vke_component
{
    void Canvas::unloadSubtree(vke_ui::UIComponent &node)
    {
        for (auto &child : node.children) unloadSubtree(*child);
        node.unload();
        nodes.erase(node.handle);
        vke_common::ScriptManager::GetUIFunctions().unregisterNodeCallbacks(node.handle);
    }
}

namespace vke_ui
{
    vke_common::SceneResult<void> UINodeData::ValidateJSON(const nlohmann::json &json)
    {
        using namespace vke_common::json_validation;
        if (auto valid = Object(json).Require({"type"}).Strings({"type"}).Result(); !valid) return valid;
        if (json["type"] == "uiText") return UITextData::ValidateJSON(json);
        if (json["type"] == "uiImage") return UIImageData::ValidateJSON(json);
        if (json["type"] == "uiButton") return UIButtonData::ValidateJSON(json);
        if (json["type"] == "uiContainer") return UIContainerData::ValidateJSON(json);
        return std::unexpected("unknown UI node type");
    }

    std::unique_ptr<UINodeData> UINodeData::FromJSON(const nlohmann::json &json)
    {
        if (json["type"] == "uiText") return std::make_unique<UITextData>(json);
        if (json["type"] == "uiImage") return std::make_unique<UIImageData>(json);
        if (json["type"] == "uiButton") return std::make_unique<UIButtonData>(json);
        return std::make_unique<UIContainerData>(json);
    }
}
