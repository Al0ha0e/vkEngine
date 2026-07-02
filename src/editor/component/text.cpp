#include <editor/editor.hpp>
#include <component/text.hpp>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace vke_editor
{
    static int TextResizeCallback(ImGuiInputTextCallbackData *data)
    {
        if (data->EventFlag != ImGuiInputTextFlags_CallbackResize)
            return 0;

        std::string *text = static_cast<std::string *>(data->UserData);
        text->resize(data->BufTextLen);
        data->Buf = text->data();
        return 0;
    }

    static bool InputTextMultilineString(const char *label, std::string &text, const ImVec2 &size)
    {
        text.reserve(text.size() + 1);
        return ImGui::InputTextMultiline(
            label,
            text.data(),
            text.capacity() + 1,
            size,
            ImGuiInputTextFlags_CallbackResize,
            TextResizeCallback,
            &text);
    }

    void Editor::drawUITextComponent(vke_common::Scene *scene)
    {
        if (scene == nullptr || selectedEntity == entt::null ||
            !scene->registry.all_of<vke_component::UIText>(selectedEntity))
            return;

        if (!ImGui::TreeNodeEx("UIText", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        vke_component::UIText &text = scene->registry.get<vke_component::UIText>(selectedEntity);

        std::string textValue = text.GetText();
        const float textHeight = ImGui::GetTextLineHeight() * 6.0f;
        if (InputTextMultilineString("Text", textValue, ImVec2(-FLT_MIN, textHeight)))
            text.SetText(textValue);

        glm::vec4 color = text.GetColor();
        if (ImGui::ColorEdit4("Color", glm::value_ptr(color)))
            text.SetColor(glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f)));

        ImGui::TreePop();
    }
}
