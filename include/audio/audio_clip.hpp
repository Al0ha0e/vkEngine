#ifndef AUDIO_CLIP_H
#define AUDIO_CLIP_H

#include <string>
#include <cstdint>
#include <common.hpp>

namespace vke_audio
{
    class AudioClip
    {
    public:
        vke_common::AssetHandle handle = 0;
        std::string path;

        AudioClip() = default;
        AudioClip(vke_common::AssetHandle h, const std::string &p) : handle(h), path(p) {}

        bool IsValid() const { return !path.empty(); }
    };
}

#endif
