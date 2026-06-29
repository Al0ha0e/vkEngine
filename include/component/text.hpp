#ifndef UI_TEXT_COMPONENT_H
#define UI_TEXT_COMPONENT_H

#include <component/ui.hpp>
#include <font.hpp>
#include <nlohmann/json.hpp>
#include <asset.hpp>
#include <string>
#include <string_view>

namespace vke_component
{
    class UIText : public UIComponent
    {
    public:
        UIText(const vke_common::Transform &transform, std::string text,
               const glm::vec4 &color = glm::vec4(1.0f),
               std::shared_ptr<vke_render::Material> material = nullptr,
               vke_render::CPUGlyphData *glyphData = nullptr)
            : UIComponent(transform, std::move(material), glyphData), text(std::move(text)), color(color),
              font(vke_common::AssetManager::LoadFont(vke_common::BUILTIN_FONT_ARIAL_ID))
        {
            rebuild();
        }

        UIText(const vke_common::Transform &transform, const nlohmann::json &json, vke_render::CPUGlyphData *glyphData)
            : UIComponent(transform,
                          json.contains("material") && json["material"].get<vke_common::AssetHandle>() != 0
                              ? vke_common::AssetManager::LoadMaterial(json["material"].get<vke_common::AssetHandle>())
                              : nullptr,
                          glyphData),
              text(json.value("text", std::string())),
              font(vke_common::AssetManager::LoadFont(vke_common::BUILTIN_FONT_ARIAL_ID))
        {
            if (json.contains("color"))
            {
                const auto &value = json["color"];
                color = glm::vec4(value[0].get<float>(), value[1].get<float>(),
                                  value[2].get<float>(), value[3].get<float>());
            }
            rebuild();
        }

        bool LoadToEngine()
        {
            return UIComponent::LoadToEngine();
        }

        void SetText(std::string_view newText)
        {
            text.assign(newText);
            rebuild();
        }

        void SetColor(const glm::vec4 &newColor)
        {
            color = newColor;
            SetGlyphColor(newColor);
        }

        const std::string &GetText() const { return text; }
        const glm::vec4 &GetColor() const { return color; }
        nlohmann::json ToJSON() const
        {
            return {
                {"type", "uiText"},
                {"text", text},
                {"color", {color.r, color.g, color.b, color.a}},
                {"material", GetMaterial() == nullptr ? 0 : GetMaterial()->handle}};
        }

    private:
        std::string text;
        glm::vec4 color{1.0f};
        std::shared_ptr<vke_common::Font> font;

        bool rebuild()
        {
            std::vector<vke_render::GlyphInstanceGPU> glyphs;
            glyphs.reserve(text.size());

            int penX = 0;
            int baselineY = font->ascender;
            glm::vec2 minimum(std::numeric_limits<float>::max());
            glm::vec2 maximum(std::numeric_limits<float>::lowest());
            bool hasBounds = false;

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

                    vke_render::GlyphInstanceGPU instance{};
                    instance.quadRect = glm::vec4(minX, minY, maxX, maxY);
                    instance.uvRect = glm::vec4(
                        static_cast<float>(glyph->atlasX), static_cast<float>(glyph->atlasY),
                        static_cast<float>(glyph->atlasX + glyph->width),
                        static_cast<float>(glyph->atlasY + glyph->height));
                    instance.color = color;
                    instance.atlasInfo.x = static_cast<uint32_t>(glyph->atlasType);
                    glyphs.push_back(instance);

                    minimum = glm::min(minimum, glm::vec2(minX, minY));
                    maximum = glm::max(maximum, glm::vec2(maxX, maxY));
                    hasBounds = true;
                }
                penX += glyph->advanceX;
            }

            const vke_common::AABB2D bounds = hasBounds
                                                  ? vke_common::AABB2D(minimum, maximum)
                                                  : vke_common::AABB2D(glm::vec2(0.0f), glm::vec2(0.0f));
            return SetGeometry(std::move(glyphs), bounds);
        }
    };
}

#endif
