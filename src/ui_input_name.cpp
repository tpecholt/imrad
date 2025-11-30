// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#include "ui_input_name.h"
#include "ui_message_box.h"

InputName inputName;


void InputName::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImRad::GetUserData().dimBgRatio = 1.f;
    ImGui::OpenPopup(ID);
    Init();
}

void InputName::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    ImRad::GetUserData().dimBgRatio = 0.f;
}

void InputName::Init()
{
    // TODO: Add your code here
    name = "";
}

void InputName::Draw()
{
    /// @dpi-info 141.357,1.25
    /// @style imrad
    /// @unit px
    /// @begin TopWindow
    ID = ImGui::GetID("###InputName");
    ImGui::SetNextWindowSize({ 0, 0 }); //{ 250, 120 }
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal(ImRad::Format("{}###InputName", title).c_str(), &tmpOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
        if (ImRad::GetUserData().activeActivity != "")
            ImRad::RenderDimmedBackground(ImRad::GetUserData().WorkRect(), ImRad::GetUserData().dimBgRatio);
        if (modalResult != ImRad::None)
        {
            ImGui::CloseCurrentPopup();
            if (modalResult != ImRad::Cancel)
                callback(modalResult);
        }
        if (ImGui::Shortcut(ImGuiKey_Escape))
            ClosePopup();
        /// @separator

        // TODO: Add Draw calls of dependent popup windows here
        messageBox.Draw();

        /// @begin Input
        ImGui::SetNextItemWidth(300);
        ImGui::InputTextWithHint("##name", ImRad::Format("{}", hint).c_str(), &name, ImGuiInputTextFlags_CharsNoBlank);
        if (ImGui::IsItemActive())
            ImRad::GetUserData().imeType = ImRad::ImeText;
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere(-1);
        /// @end Input

        /// @begin Table
        ImRad::Spacing(3);
        if (ImGui::BeginTable("table1", 3, ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, { -1, 0 }))
        {
            ImGui::TableSetupColumn("left-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableSetupColumn("right-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupScrollFreeze(0, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Button
            ImRad::TableNextColumn(1);
            ImGui::BeginDisabled(name==""||name=="Classic"||name=="Dark"||name=="Light");
            if (ImGui::Button("OK", { 100, 30 }))
            {
                ClosePopup(ImRad::Ok);
            }
            ImGui::EndDisabled();
            /// @end Button

            /// @separator
            ImGui::EndTable();
        }
        /// @end Table

        /// @separator
        ImGui::EndPopup();
    }
    /// @end TopWindow
}
