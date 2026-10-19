#include <ui/ui.hpp>
#include <ui/text.hpp>
#include <ui/image.hpp>

namespace vke_ui
{
    vke_common::SceneResult<void> UINodeData::ValidateJSON(const nlohmann::json &json)
    {
        using namespace vke_common::json_validation;
        if (auto valid = Object(json).Require({"type"}).Strings({"type"}).Result(); !valid) return valid;
        if (json["type"] == "uiText") return UITextData::ValidateJSON(json);
        if (json["type"] == "uiImage") return UIImageData::ValidateJSON(json);
        if (json["type"] == "uiContainer") return UIContainerData::ValidateJSON(json);
        return std::unexpected("unknown UI node type");
    }

    std::unique_ptr<UINodeData> UINodeData::FromJSON(const nlohmann::json &json)
    {
        if (json["type"] == "uiText") return std::make_unique<UITextData>(json);
        if (json["type"] == "uiImage") return std::make_unique<UIImageData>(json);
        return std::make_unique<UIContainerData>(json);
    }
}
