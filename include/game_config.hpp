#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <common.hpp>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <reflect.hpp>

namespace vke_common
{
    REFLECT_STRUCT(GameConfig)
    {
    private:
        static GameConfig *instance;

        GameConfig(const GameConfig &);
        GameConfig &operator=(const GameConfig &);

    public:
        REFLECT_FIELD(uint32_t, windowWidth);
        REFLECT_FIELD(uint32_t, windowHeight);
        REFLECT_FIELD(std::string, assetLUTPath);
        REFLECT_FIELD(std::string, defaultScenePath);
        REFLECT_FIELD(std::string, gameScriptPath);

        GameConfig() : windowWidth(800), windowHeight(600),
                       assetLUTPath(), defaultScenePath(), gameScriptPath() {}
        GameConfig(const nlohmann::json &json)
        {
            loadJSON(json);
        }

        static GameConfig *GetInstance()
        {
            VKE_FATAL_IF(instance == nullptr, "GameConfig not initialized!")
            return instance;
        }

        static GameConfig *Init(const nlohmann::json &json)
        {
            instance = new GameConfig(json);
            return instance;
        }
        static GameConfig *Init()
        {
            instance = new GameConfig();
            return instance;
        }

        static void Dispose()
        {
            delete instance;
            instance = nullptr;
        }

    private:
        LOAD_JSON;
    };
}

#endif
