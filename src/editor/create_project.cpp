#include <editor/editor.hpp>
#include <imgui/dialog/ImGuiFileDialog.h>
#include <fstream>

namespace vke_editor
{
    void Editor::EnterProjectCreationMode()
    {
        instance->projectCreationActive = true;
        instance->projectCreationPending = false;
        instance->projectCancelRequested = false;
        std::memset(instance->pendingProjectName, 0, sizeof(instance->pendingProjectName));
        std::memset(instance->pendingProjectDir, 0, sizeof(instance->pendingProjectDir));
    }

    void Editor::showProjectCreationDialog()
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::Begin("Create New Project", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::TextWrapped("Create a new project to get started with the editor.");
        ImGui::Separator();

        ImGui::InputText("Project Name", pendingProjectName, sizeof(pendingProjectName));

        ImGui::InputText("Parent Directory", pendingProjectDir, sizeof(pendingProjectDir));
        ImGui::SameLine();
        if (ImGui::Button("Browse..."))
        {
            IGFD::FileDialogConfig browseConfig;
            browseConfig.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("ChooseProjectDir", "Choose Directory", nullptr, browseConfig);
        }

        ImGui::Separator();

        const bool canCreate = std::strlen(pendingProjectName) > 0 && std::strlen(pendingProjectDir) > 0;
        if (!canCreate)
            ImGui::BeginDisabled();
        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            projectCreationPending = true;
        }
        if (!canCreate)
            ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            projectCancelRequested = true;
        }

        ImGui::End();

        if (ImGuiFileDialog::Instance()->Display("ChooseProjectDir"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                const std::string path = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::strncpy(pendingProjectDir, path.c_str(), sizeof(pendingProjectDir) - 1);
            }
            ImGuiFileDialog::Instance()->Close();
        }
    }

    std::filesystem::path Editor::finalizeProjectCreation()
    {
        namespace fs = std::filesystem;

        const fs::path projectPath = fs::path(pendingProjectDir) / pendingProjectName;
        fs::create_directories(projectPath);
        VKE_LOG_INFO("Created project directory: {}", projectPath.string());

        {
            nlohmann::json j;
            j["windowWidth"] = 1920;
            j["windowHeight"] = 1080;
            j["workingDirectory"] = projectPath.string();
            j["assetDBPath"] = "asset_db.json";
            j["gameConfigPath"] = "gameconfig.json";

            std::ofstream ofs(projectPath / "editorconfig.json");
            ofs << j.dump(4);
            VKE_LOG_INFO("Written editorconfig.json");
        }

        {
            nlohmann::json j;
            j["windowWidth"] = 1920;
            j["windowHeight"] = 1080;
            j["enableVulkanValidationLayers"] = false;
            j["assetLUTPath"] = (projectPath / "asset_db.json").string();
            j["defaultScenePath"] = (projectPath / "scene.json").string();
            j["gameScriptPath"] = "";
            j["physicsConfig"] = nlohmann::json::object();
            j["renderConfig"] = nlohmann::json::object();

            std::ofstream ofs(projectPath / "gameconfig.json");
            ofs << j.dump(4);
            VKE_LOG_INFO("Written gameconfig.json");
        }

        {
            nlohmann::json j;
            j["maxid"] = 3;
            j["layers"] = {"default", "editor"};
            j["objects"] = nlohmann::json::array();

            // Camera entity (ID 1)
            nlohmann::json cameraObj;
            cameraObj["id"] = 1;
            cameraObj["static"] = false;
            cameraObj["name"] = "Camera";
            cameraObj["layer"] = 0;
            cameraObj["parent"] = 0;
            cameraObj["children"] = nlohmann::json::array();
            cameraObj["transform"] = {
                {"pos", {0.0f, 0.0f, 5.0f}},
                {"scl", {1.0f, 1.0f, 1.0f}},
                {"rot", {0.0f, 0.0f, 0.0f, 1.0f}}};
            cameraObj["components"] = {
                {{"type", "camera"}, {"fov", 60.0}, {"width", 1920.0}, {"height", 1080.0}, {"near", 0.01}, {"far", 1000.0}}};

            nlohmann::json lightObj;
            lightObj["id"] = 2;
            lightObj["static"] = false;
            lightObj["name"] = "Directional Light";
            lightObj["layer"] = 0;
            lightObj["parent"] = 0;
            lightObj["children"] = nlohmann::json::array();
            lightObj["transform"] = {
                {"pos", {0.0f, 0.0f, 0.0f}},
                {"scl", {1.0f, 1.0f, 1.0f}},
                {"rot", {-0.6324555f, 0.3162278f, 0.0f, 0.7071068f}}};
            lightObj["components"] = {
                {{"type", "directionalLight"}, {"color", {1.0f, 0.96f, 0.9f}}, {"intensity", 4.0f}}};

            j["objects"].push_back(cameraObj);
            j["objects"].push_back(lightObj);

            std::ofstream ofs(projectPath / "scene.json");
            ofs << j.dump(4);
            VKE_LOG_INFO("Written scene.json with camera and directional light");
        }

        {
            nlohmann::json j = nlohmann::json::array();
            std::ofstream ofs(projectPath / "asset_db.json");
            ofs << j.dump(4);
            VKE_LOG_INFO("Written asset_db.json");
        }
        return projectPath;
    }
}