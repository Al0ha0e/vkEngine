#ifndef EDITOR_CONFIG_H
#define EDITOR_CONFIG_H

#include <game_config.hpp>
#include <filesystem>

namespace vke_editor
{
    struct EditorConfig
    {
    private:
        static EditorConfig *instance;

        EditorConfig(const EditorConfig &) = delete;
        EditorConfig &operator=(const EditorConfig &) = delete;

    public:
        uint32_t windowWidth;
        uint32_t windowHeight;
        std::filesystem::path workingDirectory;
        std::filesystem::path assetDBPath;    // relative to workingDirectory
        std::filesystem::path gameConfigPath; // relative to workingDirectory
        vke_common::GameConfig *gameConfig;

        EditorConfig() : windowWidth(1920), windowHeight(1080), gameConfig(nullptr) {};

        EditorConfig(const nlohmann::json &json)
            : windowWidth(json["windowWidth"]), windowHeight(json["windowHeight"]),
              workingDirectory(json["workingDirectory"].get<std::string>()),
              assetDBPath(json["assetDBPath"].get<std::string>()),
              gameConfigPath(json.value("gameConfigPath", "")),
              gameConfig(nullptr) {}

        static EditorConfig *GetInstance()
        {
            VKE_FATAL_IF(instance == nullptr, "EditorConfig not initialized!")
            return instance;
        }

        static EditorConfig *Init(const nlohmann::json &json)
        {
            instance = new EditorConfig(json);
            const nlohmann::json &gameConfigJSON = vke_common::AssetManager::LoadJSON(
                GetFullPath(instance->gameConfigPath).string());
            instance->gameConfig = vke_common::GameConfig::Init(gameConfigJSON);
            return instance;
        }
        static EditorConfig *Init()
        {
            instance = new EditorConfig();
            instance->gameConfig = vke_common::GameConfig::Init();
            return instance;
        }

        static void Dispose()
        {
            vke_common::GameConfig::Dispose();
            delete instance;
            instance = nullptr;
        }

        static std::filesystem::path GetFullPath(const std::filesystem::path &relPath)
        {
            return instance->workingDirectory / relPath;
        }
    };
}

#endif
