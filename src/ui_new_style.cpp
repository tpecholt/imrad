// Generated with ImRAD 0.10-WIP
// visit https://github.com/tpecholt/imrad

#include "ui_new_style.h"
#include "ui_message_box.h"

NewStyleDlg newStyleDlg;


void NewStyleDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImRad::GetUserData().dimBgRatio = 1.f;
    IM_ASSERT(ID && "Call Draw at least once to get ID assigned");
    ImGui::OpenPopup(ID);
    Init();
}

void NewStyleDlg::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    ImRad::GetUserData().dimBgRatio = 0.f;
}

void NewStyleDlg::Init()
{
    // TODO: Add your code here
    name = "";
}

void NewStyleDlg::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    hb5.Reset();
}

void NewStyleDlg::Draw()
{
    /// @dpi-info 141.357,1.25
    /// @style imrad
    /// @unit px
    /// @begin TopWindow
    ID = ImGui::GetID("###NewStyleDlg");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10, 10 });
    ImGui::SetNextWindowSize({ 0, 0 }); //{ 250, 120 }
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal(ImRad::Format("{}###NewStyleDlg", title).c_str(), &tmpOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
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

        /// @begin Text
        ImGui::TextUnformatted("Name:");
        /// @end Text

        // TODO: Add Draw calls of dependent popup windows here
        messageBox.Draw();

        /// @begin Input
        ImGui::SetNextItemWidth(250);
        ImGui::InputText("##name", &name, ImGuiInputTextFlags_CharsNoBlank);
        if (ImGui::IsItemActive())
            ImRad::GetUserData().imeType = ImRad::ImeText;
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere(-1);
        /// @end Input

        /// @begin Text
        ImRad::Spacing(1);
        ImGui::TextUnformatted("Copy from:");
        /// @end Text

        /// @begin Combo
        ImGui::SetNextItemWidth(250);
        ImRad::Combo("##source", &source, sources, 0);
        /// @end Combo

        /// @begin Spacer
        hb5.BeginLayout();
        ImRad::Spacing(3);
        ImRad::Dummy({ hb5.GetSize(), 20 });
        hb5.AddSize(0 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::Stretch(1.0f));
        /// @end Spacer

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(name==""||name=="Classic"||name=="Dark"||name=="Light");
        if (ImGui::Button("OK", { 100, 30 }))
        {
            ClosePopup(ImRad::Ok);
        }
        hb5.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 100);
        ImGui::EndDisabled();
        /// @end Button

        /// @separator
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    /// @end TopWindow
}
