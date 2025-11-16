// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#include "ui_settings_dlg.h"
#include "utils.h"

SettingsDlg settingsDlg;


void SettingsDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImRad::GetUserData().dimBgRatio = 1.f;
    ImGui::OpenPopup(ID);
    Init();
}

void SettingsDlg::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    ImRad::GetUserData().dimBgRatio = 0.f;
}

void SettingsDlg::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    vb1.Reset();
    hb1.Reset();
    hb2.Reset();
    hb3.Reset();
}

void SettingsDlg::Init()
{
    // TODO: Add your code here
}

void SettingsDlg::Draw()
{
    /// @dpi-info 141.767,1.25
    /// @style imrad
    /// @unit px
    /// @begin TopWindow
    ID = ImGui::GetID("###SettingsDlg");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10, 10 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10, 5 });
    ImGui::SetNextWindowSize({ 700, 520 }, ImGuiCond_FirstUseEver); //{ 700, 520 }
    ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Settings###SettingsDlg", &tmpOpen, ImGuiWindowFlags_NoCollapse))
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

        /// @begin Selectable
        vb1.BeginLayout();
        hb1.BeginLayout();
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0.5f });
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImRad::Selectable("Category", false, ImGuiSelectableFlags_NoAutoClosePopups, { hb1.GetSize(), 0 });
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
        vb1.AddSize(0, ImRad::VBox::ItemSize);
        hb1.AddSize(0, ImRad::HBox::Stretch(0.3f));
        /// @end Selectable

        /// @begin Selectable
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImRad::Selectable("Environment Settings", false, ImGuiSelectableFlags_NoAutoClosePopups, { hb1.GetSize(), 0 });
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        hb1.AddSize(1, ImRad::HBox::Stretch(1.0f));
        /// @end Selectable

        // TODO: Add Draw calls of dependent popup windows here

        /// @begin Table
        hb2.BeginLayout();
        ImRad::Spacing(1);
        if (ImGui::BeginTable("table1", 1, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV, { hb2.GetSize(), vb1.GetSize() }))
        {
            ImGui::TableSetupColumn("A", 0, 0);
            ImGui::TableSetupScrollFreeze(0, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Selectable
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0.5f });
            ImRad::Selectable("\xee\x85\xa3  Environment", true, ImGuiSelectableFlags_NoAutoClosePopups, { -1, 30 });
            ImGui::PopStyleVar();
            /// @end Selectable

            /// @separator
            ImGui::EndTable();
        }
        vb1.AddSize(2, ImRad::VBox::Stretch(1.0f));
        hb2.AddSize(0, ImRad::HBox::Stretch(0.3f));
        /// @end Table

        /// @begin Child
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xffd0d0d0);
        ImGui::BeginChild("child2", { hb2.GetSize(), vb1.GetSize() }, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoSavedSettings);
        {
            /// @separator

            /// @begin Text
            ImGui::PushFont(nullptr, ::uiFontSize*1.1f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("UI font");
            ImGui::PopFont();
            /// @end Text

            /// @begin Selectable
            ImRad::Spacing(1);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::AlignTextToFramePadding();
            ImRad::Selectable("Regular:##reg1", false, ImGuiSelectableFlags_NoAutoClosePopups, { 60, 0 });
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
            /// @end Selectable

            /// @begin Combo
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::SetNextItemWidth(200);
            ImRad::Combo("##uiFontName", &uiFontName, fontNames, 0);
            ImGui::PopStyleVar();
            /// @end Combo

            /// @begin Text
            ImGui::SameLine(0, 2 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::TextUnformatted("Size:");
            /// @end Text

            /// @begin Combo
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::SetNextItemWidth(100);
            ImRad::Combo("##uiFontSize", &uiFontSize, fontSizes, 0);
            ImGui::PopStyleVar();
            /// @end Combo

            /// @begin Text
            ImRad::Spacing(5);
            ImGui::PushFont(nullptr, ::uiFontSize*1.1f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Property Grid Fonts");
            ImGui::PopFont();
            /// @end Text

            /// @begin Selectable
            ImRad::Spacing(1);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::AlignTextToFramePadding();
            ImRad::Selectable("Regular:##reg2", false, ImGuiSelectableFlags_NoAutoClosePopups, { 60, 0 });
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
            /// @end Selectable

            /// @begin Combo
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::SetNextItemWidth(200);
            ImRad::Combo("##pgFontName", &pgFontName, fontNames, 0);
            ImGui::PopStyleVar();
            /// @end Combo

            /// @begin Text
            ImGui::SameLine(0, 2 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::TextUnformatted("Size:");
            /// @end Text

            /// @begin Combo
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::SetNextItemWidth(100);
            ImRad::Combo("##pgFontSize", &pgFontSize, fontSizes, 0);
            ImGui::PopStyleVar();
            /// @end Combo

            /// @begin Selectable
            ImRad::Spacing(1);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::AlignTextToFramePadding();
            ImRad::Selectable("Bold:##bold2", false, ImGuiSelectableFlags_NoAutoClosePopups, { 60, 0 });
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
            /// @end Selectable

            /// @begin Combo
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::SetNextItemWidth(200);
            ImRad::Combo("##pgbFontName", &pgbFontName, fontNames, 0);
            ImGui::PopStyleVar();
            /// @end Combo

            /// @begin Text
            ImRad::Spacing(5);
            ImGui::PushFont(nullptr, ::uiFontSize*1.1f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Designer font");
            ImGui::PopFont();
            /// @end Text

            /// @begin Selectable
            ImRad::Spacing(1);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::AlignTextToFramePadding();
            ImRad::Selectable("Regular:##reg3", false, ImGuiSelectableFlags_NoAutoClosePopups, { 60, 0 });
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
            /// @end Selectable

            /// @begin Combo
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::SetNextItemWidth(200);
            ImRad::Combo("##designFontName", &designFontName, fontNames, 0);
            ImGui::PopStyleVar();
            /// @end Combo

            /// @begin Text
            ImGui::SameLine(0, 2 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::TextUnformatted("Size:");
            /// @end Text

            /// @begin Combo
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::SetNextItemWidth(100);
            ImRad::Combo("##designFontSize", &designFontSize, fontSizes, 0);
            ImGui::PopStyleVar();
            /// @end Combo

            /// @separator
            ImGui::EndChild();
        }
        ImGui::PopStyleColor();
        vb1.UpdateSize(0, ImRad::VBox::Stretch(1.0f));
        hb2.AddSize(1, ImRad::HBox::Stretch(1.0f));
        /// @end Child

        /// @begin Spacer
        hb3.BeginLayout();
        ImRad::Spacing(1);
        ImRad::Dummy({ hb3.GetSize(), 0 });
        vb1.AddSize(2, ImRad::VBox::ItemSize);
        hb3.AddSize(0, ImRad::HBox::Stretch(1.0f));
        /// @end Spacer

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("OK", { 100, 30 }))
        {
            ClosePopup(ImRad::Ok);
        }
        vb1.UpdateSize(0, 30);
        hb3.AddSize(1, 100);
        /// @end Button

        /// @separator
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    /// @end TopWindow
}
