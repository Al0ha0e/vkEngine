#ifndef INTEROP_TEXT_H
#define INTEROP_TEXT_H

#include <cstdint>
#include <interop/interop.hpp>
#include <interop/math.hpp>

namespace vke_interop
{
    using GetUITextLengthFn = uint32_t(VKE_INTEROP_CDECL *)(uint32_t);
    using GetUITextTextFn = uint32_t(VKE_INTEROP_CDECL *)(uint32_t, char *, uint32_t);
    using SetUITextTextFn = void(VKE_INTEROP_CDECL *)(uint32_t, const char *, uint32_t);
    using GetUITextColorFn = void(VKE_INTEROP_CDECL *)(uint32_t, Vector4<float> *);
    using SetUITextColorFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector4<float> *);

    uint32_t VKE_INTEROP_CDECL GetUITextLength(uint32_t entity);
    uint32_t VKE_INTEROP_CDECL GetUITextText(uint32_t entity, char *buffer, uint32_t bufferLength);
    void VKE_INTEROP_CDECL SetUITextText(uint32_t entity, const char *text, uint32_t length);
    void VKE_INTEROP_CDECL GetUITextColor(uint32_t entity, Vector4<float> *color);
    void VKE_INTEROP_CDECL SetUITextColor(uint32_t entity, const Vector4<float> *color);
}

#endif
