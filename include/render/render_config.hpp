#ifndef RENDER_CONFIG_H
#define RENDER_CONFIG_H

#include <cstdint>
#include <nlohmann/json.hpp>

namespace vke_render
{
    struct BloomConfig
    {
        float threshold = 1.0f;
        float intensity = 0.35f;
        float radius = 1.0f;
        float knee = 0.25f;

        BloomConfig() = default;
        explicit BloomConfig(const nlohmann::json &json)
        {
            LoadJSON(json);
        }

        void LoadJSON(const nlohmann::json &json)
        {
            if (json.is_null())
                return;

            threshold = json.value("threshold", threshold);
            intensity = json.value("intensity", intensity);
            radius = json.value("radius", radius);
            knee = json.value("knee", knee);
        }
    };

    struct ToneMappingConfig
    {
        float exposure = 1.0f;
        uint32_t toneMappingMode = 0;

        ToneMappingConfig() = default;
        explicit ToneMappingConfig(const nlohmann::json &json)
        {
            LoadJSON(json);
        }

        void LoadJSON(const nlohmann::json &json)
        {
            if (json.is_null())
                return;

            exposure = json.value("exposure", exposure);
            toneMappingMode = json.value("toneMappingMode", toneMappingMode);
        }
    };

    struct SSAOConfig
    {
        float radius = 0.1f;
        float bias = 0.025f;
        float intensity = 0.6f;
        float power = 1.0f;

        SSAOConfig() = default;
        explicit SSAOConfig(const nlohmann::json &json)
        {
            LoadJSON(json);
        }

        void LoadJSON(const nlohmann::json &json)
        {
            if (json.is_null())
                return;

            radius = json.value("radius", radius);
            bias = json.value("bias", bias);
            intensity = json.value("intensity", intensity);
            power = json.value("power", power);
        }
    };

    struct RenderConfig
    {
        BloomConfig bloom;
        ToneMappingConfig toneMapping;
        SSAOConfig ssao;
        nlohmann::json sourceJSON = nlohmann::json::object();

        RenderConfig() = default;
        explicit RenderConfig(const nlohmann::json &json)
        {
            LoadJSON(json);
        }

        void LoadJSON(const nlohmann::json &json)
        {
            sourceJSON = json.is_null() ? nlohmann::json::object() : json;
            if (json.is_null())
                return;

            bloom.LoadJSON(json.value("bloom", nlohmann::json::object()));
            toneMapping.LoadJSON(json.value("toneMapping", nlohmann::json::object()));
            ssao.LoadJSON(json.value("ssao", nlohmann::json::object()));
        }
    };
}

#endif
