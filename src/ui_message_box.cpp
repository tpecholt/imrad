#include "ui_message_box.h"
#include "IconsFontAwesome6.h"

MessageBox messageBox;

void MessageBox::OpenPopup(std::function<void(ImRad::ModalResult)> f)
{
    callback = std::move(f);
    ImGui::OpenPopup(ID);
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
    if (ImGui::BeginPopupModal((title + "###MessageBox").c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        wasOpen = ID;
        /*bool resizing = ImGui::IsWindowAppearing() ||
            std::abs(ImGui::GetWindowSize().x - lastSize.x) >= 1 ||
            std::abs(ImGui::GetWindowSize().y - lastSize.y) >= 1;
        lastSize = ImGui::GetWindowSize();*/

        const float WIDTH = 300;
        const char* ICON[] = { "", ICON_FA_CIRCLE_INFO, ICON_FA_TRIANGLE_EXCLAMATION, ICON_FA_CIRCLE_EXCLAMATION };
        const ImU32 ICONCLR[] = { 0, 0xffc03030, 0xff00b0b0, 0xff3030a0 };
        ImVec2 szm = ImGui::CalcTextSize(message.c_str(), 0, false, WIDTH);
        float height = std::max(30.f, szm.y);
        if (icon != None) {
            ImGui::PushFont(nullptr, 25.f);
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0.5 });
            ImGui::PushStyleColor(ImGuiCol_Text, ICONCLR[icon]);
            ImRad::Selectable(ICON[icon], false, 0, { 0, height });
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::PopItemFlag();
            ImGui::PopFont();
            ImGui::SameLine(0, 2 * ImGui::GetStyle().ItemSpacing.x);
        }
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (height - szm.y) / 2);
        ImGui::BeginChild("ch", { WIDTH, height + 3 * ImGui::GetStyle().ItemSpacing.y });
        ImGui::TextWrapped("%s", message.c_str());
        ImGui::EndChild();
        ImGui::Spacing();

        int n = (bool)(buttons & ImRad::Ok) + (bool)(buttons & ImRad::Cancel) + (bool)(buttons & ImRad::Yes) + (bool)(buttons & ImRad::No);
        float w = (n * BWIDTH) + (n - 1) * ImGui::GetStyle().ItemSpacing.x;
        float x = (ImGui::GetContentRegionAvail().x + ImGui::GetStyle().FramePadding.x - w) / 2.f;
        x += ImGui::GetStyle().WindowPadding.x;
        ImGui::SetCursorPosX(x);
        if (buttons & ImRad::Ok)
        {
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();
            if (ImGui::Button("OK", { BWIDTH, BHEIGHT }) ||
                (buttons == ImRad::Ok && ImGui::IsKeyPressed(ImGuiKey_Escape)))
            {
                ImGui::CloseCurrentPopup();
                callback(ImRad::Ok);
            }
            ImGui::SameLine();
        }
        if (buttons & ImRad::Yes)
        {
            if (ImGui::Button("Yes", { BWIDTH, BHEIGHT })) {
                ImGui::CloseCurrentPopup();
                callback(ImRad::Yes);
            }
            ImGui::SameLine();
        }
        if (buttons & ImRad::No)
        {
            if (ImGui::Button("No", { BWIDTH, BHEIGHT })) {
                ImGui::CloseCurrentPopup();
                callback(ImRad::No);
            }
            ImGui::SameLine();
        }
        if (buttons & ImRad::Cancel)
        {
            if (ImGui::Button("Cancel", { BWIDTH, BHEIGHT }) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();
                callback(ImRad::Cancel); //currently used in Save File Confirmation upon Exit
            }
            ImGui::SameLine();
        }

        ImGui::EndPopup();
    }
    else if (wasOpen == ID)
    {
        //hack - clear fields for next invocation
        wasOpen = 0;
        icon = None;
        title = "title";
        message = "";
        buttons = ImRad::Ok;
    }
    ImGui::PopStyleVar();
}