// Generated with ImRAD 0.1
// github.com/xyz

#include "ui_about_dlg.h"
#include "utils.h"
#include <misc/cpp/imgui_stdlib.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#endif

AboutDlg aboutDlg;

void AboutDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    requestClose = false;
    ImGui::OpenPopup(ID); // must be called here from current Begin-stack

    // Add your init code here
}

void AboutDlg::ClosePopup()
{
    requestClose = true; // CloseCurrentPopup must be called later from our Begin-stack
}

void AboutDlg::Draw()
{
    /// @begin TopWindow
    ImGui::SetNextWindowSize({ 300, 140 }, ImGuiCond_Appearing);
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ID = ImGui::GetID("About");
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("About", &tmpOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
        if (requestClose) ImGui::CloseCurrentPopup();
        /// @separator

        /// @begin Text
        ImGui::TextUnformatted(VER_STR.c_str());
        /// @end Text

        /// @begin Text
		ImGui::Spacing();
		ImGui::TextUnformatted("visit");
        /// @end Text

        /// @begin Text
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, 0xffff9018);
		ImGui::TextUnformatted(GITHUB_STR.c_str());
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(7);
        if (ImGui::IsItemClicked())
            OpenURL();
        /// @end Text

        /// @begin Child
        ImGui::BeginChild("child1725042201600", { -100, 10 }, false);
        {
            /// @separator

            /// @separator
            ImGui::EndChild();
        }
        /// @end Child

        /// @begin Button
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - 100) / 2);
        if (ImGui::Button("OK", { 100, 0 }))
        {
            ClosePopup();
            callback(ImRad::Ok);
        }
        /// @end Button

        /// @separator
        ImGui::EndPopup();
    }
    /// @end TopWindow
}


void AboutDlg::OpenURL()
{
#ifdef WIN32
	ShellExecuteA(nullptr, "open", ("http://" + GITHUB_STR).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	system(("xdg-open http://" + GITHUB_STR).c_str());
#endif
}
