#include <editor/editor.hpp>
#include <imgui_impl_vulkan.h>
#include <common.hpp>
#include <algorithm>
#include <cstring>

namespace vke_editor
{
    const std::string EditorAssetLUTPath = std::string(REL_DIR) + "/editor_assets/assets.json";
    static std::string NormalizeAssetPath(std::string path)
    {
        std::replace(path.begin(), path.end(), '\\', '/');
        while (!path.empty() && path.front() == '/')
            path.erase(path.begin());
        return path;
    }

    static std::vector<std::string> GetAssetDirectoryParts(const std::string &assetPath)
    {
        std::string normalizedPath = NormalizeAssetPath(assetPath);
        const size_t fileNamePos = normalizedPath.find_last_of('/');
        if (fileNamePos == std::string::npos)
            return {};

        std::vector<std::string> parts;
        std::string directory = normalizedPath.substr(0, fileNamePos);
        size_t begin = 0;
        while (begin < directory.length())
        {
            const size_t end = directory.find('/', begin);
            std::string part = directory.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
            if (!part.empty())
                parts.push_back(part);
            if (end == std::string::npos)
                break;
            begin = end + 1;
        }
        return parts;
    }

    static const char *AssetBrowserModeName(AssetBrowserMode mode)
    {
        switch (mode)
        {
        case AssetBrowserMode::ByDirectory:
            return "Directory";
        default:
            return "Type";
        }
    }

    static const char *AssetTypeName(vke_common::AssetType type)
    {
        return vke_common::AssetTypeToName[static_cast<int>(type)].c_str();
    }

    template <typename AssetMap>
    static void AddAssetsToDirectoryTree(AssetTreeNode &root, vke_common::AssetType type, const AssetMap &assets)
    {
        for (const auto &[id, asset] : assets)
        {
            AssetTreeNode *node = &root;
            std::vector<std::string> directoryParts = GetAssetDirectoryParts(asset.path);
            if (directoryParts.empty())
                directoryParts.push_back("No Path");

            for (const std::string &part : directoryParts)
            {
                AssetTreeNode &child = node->children[part];
                if (child.name.empty())
                    child.name = part;
                node = &child;
            }

            node->assets.push_back({type, id, asset.name, asset.path});
        }
    }

    static const char *RenderModeName(vke_render::MaterialRenderMode mode)
    {
        switch (mode)
        {
        case vke_render::MaterialRenderMode::CUTOFF_MODE:
            return "Cutoff";
        case vke_render::MaterialRenderMode::BLEND_MODE:
            return "Blend";
        default:
            return "Opaque";
        }
    }

    static const char *BlendModeName(vke_render::MaterialBlendMode mode)
    {
        switch (mode)
        {
        case vke_render::MaterialBlendMode::PREMULTIPLIED_ALPHA:
            return "Premultiplied Alpha";
        case vke_render::MaterialBlendMode::ADDITIVE:
            return "Additive";
        default:
            return "Alpha";
        }
    }

    static std::string TextureDisplayName(vke_common::AssetHandle handle)
    {
        vke_common::TextureAsset *texture = vke_common::AssetManager::GetTextureAsset(handle);
        if (texture == nullptr)
            return std::to_string(handle) + "  <missing>";
        return std::to_string(handle) + "  " + texture->name;
    }

    static bool GetAssetInfo(vke_common::AssetType type, vke_common::AssetHandle handle, AssetTreeEntry &entry)
    {
        vke_common::AssetManager *assetManager = vke_common::AssetManager::GetInstance();
        switch (type)
        {
        case vke_common::ASSET_TEXTURE:
            if (vke_common::TextureAsset *asset = vke_common::AssetManager::GetTextureAsset(handle))
            {
                entry = {type, handle, asset->name, asset->path};
                return true;
            }
            break;
        case vke_common::ASSET_MESH:
            if (auto it = assetManager->meshCache.find(handle); it != assetManager->meshCache.end())
            {
                entry = {type, handle, it->second.name, it->second.path};
                return true;
            }
            break;
        case vke_common::ASSET_VF_SHADER:
            if (auto it = assetManager->vfShaderCache.find(handle); it != assetManager->vfShaderCache.end())
            {
                entry = {type, handle, it->second.name, it->second.path};
                return true;
            }
            break;
        case vke_common::ASSET_COMPUTE_SHADER:
            if (auto it = assetManager->computeShaderCache.find(handle); it != assetManager->computeShaderCache.end())
            {
                entry = {type, handle, it->second.name, it->second.path};
                return true;
            }
            break;
        case vke_common::ASSET_MATERIAL:
            if (vke_common::MaterialAsset *asset = vke_common::AssetManager::GetMaterialAsset(handle))
            {
                entry = {type, handle, asset->name, asset->path};
                return true;
            }
            break;
        case vke_common::ASSET_SKELETON:
            if (auto it = assetManager->skeletonCache.find(handle); it != assetManager->skeletonCache.end())
            {
                entry = {type, handle, it->second.name, it->second.path};
                return true;
            }
            break;
        case vke_common::ASSET_ANIMATION:
            if (auto it = assetManager->animationCache.find(handle); it != assetManager->animationCache.end())
            {
                entry = {type, handle, it->second.name, it->second.path};
                return true;
            }
            break;
        case vke_common::ASSET_SCENE:
            if (auto it = assetManager->sceneCache.find(handle); it != assetManager->sceneCache.end())
            {
                entry = {type, handle, it->second.name, it->second.path};
                return true;
            }
            break;
        case vke_common::ASSET_FONT:
            if (auto it = assetManager->fontCache.find(handle); it != assetManager->fontCache.end())
            {
                entry = {type, handle, it->second.name, it->second.path};
                return true;
            }
            break;
        default:
            break;
        }
        return false;
    }

    VkDescriptorSet Editor::getTexturePreviewDescriptorSet(vke_common::AssetHandle textureAsset)
    {
        const auto descriptorIt = texturePreviewDescriptorSets.find(textureAsset);
        if (descriptorIt != texturePreviewDescriptorSets.end())
            return descriptorIt->second;

        std::shared_ptr<vke_render::Texture2D> texture = vke_common::AssetManager::LoadTexture2D(textureAsset);
        if (texture == nullptr)
            return VK_NULL_HANDLE;

        VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(
            texture->textureImageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        texturePreviewDescriptorSets[textureAsset] = descriptorSet;
        return descriptorSet;
    }

    void Editor::drawTexturePreview(vke_common::AssetHandle textureAsset, float maxSize)
    {
        std::shared_ptr<vke_render::Texture2D> texture = vke_common::AssetManager::LoadTexture2D(textureAsset);
        VkDescriptorSet descriptorSet = getTexturePreviewDescriptorSet(textureAsset);
        if (texture == nullptr || descriptorSet == VK_NULL_HANDLE)
        {
            ImGui::TextDisabled("Preview unavailable");
            return;
        }

        const float width = static_cast<float>(texture->width);
        const float height = static_cast<float>(texture->height);
        const float scale = width <= 0.0f || height <= 0.0f ? 1.0f : std::min(maxSize / width, maxSize / height);
        ImGui::Image(
            ImTextureRef((ImTextureID)descriptorSet),
            ImVec2(width * scale, height * scale));
    }

    void Editor::showAssets()
    {
        ImGui::Begin("Assets");

        vke_common::Scene *scene = vke_common::SceneManager::GetInstance()->currentScene.get();
        if (scene != nullptr)
            ImGui::Text("Scene: %s", scene->path.empty() ? "<unsaved>" : scene->path.c_str());

        vke_common::AssetManager *assetManager = vke_common::AssetManager::GetInstance();
        const bool byType = assetBrowserMode == AssetBrowserMode::ByType;
        if (ImGui::Button(byType ? "Directory View" : "Type View"))
            assetBrowserMode = byType ? AssetBrowserMode::ByDirectory : AssetBrowserMode::ByType;
        ImGui::SameLine();
        ImGui::TextDisabled("Mode: %s", AssetBrowserModeName(assetBrowserMode));
        ImGui::Separator();

        if (assetBrowserMode == AssetBrowserMode::ByDirectory)
            showAssetsByDirectory(assetManager);
        else
            showAssetsByType(assetManager);

        ImGui::End();
    }

    void Editor::showAssetsByType(vke_common::AssetManager *assetManager)
    {
        const auto showAssetGroup = [this](const char *label, vke_common::AssetType type, const auto &assets)
        {
            if (!ImGui::TreeNode(label))
                return;

            for (const auto &[id, asset] : assets)
            {
                AssetTreeEntry entry{type, id, asset.name, asset.path};
                drawAssetEntry(entry);
            }

            ImGui::TreePop();
        };

        showAssetGroup("Textures", vke_common::ASSET_TEXTURE, assetManager->textureCache);
        showAssetGroup("Materials", vke_common::ASSET_MATERIAL, assetManager->materialCache);
        showAssetGroup("VF Shaders", vke_common::ASSET_VF_SHADER, assetManager->vfShaderCache);
        showAssetGroup("Compute Shaders", vke_common::ASSET_COMPUTE_SHADER, assetManager->computeShaderCache);
        showAssetGroup("Meshes", vke_common::ASSET_MESH, assetManager->meshCache);
        showAssetGroup("Skeletons", vke_common::ASSET_SKELETON, assetManager->skeletonCache);
        showAssetGroup("Scenes", vke_common::ASSET_SCENE, assetManager->sceneCache);
        showAssetGroup("Fonts", vke_common::ASSET_FONT, assetManager->fontCache);
        showAssetGroup("Animations", vke_common::ASSET_ANIMATION, assetManager->animationCache);
    }

    void Editor::showAssetsByDirectory(vke_common::AssetManager *assetManager)
    {
        if (assetDirectoryTree.name.empty())
            rebuildAssetDirectoryTree(assetManager);

        if (assetDirectoryTree.children.empty() && assetDirectoryTree.assets.empty())
        {
            ImGui::TextDisabled("No assets");
            return;
        }
        drawAssetDirectoryNode(assetDirectoryTree);
    }

    void Editor::rebuildAssetDirectoryTree(vke_common::AssetManager *assetManager)
    {
        assetDirectoryTree = {};
        assetDirectoryTree.name = "Assets";
        AddAssetsToDirectoryTree(assetDirectoryTree, vke_common::ASSET_TEXTURE, assetManager->textureCache);
        AddAssetsToDirectoryTree(assetDirectoryTree, vke_common::ASSET_MESH, assetManager->meshCache);
        AddAssetsToDirectoryTree(assetDirectoryTree, vke_common::ASSET_VF_SHADER, assetManager->vfShaderCache);
        AddAssetsToDirectoryTree(assetDirectoryTree, vke_common::ASSET_COMPUTE_SHADER, assetManager->computeShaderCache);
        AddAssetsToDirectoryTree(assetDirectoryTree, vke_common::ASSET_MATERIAL, assetManager->materialCache);
        AddAssetsToDirectoryTree(assetDirectoryTree, vke_common::ASSET_SKELETON, assetManager->skeletonCache);
        AddAssetsToDirectoryTree(assetDirectoryTree, vke_common::ASSET_ANIMATION, assetManager->animationCache);
        AddAssetsToDirectoryTree(assetDirectoryTree, vke_common::ASSET_SCENE, assetManager->sceneCache);
        AddAssetsToDirectoryTree(assetDirectoryTree, vke_common::ASSET_FONT, assetManager->fontCache);
    }

    void Editor::drawAssetDirectoryNode(const AssetTreeNode &node)
    {
        for (const auto &entry : node.assets)
            drawAssetEntry(entry);

        for (const auto &[name, child] : node.children)
        {
            ImGui::PushID(name.c_str());
            if (ImGui::TreeNodeEx(child.name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick))
            {
                drawAssetDirectoryNode(child);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    void Editor::drawAssetEntry(const AssetTreeEntry &entry)
    {
        ImGui::PushID(static_cast<int>(entry.type));
        ImGui::PushID(static_cast<int>(entry.id));
        const std::string label = "[" + std::string(AssetTypeName(entry.type)) + "] " +
                                  std::to_string(entry.id) + "  " + entry.name;
        const bool selected = selectedAssetType == entry.type && selectedAsset == entry.id;
        if (ImGui::Selectable(label.c_str(), selected))
            selectAsset(entry.type, entry.id);

        if (!entry.path.empty() && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", entry.path.c_str());
        ImGui::PopID();
        ImGui::PopID();
    }

    void Editor::selectAsset(vke_common::AssetType assetType, vke_common::AssetHandle asset)
    {
        AssetTreeEntry entry;
        if (!GetAssetInfo(assetType, asset, entry))
            return;

        selectedEntity = entt::null;
        selectedAssetType = assetType;
        selectedAsset = asset;
        if (assetType == vke_common::ASSET_TEXTURE)
            getTexturePreviewDescriptorSet(asset);
    }

    void Editor::clearSelectedAsset()
    {
        selectedAssetType = vke_common::ASSET_CNT_FLAG;
        selectedAsset = 0;
    }

    void Editor::disposeTexturePreviewDescriptorSets()
    {
        for (auto &[asset, descriptorSet] : texturePreviewDescriptorSets)
        {
            if (descriptorSet != VK_NULL_HANDLE)
                ImGui_ImplVulkan_RemoveTexture(descriptorSet);
        }
        texturePreviewDescriptorSets.clear();
        if (selectedAssetType == vke_common::ASSET_TEXTURE)
        {
            selectedAssetType = vke_common::ASSET_CNT_FLAG;
            selectedAsset = 0;
        }
    }

    void Editor::showSelectedTextureInspector()
    {
        vke_common::TextureAsset *asset = vke_common::AssetManager::GetTextureAsset(selectedAsset);
        if (asset == nullptr)
        {
            clearSelectedAsset();
            ImGui::TextUnformatted("No object selected");
            return;
        }

        std::shared_ptr<vke_render::Texture2D> texture = vke_common::AssetManager::LoadTexture2D(selectedAsset);
        VkDescriptorSet descriptorSet = getTexturePreviewDescriptorSet(selectedAsset);
        if (texture == nullptr || descriptorSet == VK_NULL_HANDLE)
        {
            ImGui::TextUnformatted("Texture preview unavailable");
            return;
        }

        ImGui::Text("Texture: %s", asset->name.c_str());
        ImGui::Text("ID: %llu", static_cast<unsigned long long>(asset->id));
        ImGui::Text("Path: %s", asset->path.c_str());
        ImGui::Text("Size: %u x %u", texture->width, texture->height);
        ImGui::Separator();

        const float availableWidth = ImGui::GetContentRegionAvail().x;
        const float imageWidth = std::min(availableWidth, 512.0f);
        const float aspect = texture->width == 0 ? 1.0f : static_cast<float>(texture->height) / static_cast<float>(texture->width);
        ImGui::Image(
            ImTextureRef((ImTextureID)descriptorSet),
            ImVec2(imageWidth, imageWidth * aspect));
    }

    void Editor::showSelectedMaterialInspector()
    {
        vke_common::MaterialAsset *asset = vke_common::AssetManager::GetMaterialAsset(selectedAsset);
        if (asset == nullptr)
        {
            clearSelectedAsset();
            ImGui::TextUnformatted("No object selected");
            return;
        }

        ImGui::Text("Material: %s", asset->name.c_str());
        ImGui::Text("ID: %llu", static_cast<unsigned long long>(asset->id));
        ImGui::Text("Path: %s", asset->path.c_str());
        ImGui::Text("Shader: %llu", static_cast<unsigned long long>(asset->shader));
        ImGui::Text("Render Mode: %s", RenderModeName(asset->renderMode));
        ImGui::Text("Blend Mode: %s", BlendModeName(asset->blendMode));

        if (ImGui::TreeNodeEx("Textures", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (asset->textures.empty())
                ImGui::TextDisabled("No textures");
            for (size_t i = 0; i < asset->textures.size(); ++i)
            {
                const vke_common::AssetHandle textureHandle = asset->textures[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::Text("%zu: %s", i, TextureDisplayName(textureHandle).c_str());
                drawTexturePreview(textureHandle, 128.0f);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Texture Bindings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (asset->textureBindingInfos == nullptr || asset->textureBindingInfos->empty())
                ImGui::TextDisabled("No texture bindings");
            else
            {
                for (size_t i = 0; i < asset->textureBindingInfos->size(); ++i)
                {
                    const vke_render::TextureBindingInfo &info = (*asset->textureBindingInfos)[i];
                    ImGui::Text(
                        "%zu: binding=%u offset=%u count=%u",
                        i,
                        info.binding,
                        info.offset,
                        info.cnt);
                }
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Push Constants", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (asset->pushConstantInfos == nullptr || asset->pushConstantInfos->empty())
                ImGui::TextDisabled("No push constants");
            else
            {
                for (size_t i = 0; i < asset->pushConstantInfos->size(); ++i)
                {
                    const vke_render::PushConstantInfo &info = (*asset->pushConstantInfos)[i];
                    ImGui::PushID(static_cast<int>(i));
                    const std::string label = "Constant " + std::to_string(i);
                    if (ImGui::TreeNode(label.c_str()))
                    {
                        ImGui::Text("Offset: %u", info.offset);
                        ImGui::Text("Size: %u bytes", info.size);
                        ImGui::Text("Type: %s", info.isFloat ? "float" : "int");

                        const uint32_t valueCount = info.size / sizeof(uint32_t);
                        if (info.pValues == nullptr)
                        {
                            ImGui::TextDisabled("No values");
                            ImGui::TreePop();
                            ImGui::PopID();
                            continue;
                        }

                        for (uint32_t valueIndex = 0; valueIndex < valueCount; ++valueIndex)
                        {
                            const std::string valueLabel = "[" + std::to_string(valueIndex) + "]";
                            uint32_t *rawValue = static_cast<uint32_t *>(const_cast<void *>(info.pValues)) + valueIndex;
                            if (info.isFloat)
                            {
                                float value;
                                std::memcpy(&value, rawValue, sizeof(value));
                                if (ImGui::InputFloat(valueLabel.c_str(), &value, 0.0f, 0.0f, "%.6f"))
                                    std::memcpy(rawValue, &value, sizeof(value));
                            }
                            else
                            {
                                int32_t value;
                                std::memcpy(&value, rawValue, sizeof(value));
                                if (ImGui::InputInt(valueLabel.c_str(), &value))
                                    std::memcpy(rawValue, &value, sizeof(value));
                            }
                        }

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::TreePop();
        }
    }

    void Editor::showSelectedAssetInspector()
    {
        AssetTreeEntry entry;
        if (!GetAssetInfo(selectedAssetType, selectedAsset, entry))
        {
            clearSelectedAsset();
            ImGui::TextUnformatted("No object selected");
            return;
        }

        ImGui::Text("Asset: %s", entry.name.c_str());
        ImGui::Text("Type: %s", AssetTypeName(entry.type));
        ImGui::Text("ID: %llu", static_cast<unsigned long long>(entry.id));
        ImGui::Text("Path: %s", entry.path.c_str());

        vke_common::AssetManager *assetManager = vke_common::AssetManager::GetInstance();
        switch (entry.type)
        {
        case vke_common::ASSET_VF_SHADER:
            if (auto it = assetManager->vfShaderCache.find(entry.id); it != assetManager->vfShaderCache.end())
                ImGui::Text("Fragment Path: %s", it->second.fragPath.c_str());
            break;
        case vke_common::ASSET_FONT:
            if (auto it = assetManager->fontCache.find(entry.id); it != assetManager->fontCache.end())
            {
                ImGui::Text("Pixel Size: %u", it->second.pixelSize);
                ImGui::Text("First Codepoint: %u", it->second.firstCodepoint);
                ImGui::Text("Character Count: %u", it->second.characterCount);
            }
            break;
        default:
            break;
        }
    }
}
