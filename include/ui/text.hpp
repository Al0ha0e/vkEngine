#ifndef UI_TEXT_COMPONENT_H
#define UI_TEXT_COMPONENT_H

#include <ui/ui_renderable.hpp>
#include <font.hpp>
#include <asset/asset_manager.hpp>
#include <asset/asset_ref.hpp>
#include <render/material.hpp>
#include <string>
#include <string_view>

namespace vke_ui
{
    struct UITextData final : UINodeData
    {
        std::string text;
        glm::vec4 color{1.0f};
        vke_common::AssetRef<vke_render::Material> material;

        static vke_common::SceneResult<void> ValidateJSON(const nlohmann::json &json)
        {
            using namespace vke_common::json_validation;
            if (auto valid = Object(json).Strings({"text"}).Unsigneds({"material"}).Vectors({"color"}, 4).Result(); !valid)
                return valid;
            return validateCommonJSON(json);
        }

        UITextData() = default;
        explicit UITextData(const nlohmann::json &json)
            : UINodeData(json), text(json.value("text", std::string())),
              material(json.value("material", vke_common::AssetHandle{0}))
        {
            if (json.contains("color"))
            {
                const auto &value = json["color"];
                color = glm::vec4(value[0].get<float>(), value[1].get<float>(),
                                  value[2].get<float>(), value[3].get<float>());
            }
        }

        UIType GetType() const override { return UIType::Text; }
        std::unique_ptr<UINodeData> Clone() const override { return std::make_unique<UITextData>(*this); }

        vke_common::SceneResult<void> ValidateAssets() const override
        {
            using vke_common::AssetManager;
            if (material.Handle() != 0 && !material.Get())
                if (auto result = AssetManager::ValidateMaterial(material.Handle()); !result)
                    return std::unexpected("material: " + result.error());
            if (auto result = AssetManager::ValidateFont(vke_common::BUILTIN_FONT_ARIAL_ID); !result)
                return result;
            return UINodeData::ValidateAssets();
        }

        vke_common::SceneResult<void> LoadAssets() override
        {
            using vke_common::AssetManager;
            if (material.Handle() != 0 && !material.Get())
            {
                material.Resolve(AssetManager::LoadMaterial(material.Handle()));
                if (!material.Get())
                    return std::unexpected("material asset " + std::to_string(material.Handle()) + ": loading failed");
            }
            if (!AssetManager::LoadFont(vke_common::BUILTIN_FONT_ARIAL_ID))
                return std::unexpected("built-in font loading failed");
            return UINodeData::LoadAssets();
        }

        nlohmann::json ToJSON() const override
        {
            auto json = UINodeData::ToJSON();
            json["type"] = "uiText";
            json["text"] = text;
            json["color"] = {color.r, color.g, color.b, color.a};
            json["material"] = material.Handle();
            return json;
        }
    };

    class UIText final : public UIRenderableComponent
    {
    public:
        explicit UIText(const UITextData &data)
            : UIRenderableComponent(data, data.material.Get()),
              text(data.text),
              font(vke_common::AssetManager::LoadFont(vke_common::BUILTIN_FONT_ARIAL_ID))
        {
            if (!rebuild(data.color))
            {
                vke_render::QuadInstanceGPU quad{};
                quad.color = data.color;
                setGeometry({quad});
                VKE_LOG_ERROR("Cannot allocate UIText geometry for {}", name);
            }
        }

        UIType GetType() const override { return UIType::Text; }
        std::unique_ptr<UINodeData> ToData() const override
        {
            auto data = std::make_unique<UITextData>();
            fillData(*data);
            data->text = text;
            data->color = GetColor();
            data->material.SetAsset(material);
            return data;
        }

        void SetText(std::string_view newText)
        {
            text.assign(newText);
            if (!rebuild(GetColor()))
                VKE_LOG_ERROR("Cannot allocate UIText geometry for {}", name);
        }

        void SetColor(const glm::vec4 &newColor)
        {
            setQuadColor(newColor);
        }

        const std::string &GetText() const { return text; }
        glm::vec4 GetColor() const
        {
            return quadIDs.empty() ? glm::vec4(1.0f)
                                   : vke_render::Renderer::GetQuadManager()->Get(quadIDs.front()).color;
        }

    private:
        std::string text;
        std::shared_ptr<vke_common::Font> font;

        bool rebuild(glm::vec4 color)
        {
            std::vector<vke_render::QuadInstanceGPU> quads;
            quads.reserve(text.size());

            int penX = 0;
            int baselineY = font->ascender;

            for (uint32_t codepoint : vke_common::Font::DecodeUTF8(text))
            {
                if (codepoint == '\n')
                {
                    penX = 0;
                    baselineY += font->lineHeight;
                    continue;
                }

                vke_common::Glyph *glyph = font->GetOrCreateGlyph(codepoint);
                if (glyph == nullptr)
                    glyph = font->GetOrCreateGlyph('?');
                if (glyph == nullptr)
                    continue;

                if (glyph->width > 0 && glyph->height > 0)
                {
                    const float minX = static_cast<float>(penX + glyph->bearingX);
                    const float minY = static_cast<float>(baselineY - glyph->bearingY);
                    const float maxX = minX + static_cast<float>(glyph->width);
                    const float maxY = minY + static_cast<float>(glyph->height);

                    vke_render::QuadInstanceGPU instance{};
                    instance.quadRect = glm::vec4(minX, minY, maxX, maxY);
                    instance.uvRect = glm::vec4(
                        static_cast<float>(glyph->atlasX), static_cast<float>(glyph->atlasY),
                        static_cast<float>(glyph->atlasX + glyph->width),
                        static_cast<float>(glyph->atlasY + glyph->height));
                    instance.color = color;
                    instance.atlasInfo.x = static_cast<uint32_t>(glyph->atlasType);
                    quads.push_back(instance);
                }
                penX += glyph->advanceX;
            }

            if (quads.empty())
            {
                vke_render::QuadInstanceGPU quad{};
                quad.color = color;
                quads.push_back(quad);
            }
            return setGeometry(std::move(quads));
        }
    };
}

#endif
