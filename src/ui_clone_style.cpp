// Generated with ImRAD 0.7
// visit https://github.com/tpecholt/imrad

#include "ui_clone_style.h"
#include "ui_message_box.h"

CloneStyle cloneStyle;


void CloneStyle::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImGui::OpenPopup(ID);
    Init();
}

void CloneStyle::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
}

void CloneStyle::Init()
{
	// TODO: Add your code here
	styleName = "";
}

void CloneStyle::Draw()
{
    /// @style Dark
    /// @unit px
    /// @begin TopWindow
    auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;
    ID = ImGui::GetID("###CloneStyle");
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 0, { 0.5f, 0.5f }); //Center
    ImGui::SetNextWindowSize({ 250, 120 });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Clone Style###CloneStyle", &tmpOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        if (modalResult != ImRad::None)
        {
            ImGui::CloseCurrentPopup();
            if (modalResult != ImRad::Cancel) callback(modalResult);
        }
        /// @separator

        // TODO: Add Draw calls of dependent popup windows here
		messageBox.Draw();

        /// @begin Input
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##styleName", "New style name", &styleName, ImGuiInputTextFlags_CharsNoBlank);
        if (ImGui::IsItemActive())
            ioUserData->imeType = ImRad::ImeText;
        /// @end Input

        /// @begin Table
        ImRad::Spacing(3);
        if (ImGui::BeginTable("table1", 3, ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, { 0, 0 }))
        {
            ImGui::TableSetupColumn("left-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableSetupColumn("right-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Button
            ImRad::TableNextColumn(1);
            ImGui::BeginDisabled(styleName==""||styleName=="Classic"||styleName=="Dark"||styleName=="Light");
            if (ImGui::Button("OK", { 90, 30 }) ||
                (!ImRad::IsItemDisabled() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)))
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
