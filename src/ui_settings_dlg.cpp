// Generated with ImRAD 0.8
// visit https://github.com/tpecholt/imrad

#include "ui_settings_dlg.h"
#include "utils.h"

SettingsDlg settingsDlg;


void SettingsDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 1.f;
    ImGui::OpenPopup(ID);
    Init();
}

void SettingsDlg::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 0.f;
}

void SettingsDlg::ResetLayout()
{
    vb1.Reset();
    hb3.Reset();
}

void SettingsDlg::Init()
{
    // TODO: Add your code here
}

void SettingsDlg::Draw()
{
    /// @style imrad
    /// @unit px
    /// @begin TopWindow
    auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;
    ID = ImGui::GetID("###SettingsDlg");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10, 10 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10, 5 });
    ImGui::SetNextWindowSize({ 640, 480 }, ImGuiCond_FirstUseEver); //{ 640, 480 }
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Settings###SettingsDlg", &tmpOpen, ImGuiWindowFlags_NoCollapse))
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

        /// @begin Selectable
        vb1.BeginLayout();
        ImGui::SetCursorPosY(vb1);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0.5f });
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImRad::Selectable("Category", false, ImGuiSelectableFlags_DontClosePopups, { 140, 24 });
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
        vb1.AddSize(0, 24);
        ImGui::PopStyleColor();
        /// @end Selectable

        /// @begin Text
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushFont(ImRad::GetFontByName("imrad.H3"));
        ImGui::TextUnformatted("Environment Settings");
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        ImGui::PopFont();
        /// @end Text

        // TODO: Add Draw calls of dependent popup windows here

        /// @begin Table
        ImGui::SetCursorPosY(vb1);
        if (ImGui::BeginTable("table1", 1, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV, { 140, vb1.GetSize(false) }))
        {
            ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_None, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Selectable
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0.5f });
            ImRad::Selectable("\xee\x85\xa3  Environment", true, ImGuiSelectableFlags_DontClosePopups, { 0, 30 });
            ImGui::PopStyleVar();
            /// @end Selectable


            /// @separator
            ImGui::EndTable();
        }
        vb1.AddSize(1, ImRad::VBox::Stretch);
        /// @end Table

        /// @begin Child
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xffd0d0d0);
        ImGui::BeginChild("child2", { -1, vb1.GetSize(true) }, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoSavedSettings);
        {
            /// @separator

            /// @begin Text
            ImRad::Spacing(1);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Font:");
            /// @end Text

            /// @begin Combo
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::SetNextItemWidth(200);
            ImRad::Combo("##fontName", &fontName, fontNames, ImGuiComboFlags_None);
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
            ImRad::Combo("##fontSize", &fontSize, "12\00014\00016\00018\00019\00020\00021\00022\00024\00026\000", ImGuiComboFlags_None);
            ImGui::PopStyleVar();
            /// @end Combo

            /// @separator
            ImGui::EndChild();
        }
        ImGui::PopStyleColor();
        vb1.UpdateSize(0, ImRad::VBox::Stretch);
        /// @end Child

        /// @begin Spacer
        ImGui::SetCursorPosY(vb1);
        hb3.BeginLayout();
        ImGui::SetCursorPosX(hb3);
        ImRad::Dummy({ hb3.GetSize(), 0 });
        vb1.AddSize(2, ImRad::VBox::ItemSize);
        hb3.AddSize(0, ImRad::HBox::Stretch);
        /// @end Spacer

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::SetCursorPosX(hb3);
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
