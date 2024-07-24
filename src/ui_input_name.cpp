// Generated with ImRAD 0.8
// visit https://github.com/tpecholt/imrad

#include "ui_input_name.h"
#include "ui_message_box.h"

InputName inputName;


void InputName::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 1.f;
    ImGui::OpenPopup(ID);
    Init();
}

void InputName::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 0.f;
}

void InputName::Init()
{
	// TODO: Add your code here
	name = "";
}

void InputName::Draw()
{
    /// @style imrad
    /// @unit px
    /// @begin TopWindow
    auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;
    ID = ImGui::GetID("###InputName");
    ImGui::SetNextWindowSize({ 250, 120 }, ImGuiCond_FirstUseEver); //{ 250, 120 }
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal(ImRad::Format("{}###InputName", title).c_str(), &tmpOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        if (ioUserData->activeActivity != "")
            ImRad::RenderDimmedBackground(ioUserData->WorkRect(), ioUserData->dimBgRatio);
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
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##name", ImRad::Format("{}", hint).c_str(), &name, ImGuiInputTextFlags_CharsNoBlank);
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
