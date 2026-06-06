#ifndef PUSH_CONSTANT_H
#define PUSH_CONSTANT_H

namespace vke_render
{
    struct PushConstantInfo
    {
        uint32_t size;
        uint32_t offset;
        bool isFloat;
        const void *pValues;

        PushConstantInfo() : size(0), offset(0), isFloat(false), pValues(nullptr) {}
        PushConstantInfo(const uint32_t size, const void *pValues, const bool isFloat = true, const uint32_t offset = 0)
            : size(size), offset(offset), isFloat(isFloat), pValues(pValues) {}
    };
}

#endif
