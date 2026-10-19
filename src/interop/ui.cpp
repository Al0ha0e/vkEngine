#include <interop/ui.hpp>
#include <scene.hpp>
#include <ui/text.hpp>
#include <algorithm>
#include <cstring>

namespace vke_interop
{
    static vke_component::Canvas *getCanvas(uint32_t entity)
    {
        auto *scene = vke_common::SceneManager::GetInstance();
        auto id = static_cast<entt::entity>(entity);
        if (!scene->registry.valid(id) || scene->IsPendingDestroy(id)) return nullptr;
        return scene->registry.try_get<vke_component::Canvas>(id);
    }

    static vke_ui::UIComponent *getNode(uint32_t entity, uint64_t handle)
    {
        auto *canvas = getCanvas(entity);
        return canvas ? canvas->Get(handle) : nullptr;
    }

    int32_t VKE_INTEROP_CDECL GetUIType(uint32_t entity, uint64_t handle)
    {
        auto *node = getNode(entity, handle);
        return node ? static_cast<int32_t>(node->GetType()) : 0;
    }

    uint64_t VKE_INTEROP_CDECL FindUIChild(uint32_t entity, uint64_t parent, const char *name, uint32_t length)
    {
        auto *canvas = getCanvas(entity);
        if (!canvas || (!name && length)) return 0;
        auto *node = canvas->Find(std::string_view(name ? name : "", length), parent);
        return node ? node->handle : 0;
    }

    int32_t VKE_INTEROP_CDECL GetUIChildCount(uint32_t entity, uint64_t parent)
    {
        auto *canvas = getCanvas(entity);
        if (!canvas) return -1;
        auto *node = canvas->Get(parent);
        if (parent && !node) return -1;
        return static_cast<int32_t>((node ? node->children : canvas->GetChildren()).size());
    }

    uint64_t VKE_INTEROP_CDECL GetUIChild(uint32_t entity, uint64_t parent, uint32_t index)
    {
        auto *canvas = getCanvas(entity);
        if (!canvas) return 0;
        auto *node = canvas->Get(parent);
        if (parent && !node) return 0;
        const auto &children = node ? node->children : canvas->GetChildren();
        return index < children.size() ? children[index]->handle : 0;
    }

    uint64_t VKE_INTEROP_CDECL CreateUINode(uint32_t entity, uint64_t parent, int32_t type, const char *name, uint32_t length)
    {
        auto *canvas = getCanvas(entity);
        if (!canvas || (!name && length)) return 0;
        std::unique_ptr<vke_ui::UINodeData> data;
        if (type == static_cast<int32_t>(vke_ui::UIType::Text)) data = std::make_unique<vke_ui::UITextData>();
        else if (type == static_cast<int32_t>(vke_ui::UIType::Image)) data = std::make_unique<vke_ui::UIImageData>();
        else if (type == static_cast<int32_t>(vke_ui::UIType::Container)) data = std::make_unique<vke_ui::UIContainerData>();
        else return 0;
        data->name.assign(name ? name : "", length);
        auto result = canvas->Add(*data, parent);
        return result ? (*result)->handle : 0;
    }

    int32_t VKE_INTEROP_CDECL RemoveUINode(uint32_t entity, uint64_t handle)
    {
        auto *canvas = getCanvas(entity);
        return canvas && canvas->Remove(handle).has_value();
    }

    int32_t VKE_INTEROP_CDECL SetUIParent(uint32_t entity, uint64_t handle, uint64_t parent)
    {
        auto *canvas = getCanvas(entity);
        return canvas && canvas->SetParent(handle, parent).has_value();
    }

    int32_t VKE_INTEROP_CDECL SetUISiblingIndex(uint32_t entity, uint64_t handle, uint32_t index)
    {
        auto *canvas = getCanvas(entity);
        return canvas && canvas->SetSiblingIndex(handle, index).has_value();
    }

    int32_t VKE_INTEROP_CDECL GetUIRectTransform(uint32_t entity, uint64_t handle, UIRectTransform *value)
    {
        auto *node = getNode(entity, handle);
        if (!node || !value) return 0;
        const auto &rect = node->rectTransform;
        *value = {{rect.anchorMin.x, rect.anchorMin.y}, {rect.anchorMax.x, rect.anchorMax.y},
                  {rect.offsetMin.x, rect.offsetMin.y}, {rect.offsetMax.x, rect.offsetMax.y},
                  {rect.pivot.x, rect.pivot.y}, {rect.scale.x, rect.scale.y}, rect.rotation};
        return 1;
    }

    int32_t VKE_INTEROP_CDECL SetUIRectTransform(uint32_t entity, uint64_t handle, const UIRectTransform *value)
    {
        auto *node = getNode(entity, handle);
        if (!node || !value) return 0;
        vke_ui::RectTransform rect;
        rect.anchorMin = {value->anchorMin.x, value->anchorMin.y};
        rect.anchorMax = {value->anchorMax.x, value->anchorMax.y};
        rect.offsetMin = {value->offsetMin.x, value->offsetMin.y};
        rect.offsetMax = {value->offsetMax.x, value->offsetMax.y};
        rect.pivot = {value->pivot.x, value->pivot.y};
        rect.scale = {value->scale.x, value->scale.y};
        rect.rotation = value->rotation;
        return node->SetRectTransform(rect).has_value();
    }

    int32_t VKE_INTEROP_CDECL GetUIResolvedSize(uint32_t entity, uint64_t handle, Vector2<float> *value)
    {
        auto *canvas = getCanvas(entity);
        if (!canvas || !value) return 0;
        auto *node = canvas->Get(handle);
        if (handle && !node) return 0;
        const auto &size = node ? node->resolvedSize : canvas->GetResolvedSize();
        *value = {size.x, size.y};
        return 1;
    }

    int32_t VKE_INTEROP_CDECL GetCanvasReferenceResolution(uint32_t entity, Vector2<float> *value)
    {
        auto *canvas = getCanvas(entity);
        if (!canvas || !value) return 0;
        const auto &size = canvas->GetReferenceResolution();
        *value = {size.x, size.y};
        return 1;
    }

    int32_t VKE_INTEROP_CDECL SetCanvasReferenceResolution(uint32_t entity, const Vector2<float> *value)
    {
        auto *canvas = getCanvas(entity);
        return canvas && value && canvas->SetReferenceResolution({value->x, value->y}).has_value();
    }

    int32_t VKE_INTEROP_CDECL GetCanvasSortOrder(uint32_t entity, int32_t *value)
    {
        auto *canvas = getCanvas(entity);
        if (!canvas || !value) return 0;
        *value = canvas->sortOrder;
        return 1;
    }

    int32_t VKE_INTEROP_CDECL SetCanvasSortOrder(uint32_t entity, int32_t value)
    {
        auto *canvas = getCanvas(entity);
        if (!canvas) return 0;
        canvas->sortOrder = value;
        return 1;
    }

    static vke_ui::UIText *getText(uint32_t entity, uint64_t handle)
    {
        auto *node = getNode(entity, handle);
        return node && node->GetType() == vke_ui::UIType::Text ? static_cast<vke_ui::UIText *>(node) : nullptr;
    }

    int32_t VKE_INTEROP_CDECL GetUITextLength(uint32_t entity, uint64_t handle)
    {
        auto *text = getText(entity, handle);
        return text ? static_cast<int32_t>(text->GetText().size()) : -1;
    }

    int32_t VKE_INTEROP_CDECL GetUITextText(uint32_t entity, uint64_t handle, char *buffer, uint32_t length)
    {
        auto *text = getText(entity, handle);
        if (!text || (!buffer && length)) return -1;
        const auto &value = text->GetText();
        const auto written = std::min(length, static_cast<uint32_t>(value.size()));
        if (written) std::memcpy(buffer, value.data(), written);
        return static_cast<int32_t>(written);
    }

    int32_t VKE_INTEROP_CDECL SetUITextText(uint32_t entity, uint64_t handle, const char *value, uint32_t length)
    {
        auto *text = getText(entity, handle);
        if (!text || (!value && length) || length > INT32_MAX) return 0;
        text->SetText(std::string_view(value ? value : "", length));
        return 1;
    }

    int32_t VKE_INTEROP_CDECL GetUITextColor(uint32_t entity, uint64_t handle, Vector4<float> *color)
    {
        auto *text = getText(entity, handle);
        if (!text || !color) return 0;
        const auto &value = text->GetColor();
        *color = {value.r, value.g, value.b, value.a};
        return 1;
    }

    int32_t VKE_INTEROP_CDECL SetUITextColor(uint32_t entity, uint64_t handle, const Vector4<float> *color)
    {
        auto *text = getText(entity, handle);
        if (!text || !color) return 0;
        text->SetColor({color->x, color->y, color->z, color->w});
        return 1;
    }

    static vke_ui::UIImage *getImage(uint32_t entity, uint64_t handle)
    {
        auto *node = getNode(entity, handle);
        return node && node->GetType() == vke_ui::UIType::Image ? static_cast<vke_ui::UIImage *>(node) : nullptr;
    }

    int32_t VKE_INTEROP_CDECL GetUIImageTexture(uint32_t entity, uint64_t handle, uint64_t *value)
    {
        auto *image = getImage(entity, handle);
        if (!image || !value) return 0;
        *value = image->GetTexture();
        return 1;
    }

    int32_t VKE_INTEROP_CDECL SetUIImageTexture(uint32_t entity, uint64_t handle, uint64_t value)
    {
        auto *image = getImage(entity, handle);
        return image && image->SetTexture(value).has_value();
    }

    int32_t VKE_INTEROP_CDECL GetUIImageColor(uint32_t entity, uint64_t handle, Vector4<float> *value)
    {
        auto *image = getImage(entity, handle);
        if (!image || !value) return 0;
        const auto &color = image->GetColor();
        *value = {color.r, color.g, color.b, color.a};
        return 1;
    }

    int32_t VKE_INTEROP_CDECL SetUIImageColor(uint32_t entity, uint64_t handle, const Vector4<float> *value)
    {
        auto *image = getImage(entity, handle);
        return image && value && image->SetColor({value->x, value->y, value->z, value->w}).has_value();
    }

    int32_t VKE_INTEROP_CDECL GetUIImageUVRect(uint32_t entity, uint64_t handle, Vector4<float> *value)
    {
        auto *image = getImage(entity, handle);
        if (!image || !value) return 0;
        const auto &uv = image->GetUVRect();
        *value = {uv.x, uv.y, uv.z, uv.w};
        return 1;
    }

    int32_t VKE_INTEROP_CDECL SetUIImageUVRect(uint32_t entity, uint64_t handle, const Vector4<float> *value)
    {
        auto *image = getImage(entity, handle);
        return image && value && image->SetUVRect({value->x, value->y, value->z, value->w}).has_value();
    }

    int32_t VKE_INTEROP_CDECL GetUIImagePreserveAspect(uint32_t entity, uint64_t handle, int32_t *value)
    {
        auto *image = getImage(entity, handle);
        if (!image || !value) return 0;
        *value = image->GetPreserveAspect();
        return 1;
    }

    int32_t VKE_INTEROP_CDECL SetUIImagePreserveAspect(uint32_t entity, uint64_t handle, int32_t value)
    {
        auto *image = getImage(entity, handle);
        if (!image || (value != 0 && value != 1)) return 0;
        image->SetPreserveAspect(value != 0);
        return 1;
    }
}
