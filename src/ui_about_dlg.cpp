// Generated with ImRAD 0.1
// github.com/xyz

#include "ui_about_dlg.h"
#include "utils.h"

AboutDlg aboutDlg;

void AboutDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImGui::OpenPopup(ID);
    Init();
}

void AboutDlg::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
}

void AboutDlg::Draw()
{
    /// @style Dark
    /// @unit px
    /// @begin TopWindow
    auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;
    ID = ImGui::GetID("###AboutDlg");
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10, 10 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10, 7 });
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 0, { 0.5f, 0.5f }); //Center
    ImGui::SetNextWindowSize({ 0, 0 });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("About###AboutDlg", &tmpOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (modalResult != ImRad::None)
        {
            ImGui::CloseCurrentPopup();
            if (modalResult != ImRad::Cancel) callback(modalResult);
        }
        /// @separator

        // TODO: Add Draw calls of dependent popup windows here

        /// @begin Text
        ImRad::Spacing(1);
        ImGui::Indent(1 * ImGui::GetStyle().IndentSpacing / 2);
        ImGui::PushFont(ImRad::GetFontByName("imrad.H1"));
        ImGui::TextUnformatted(ImRad::Format("{}", VER_STR).c_str());
        ImGui::PopFont();
        /// @end Text

        /// @begin Text
        ImGui::TextUnformatted(ImRad::Format("build with ImGui {}", IMGUI_VERSION).c_str());
        /// @end Text

        /// @begin Text
        ImRad::Spacing(2);
        ImGui::PushStyleColor(ImGuiCol_Text, 0xffff6633);
        ImGui::TextUnformatted(ImRad::Format("{}", GITHUB_URL).c_str());
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(7);
        if (ImGui::IsItemClicked())
            OpenURL();
        /// @end Text

        /// @begin Child
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginChild("child1", { ImGui::GetStyle().ItemSpacing.x, 10 }, ImGuiChildFlags_None, ImGuiWindowFlags_NoSavedSettings);
        {
            /// @separator

            /// @separator
            ImGui::EndChild();
        }
        /// @end Child

        /// @begin Child
        ImGui::BeginChild("child2", { -100, 10 }, ImGuiChildFlags_None, ImGuiWindowFlags_NoSavedSettings);
        {
            /// @separator

            /// @separator
            ImGui::EndChild();
        }
        /// @end Child

        /// @begin Table
        if (ImGui::BeginTable("table3", 3, ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, { 0, 0 }))
        {
            ImGui::TableSetupColumn("left-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableSetupColumn("right-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Button
            ImRad::TableNextColumn(1);
            if (ImGui::Button("OK", { 100, 32 }) ||
                (!ImRad::IsItemDisabled() && ImGui::IsKeyPressed(ImGuiKey_Escape, false)))
            {
                ClosePopup(ImRad::Ok);
            }
            /// @end Button


            /// @separator
            ImGui::EndTable();
        }
        /// @end Table

        /// @separator
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    /// @end TopWindow
}


void AboutDlg::OpenURL()
{
	ShellExec(GITHUB_URL);
}

void AboutDlg::Init()
{
    // TODO: Add your code here
}
