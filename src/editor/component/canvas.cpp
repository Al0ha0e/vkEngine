#include <editor/editor.hpp>
#include <component/canvas.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cfloat>

namespace vke_editor
{
    static int textResizeCallback(ImGuiInputTextCallbackData *data)
    {
        auto &text = *static_cast<std::string *>(data->UserData);
        text.resize(data->BufTextLen);
        data->Buf = text.data();
        return 0;
    }

    static bool inputString(const char *label, std::string &text, bool multiline = false)
    {
        text.reserve(text.size() + 1);
        if (multiline)
            return ImGui::InputTextMultiline(label, text.data(), text.capacity() + 1,
                                             ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 4), ImGuiInputTextFlags_CallbackResize, textResizeCallback, &text);
        return ImGui::InputText(label, text.data(), text.capacity() + 1,
                                ImGuiInputTextFlags_CallbackResize, textResizeCallback, &text);
    }

    void Editor::drawCanvasComponent()
    {
        using namespace vke_component;
        using namespace vke_ui;
        auto *canvas = sceneManager->registry.try_get<Canvas>(selectedEntity);
        if (!canvas || !ImGui::TreeNodeEx("Canvas", ImGuiTreeNodeFlags_DefaultOpen))
            return;
        ImGui::TextUnformatted("Screen Space");
        ImGui::TextDisabled("UI coordinates are relative to the viewport.");
        auto resolution = canvas->GetReferenceResolution();
        if (ImGui::InputFloat2("Reference resolution", glm::value_ptr(resolution)))
            if (auto result = canvas->SetReferenceResolution(resolution); !result)
                VKE_LOG_ERROR("Invalid Canvas resolution: {}", result.error());
        ImGui::InputInt("Sort order", &canvas->sortOrder);

        auto addNode = [&](UIHandle parent, bool text)
        {
            std::unique_ptr<UINodeData> data;
            if (text)
            {
                auto textData = std::make_unique<UITextData>();
                textData->text = "Text";
                data = std::move(textData);
            }
            else
                data = std::make_unique<UIContainerData>();
            data->name = text ? "Text" : "Container";
            data->rectTransform.offsetMax = {400, 80};
            if (auto result = canvas->Add(*data, parent); !result)
                VKE_LOG_ERROR("Failed to add UI node: {}", result.error());
        };
        if (ImGui::Button("Add Container"))
            addNode(0, false);
        ImGui::SameLine();
        if (ImGui::Button("Add Text"))
            addNode(0, true);

        std::function<void()> pendingEdit;
        std::function<void(UIComponent &, size_t, size_t)> drawNode;
        drawNode = [&](UIComponent &node, size_t index, size_t siblingCount)
        {
            const auto handle = node.handle;
            ImGui::PushID(&node);
            const bool opened = ImGui::TreeNodeEx("node", ImGuiTreeNodeFlags_DefaultOpen, "%s (%s)",
                                                  node.name.c_str(), node.GetType() == UIType::Text ? "Text" : "Container");
            if (opened)
            {
                inputString("Name", node.name);
                auto rect = node.rectTransform;
                bool changed = ImGui::InputFloat2("Anchor min", glm::value_ptr(rect.anchorMin));
                changed |= ImGui::InputFloat2("Anchor max", glm::value_ptr(rect.anchorMax));
                changed |= ImGui::InputFloat2("Offset min", glm::value_ptr(rect.offsetMin));
                changed |= ImGui::InputFloat2("Offset max", glm::value_ptr(rect.offsetMax));
                changed |= ImGui::InputFloat2("Pivot", glm::value_ptr(rect.pivot));
                changed |= ImGui::InputFloat2("Scale", glm::value_ptr(rect.scale));
                float degrees = glm::degrees(rect.rotation);
                if (ImGui::InputFloat("Rotation (degrees)", &degrees))
                {
                    rect.rotation = glm::radians(degrees);
                    changed = true;
                }
                if (changed)
                    if (auto result = node.SetRectTransform(rect); !result)
                        VKE_LOG_ERROR("Invalid UI rectangle: {}", result.error());
                const auto &size = node.resolvedSize;
                ImGui::Text("Resolved size: %.1f x %.1f", size.x, size.y);
                if (node.GetType() == UIType::Text)
                {
                    auto &text = static_cast<UIText &>(node);
                    auto value = text.GetText();
                    if (inputString("Text", value, true))
                        text.SetText(value);
                    auto color = text.GetColor();
                    if (ImGui::ColorEdit4("Color", glm::value_ptr(color)))
                        text.SetColor(color);
                }
                auto *parent = node.parent;
                if (ImGui::BeginCombo("Parent", parent ? parent->name.c_str() : "Canvas"))
                {
                    auto chooseParent = [&](UIHandle target)
                    {
                        pendingEdit = [canvas, handle, target]()
                        {
                            if (auto result = canvas->SetParent(handle, target); !result)
                                VKE_LOG_ERROR("Cannot reparent UI node: {}", result.error());
                        };
                    };
                    if (ImGui::Selectable("Canvas", !parent))
                        chooseParent(0);
                    std::function<void(const UIComponent &)> drawChoice = [&](const UIComponent &candidate)
                    {
                        if (&candidate == &node)
                            return;
                        ImGui::PushID(&candidate);
                        if (ImGui::Selectable(candidate.name.c_str(), parent == &candidate))
                            chooseParent(candidate.handle);
                        ImGui::PopID();
                        for (const auto &child : candidate.children)
                            drawChoice(*child);
                    };
                    for (const auto &root : canvas->GetChildren())
                        drawChoice(*root);
                    ImGui::EndCombo();
                }
                ImGui::BeginDisabled(index == 0);
                if (ImGui::Button("Move up"))
                    pendingEdit = [canvas, handle, index]()
                    { canvas->SetSiblingIndex(handle, index - 1); };
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::BeginDisabled(index + 1 == siblingCount);
                if (ImGui::Button("Move down"))
                    pendingEdit = [canvas, handle, index]()
                    { canvas->SetSiblingIndex(handle, index + 1); };
                ImGui::EndDisabled();
                if (ImGui::Button("Add Container"))
                    addNode(handle, false);
                ImGui::SameLine();
                if (ImGui::Button("Add Text"))
                    addNode(handle, true);
                ImGui::SameLine();
                if (ImGui::Button("Delete subtree"))
                    pendingEdit = [canvas, handle]()
                    { canvas->Remove(handle); };
                const auto &children = node.children;
                for (size_t i = 0; i < children.size(); ++i)
                    drawNode(*children[i], i, children.size());
                ImGui::TreePop();
            }
            ImGui::PopID();
        };
        const auto &roots = canvas->GetChildren();
        for (size_t i = 0; i < roots.size(); ++i)
            drawNode(*roots[i], i, roots.size());
        if (pendingEdit)
            pendingEdit();
        ImGui::TreePop();
    }
}
