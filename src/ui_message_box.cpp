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

        ImGui::BeginChild("ch", { 300, 50 });
        ImGui::TextWrapped("%s", message.c_str());
        ImGui::EndChild();
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
        title = "title";
        message = "";
        buttons = ImRad::Ok;
    }
    ImGui::PopStyleVar();
}