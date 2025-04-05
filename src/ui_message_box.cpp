#include "ui_message_box.h"
#include "IconsFontAwesome6.h"

MessageBox messageBox;

std::string WordWrap(const std::string& str, float multilineWidth) 
{
    float textSize = 0;
    std::string tmpStr = "";
    std::string finalStr = "";
    int curChr = 0;
    while (curChr < str.size()) {

        if (str[curChr] == '\n') {
            finalStr += tmpStr + "\n";
            tmpStr = "";
        }
        else
            tmpStr += str[curChr];
        textSize = ImGui::CalcTextSize(tmpStr.c_str()).x;

        if (textSize > multilineWidth) {
            int lastSpace = (int)tmpStr.size() - 1;
            while (tmpStr[lastSpace] != ' ' && lastSpace > 0)
                lastSpace--;
            if (lastSpace == 0)
                lastSpace = (int)tmpStr.size() - 2;
            finalStr += tmpStr.substr(0, lastSpace + 1) + "\n";
            if (lastSpace + 1 > tmpStr.size())
                tmpStr = "";
            else
                tmpStr = tmpStr.substr(lastSpace + 1);
        }
        curChr++;
    }
    if (tmpStr.size() > 0)
        finalStr += tmpStr;

    return finalStr;
};

void MessageBox::OpenPopup(std::function<void(ImRad::ModalResult)> f)
{
    callback = std::move(f);
    ImGui::OpenPopup(ID);
}

int TreeNode(const std::string& str, size_t& i, int level, bool skip)
{
    struct ScopeGuard {
        ScopeGuard(std::function<void()> f) : func(f) {}
        ~ScopeGuard() { func(); }
        std::function<void()> func;
    };
    for (int n = 0; i < str.size(); ++n)
    {
        ImGui::PushID(n);
        ScopeGuard g([] { ImGui::PopID(); });

        size_t j = str.find("\n", i);
        int nlevel = 0;
        std::string label;
        if (j == std::string::npos) {
            label = str.substr(i);
            i = str.size();
        }
        else {
            label = str.substr(i, j - i);
            i = j + 1;
            for (nlevel = 0; i < str.size() && str[i] == '\t'; ++i)
                ++nlevel;
        }
        
        if (nlevel > level) {
            bool open = true;
            if (!skip) {
                ImGui::PushStyleColor(ImGuiCol_Text, !level ? 0xff2040c0 : 0xff000000);
                open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
                ImGui::PopStyleColor();
            }
            nlevel = TreeNode(str, i, nlevel, skip || !open);
            if (nlevel < level) {
                if (!skip && open)
                    ImGui::TreePop();
                return nlevel;
            }
        }
        else if (nlevel < level) {
            if (!skip)
                ImGui::TreePop();
            return nlevel;
        }
        else if (!skip) {
            ImVec2 pad = ImGui::GetStyle().FramePadding;
            if (!level) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                if (ImGui::TreeNodeEx("", 0))
                    ImGui::TreePop();
                ImGui::PopItemFlag();
                ImGui::SameLine();
            }
            else {
                ImGui::Text("-");
                ImGui::SameLine(0, pad.x * 3);
            }
            float w = ImGui::GetContentRegionAvail().x - pad.x;
            label = WordWrap(label, w);
            ImGui::Text("%s", label.c_str());
        }
    }
    return 0;
}

void MessageBox::Draw()
{
    const float BWIDTH = 90;
    const float BHEIGHT = 30;

    ID = ImGui::GetID("###MessageBox");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    //todo: ImGuiCond_Appearing won't center
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { 0.5f, 0.5f });
    bool open = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10, 10 });
    int fl = error != "" ? 0 : ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::BeginPopupModal((title + "###MessageBox").c_str(), &open, fl))
    {
        wasOpen = ID;
        /*bool resizing = ImGui::IsWindowAppearing() ||
            std::abs(ImGui::GetWindowSize().x - lastSize.x) >= 1 ||
            std::abs(ImGui::GetWindowSize().y - lastSize.y) >= 1;
        lastSize = ImGui::GetWindowSize();*/

        if (error != "")
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(192, 96, 0, 255));
            ImGui::Text(" " ICON_FA_CIRCLE_EXCLAMATION " ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextWrapped("%s", message.c_str());
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
            ImVec2 sp = ImGui::GetStyle().ItemSpacing;
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(192, 192, 192, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, sp.y });
            ImGui::BeginChild("##errors", { -1.f, -BHEIGHT - 3.f*sp.y }, ImGuiChildFlags_Borders);
            ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, ImGui::GetStyle().ItemSpacing.x / 2);
            size_t i = 0;
            TreeNode(error, i, 0, false);
            ImGui::PopStyleVar();
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            /*if (resizing)
            {
                float sw = ImGui::GetStyle().ScrollbarSize;
                float w = ImGui::CalcItemSize({ -sp.x - sw, 0 }, 0, 0).x;
                wrappedError = WordWrap(error, w);
            }
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(192, 192, 192, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
            ImGui::InputTextMultiline("##err", &wrappedError, { -1.f, -BHEIGHT - 3 * sp.y }, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);*/
        }
        else
        {
            ImGui::BeginChild("ch", { 300, 50 });
            ImGui::TextWrapped("%s", message.c_str());
            ImGui::EndChild();
        }
        ImGui::Spacing();
        ImGui::Spacing();

        int n = (bool)(buttons & ImRad::Ok) + (bool)(buttons & ImRad::Cancel) + (bool)(buttons & ImRad::Yes) + (bool)(buttons & ImRad::No);
        float w = (n * BWIDTH) + (n - 1) * ImGui::GetStyle().ItemSpacing.x;
        float x = (ImGui::GetContentRegionAvail().x + ImGui::GetStyle().FramePadding.x - w) / 2.f;
        x += ImGui::GetStyle().WindowPadding.x;
        float y = ImGui::GetCursorPosY();
        if (buttons & ImRad::Ok)
        {
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();
            ImGui::SetCursorPos({ x, y });
            if (ImGui::Button("OK", { BWIDTH, BHEIGHT }) ||
                (buttons == ImRad::Ok && ImGui::IsKeyPressed(ImGuiKey_Escape)))
            {
                ImGui::CloseCurrentPopup();
                callback(ImRad::Ok);
            }
            x += BWIDTH + ImGui::GetStyle().ItemSpacing.x;
        }
        if (buttons & ImRad::Yes)
        {
            ImGui::SetCursorPos({ x, y });
            if (ImGui::Button("Yes", { BWIDTH, BHEIGHT })) {
                ImGui::CloseCurrentPopup();
                callback(ImRad::Yes);
            }
            x += BWIDTH + ImGui::GetStyle().ItemSpacing.x;
        }
        if (buttons & ImRad::No)
        {
            ImGui::SetCursorPos({ x, y });
            if (ImGui::Button("No", { BWIDTH, BHEIGHT })) {
                ImGui::CloseCurrentPopup();
                callback(ImRad::No);
            }
            x += BWIDTH + ImGui::GetStyle().ItemSpacing.x;
        }
        if (buttons & ImRad::Cancel)
        {
            ImGui::SetCursorPos({ x, y });
            if (ImGui::Button("Cancel", { BWIDTH, BHEIGHT }) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();
                callback(ImRad::Cancel); //currently used in Save File Confirmation upon Exit
            }
            x += BWIDTH + ImGui::GetStyle().ItemSpacing.x;
        }

        ImGui::EndPopup();
    }
    else if (wasOpen == ID)
    {
        //hack - clear fields for next invocation
        wasOpen = 0;
        error = "";
        title = "title";
        message = "";
        buttons = ImRad::Ok;
    }
    ImGui::PopStyleVar();
}