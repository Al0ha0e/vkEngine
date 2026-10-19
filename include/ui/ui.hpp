#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include <ui/rect_transform.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace vke_component
{
    class Canvas;
}

namespace vke_ui
{
    using UIHandle = uint64_t;

    enum class UIType : int32_t
    {
        Container = 1,
        Text = 2
    };

    struct UINodeData
    {
        std::string name;
        RectTransform rectTransform;
        std::vector<std::unique_ptr<UINodeData>> children;

        virtual ~UINodeData() = default;
        virtual UIType GetType() const = 0;
        virtual std::unique_ptr<UINodeData> Clone() const = 0;

        static vke_common::SceneResult<void> ValidateJSON(const nlohmann::json &json);
        static std::unique_ptr<UINodeData> FromJSON(const nlohmann::json &json);

        virtual vke_common::SceneResult<void> Validate() const
        {
            if (auto valid = rectTransform.Validate(); !valid)
                return valid;
            for (const auto &child : children)
            {
                if (!child)
                    return std::unexpected("UI child must not be null");
                if (auto valid = child->Validate(); !valid)
                    return valid;
            }
            return {};
        }

        virtual vke_common::SceneResult<void> ValidateAssets() const
        {
            for (const auto &child : children)
                if (auto valid = child->ValidateAssets(); !valid)
                    return valid;
            return {};
        }

        virtual vke_common::SceneResult<void> LoadAssets()
        {
            for (auto &child : children)
                if (auto loaded = child->LoadAssets(); !loaded)
                    return loaded;
            return {};
        }

        virtual nlohmann::json ToJSON() const
        {
            nlohmann::json json{{"name", name}, {"rectTransform", rectTransform.ToJSON()}, {"children", nlohmann::json::array()}};
            for (const auto &child : children)
                json["children"].push_back(child->ToJSON());
            return json;
        }

        static vke_common::SceneResult<void> ValidateChildrenJSON(const nlohmann::json &json)
        {
            if (!json.contains("children"))
                return {};
            if (!json["children"].is_array())
                return std::unexpected("children must be an array");
            for (const auto &child : json["children"])
                if (auto valid = ValidateJSON(child); !valid)
                    return valid;
            return {};
        }

    protected:
        static vke_common::SceneResult<void> validateCommonJSON(const nlohmann::json &json)
        {
            if (auto valid = vke_common::json_validation::Object(json).Strings({"name"}).Result(); !valid)
                return valid;
            if (json.contains("rectTransform"))
                if (auto valid = RectTransform::ValidateJSON(json["rectTransform"]); !valid)
                    return valid;
            return ValidateChildrenJSON(json);
        }

        UINodeData() = default;
        explicit UINodeData(const nlohmann::json &json) : name(json.value("name", std::string()))
        {
            if (json.contains("rectTransform"))
                rectTransform = RectTransform(json["rectTransform"]);
            if (json.contains("children"))
                for (const auto &child : json["children"])
                    children.push_back(FromJSON(child));
        }

        UINodeData(const UINodeData &other) { *this = other; }
        UINodeData &operator=(const UINodeData &other)
        {
            if (this == &other)
                return *this;
            std::vector<std::unique_ptr<UINodeData>> copiedChildren;
            copiedChildren.reserve(other.children.size());
            for (const auto &child : other.children)
                copiedChildren.push_back(child ? child->Clone() : nullptr);
            name = other.name;
            rectTransform = other.rectTransform;
            children = std::move(copiedChildren);
            return *this;
        }
        UINodeData(UINodeData &&) noexcept = default;
        UINodeData &operator=(UINodeData &&) noexcept = default;
    };

    class UIComponent
    {
    public:
        std::string name;
        UIHandle handle = 0;
        UIComponent *parent = nullptr;
        std::vector<std::unique_ptr<UIComponent>> children;
        RectTransform rectTransform;
        glm::vec2 resolvedSize{0.0f};

        virtual ~UIComponent() = default;
        UIComponent(const UIComponent &) = delete;
        UIComponent &operator=(const UIComponent &) = delete;

        virtual UIType GetType() const = 0;
        vke_common::SceneResult<void> SetRectTransform(const RectTransform &value)
        {
            if (auto valid = value.Validate(); !valid)
                return valid;
            rectTransform = value;
            layoutDirty = true;
            return {};
        }
        virtual std::unique_ptr<UINodeData> ToData() const = 0;

    protected:
        explicit UIComponent(const UINodeData &data) : name(data.name), rectTransform(data.rectTransform) {}
        void fillData(UINodeData &data) const
        {
            data.name = name;
            data.rectTransform = rectTransform;
            for (const auto &child : children)
                data.children.push_back(child->ToData());
        }
        virtual void unload() {}
        virtual void updateRenderTransform(const glm::mat4 &matrix, float order) {}

    private:
        friend class vke_component::Canvas;
        bool layoutDirty = true;
        glm::mat4 layoutMatrix{1.0f};
    };

    struct UIContainerData final : UINodeData
    {
        UIContainerData() = default;
        explicit UIContainerData(const nlohmann::json &json) : UINodeData(json) {}

        static vke_common::SceneResult<void> ValidateJSON(const nlohmann::json &json)
        {
            return validateCommonJSON(json);
        }

        UIType GetType() const override { return UIType::Container; }
        std::unique_ptr<UINodeData> Clone() const override { return std::make_unique<UIContainerData>(*this); }

        nlohmann::json ToJSON() const override
        {
            auto json = UINodeData::ToJSON();
            json["type"] = "uiContainer";
            return json;
        }
    };

    class UIContainer final : public UIComponent
    {
    public:
        explicit UIContainer(const UIContainerData &data) : UIComponent(data) {}
        UIType GetType() const override { return UIType::Container; }
        std::unique_ptr<UINodeData> ToData() const override
        {
            auto data = std::make_unique<UIContainerData>();
            fillData(*data);
            return data;
        }
    };
}

#endif
