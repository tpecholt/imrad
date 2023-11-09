// Generated with ImRAD 0.7
// visit https://github.com/tpecholt/imrad

#include "test.h"

Test test;


void Test::Draw()
{
    /// @style Dark
    /// @unit dp
    /// @begin TopWindow
    const float dp = ((ImRad::IOUserData*)ImGui::GetIO().UserData)->dpiScale;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 7*dp, 10*dp });
    auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::SetNextWindowPos(ioUserData->displayRectMinOffset);
    ImGui::SetNextWindowSize({ ImGui::GetIO().DisplaySize.x - ioUserData->displayRectMinOffset.x - ioUserData->displayRectMaxOffset.x, 
                               ImGui::GetIO().DisplaySize.y - ioUserData->displayRectMinOffset.y - ioUserData->displayRectMaxOffset.y }); //{ 420*dp, 640*dp }
    bool tmpOpen;
    if (ImGui::Begin("###Test", &tmpOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        /// @separator

        // TODO: Add Draw calls of dependent popup windows here

        /// @begin Text
        ImGui::TextUnformatted("Employee to fire");
        /// @end Text

        /// @begin Input
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##value1", &value1, ImGuiInputTextFlags_None);
        /// @end Input

        /// @begin Text
        ImRad::Spacing(1);
        ImGui::TextUnformatted("Reason");
        /// @end Text

        /// @begin Combo
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##value3", &value3, "Breaks company equipment\0Late at work\0Lazy\0Disobedient\0");
        /// @end Combo

        /// @begin Text
        ImRad::Spacing(1);
        ImGui::TextUnformatted("Notice to be sent by email");
        /// @end Text

        /// @begin Input
        ImGui::InputTextMultiline("##value5", &value5, { -1, -110*dp }, ImGuiInputTextFlags_Multiline);
        /// @end Input

        /// @begin CheckBox
        ImRad::Spacing(1);
        ImGui::Checkbox("No severance pay & press charges", &value7);
        /// @end CheckBox

        /// @begin Button
        ImRad::Spacing(1);
        ImGui::PushStyleColor(ImGuiCol_Button, 0xff000080);
        ImGui::Button("Fire Employee", { 130*dp, 35*dp });
        ImGui::PopStyleColor();
        /// @end Button

        /// @separator
        ImGui::End();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    /// @end TopWindow
}

