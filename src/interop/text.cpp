#include <component/text.hpp>
#include <interop/text.hpp>
#include <scene.hpp>
#include <algorithm>
#include <cstring>

namespace vke_interop
{
    static vke_common::Scene *GetCurrentScene()
    {
        return vke_common::SceneManager::GetInstance()->currentScene.get();
    }

    static vke_component::UIText *GetUIText(uint32_t entity)
    {
        vke_common::Scene *scene = GetCurrentScene();
        entt::entity ent = static_cast<entt::entity>(entity);
        return &scene->registry.get<vke_component::UIText>(ent);
    }

    uint32_t VKE_INTEROP_CDECL GetUITextLength(uint32_t entity)
    {
        vke_component::UIText *text = GetUIText(entity);
        return static_cast<uint32_t>(text->GetText().size());
    }

    uint32_t VKE_INTEROP_CDECL GetUITextText(uint32_t entity, char *buffer, uint32_t bufferLength)
    {
        vke_component::UIText *text = GetUIText(entity);
        const std::string &value = text->GetText();
        const uint32_t written = std::min(bufferLength, static_cast<uint32_t>(value.size()));
        std::memcpy(buffer, value.data(), written);
        return written;
    }

    void VKE_INTEROP_CDECL SetUITextText(uint32_t entity, const char *textValue, uint32_t length)
    {
        vke_component::UIText *text = GetUIText(entity);
        text->SetText(std::string_view(textValue, length));
    }

    void VKE_INTEROP_CDECL GetUITextColor(uint32_t entity, Vector4<float> *color)
    {
        vke_component::UIText *text = GetUIText(entity);
        const glm::vec4 &value = text->GetColor();
        *color = Vector4<float>{value.r, value.g, value.b, value.a};
    }

    void VKE_INTEROP_CDECL SetUITextColor(uint32_t entity, const Vector4<float> *color)
    {
        vke_component::UIText *text = GetUIText(entity);
        text->SetColor(glm::vec4(color->x, color->y, color->z, color->w));
    }
}
