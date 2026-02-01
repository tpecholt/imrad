// Generated with ImRAD 0.9.1
// visit https://github.com/tpecholt/imrad

#include "ui_configuration_dlg.h"
#include "ui_message_box.h"
#include "ui_new_style.h"
#include "utils.h"
#include "stx.h"
#include "cpp_parser.h"

ConfigurationDlg configurationDlg;

const std::string DEFAULT_CFG_NAME = "(Default)";

void ConfigurationDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImRad::GetUserData().dimBgRatio = 1.f;
    IM_ASSERT(ID && "Call Draw at least once to get ID assigned");
    ImGui::OpenPopup(ID);
    Init();
}

void ConfigurationDlg::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    ImRad::GetUserData().dimBgRatio = 0.f;
}

void ConfigurationDlg::Init()
{
    // TODO: Add your code here
    Check();
}

void ConfigurationDlg::Draw()
{
    /// @dpi-info 141.357,1.25
    /// @style imrad
    /// @unit dp
    /// @begin TopWindow
    const float dp = ImRad::GetUserData().dpiScale;
    ID = ImGui::GetID("###ConfigurationDlg");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10*dp, 10*dp });
    ImGui::SetNextWindowSize({ 680*dp, 480*dp }, ImGuiCond_FirstUseEver); //{ 680*dp, 480*dp }
    ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Configuration Manager###ConfigurationDlg", &tmpOpen, 0))
    {
        if (ImRad::GetUserData().activeActivity != "")
            ImRad::RenderDimmedBackground(ImRad::GetUserData().WorkRect(), ImRad::GetUserData().dimBgRatio);
        if (modalResult != ImRad::None)
        {
            ImGui::CloseCurrentPopup();
            if (modalResult != ImRad::Cancel)
                callback(modalResult);
        }
        /// @separator

        // TODO: Add Draw calls of dependent popup windows here
        messageBox.Draw();
        newStyleDlg.Draw();

        /// @begin Text
        vb1.BeginLayout();
        ImRad::Spacing(1);
        ImGui::TextUnformatted("Existing configurations:");
        vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
        /// @end Text

        /// @begin Table
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 0, 1*dp });
        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV, { -1, vb1.GetSize() }))
        {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 25*dp);
            ImGui::TableSetupColumn(" Name", 0, 0);
            ImGui::TableSetupColumn(" Style", 0, 0);
            ImGui::TableSetupColumn(" Units", 0, 0);
            ImGui::TableSetupScrollFreeze(0, 0);
            ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
            ImGui::TableHeadersRow();
            ImGui::PopItemFlag();

            for (curRow = 0; curRow < configs.size(); ++curRow)
            {
                auto& _item = configs[curRow];
                ImGui::PushID(curRow);
                ImGui::TableNextRow(0, 0);
                ImGui::TableSetColumnIndex(0);
                /// @separator

                /// @begin Button
                if (curRow==selRow)
                {
                    //visible
                    ImGui::BeginDisabled(true);
                    ImGui::PushStyleColor(ImGuiCol_Button, 0x00ffffff);
                    ImGui::ArrowButton("##button1", ImGuiDir_Right);
                    ImGui::PopStyleColor();
                    ImGui::EndDisabled();
                }
                /// @end Button

                /// @begin Input
                ImRad::TableNextColumn(1);
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputTextWithHint("##_item.name", DEFAULT_CFG_NAME.c_str(), &_item.name, 0))
                    Name_Change();
                if (ImGui::IsItemActive())
                    ImRad::GetUserData().imeType = ImRad::ImeText;
                if (ImGui::IsItemActivated())
                    Input_Activated();
                /// @end Input

                /// @begin Combo
                ImRad::TableNextColumn(1);
                ImGui::SetNextItemWidth(-1);
                if (ImGui::BeginCombo("##_item.style", _item.style.c_str(), 0))
                {
                    StyleCombo_DrawItems();
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemActivated())
                    Input_Activated();
                /// @end Combo

                /// @begin Combo
                ImRad::TableNextColumn(1);
                ImGui::SetNextItemWidth(-1);
                ImRad::Combo("##_item.unit", &_item.unit, "px\000dp\000", 0);
                if (ImGui::IsItemActivated())
                    Input_Activated();
                /// @end Combo

                /// @separator
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::Stretch(1.0f));
        ImGui::PopItemFlag();
        /// @end Table

        /// @begin Button
        hb3.BeginLayout();
        if (ImGui::Button("\xef\x80\x93+", { 37*dp, 0 }))
        {
            AddButton_Change();
        }
        vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
        hb3.AddSize(0 * ImGui::GetStyle().ItemSpacing.x, 37*dp);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Add new configuration");
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("\xef\x94\xbf+", { 37*dp, 0 }))
        {
            AddStyleButton_Change();
        }
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        hb3.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 37*dp);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Create new style");
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 2 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(configs.size()<=1);
        if (ImGui::Button("\xef\x8b\xad", { 37*dp, 0 }))
        {
            RemoveButton_Change();
        }
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        hb3.AddSize(2 * ImGui::GetStyle().ItemSpacing.x, 37*dp);
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Remove configuration");
        /// @end Button

        /// @begin Spacer
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImRad::Dummy({ hb3.GetSize(), 20*dp });
        vb1.UpdateSize(0, 20*dp);
        hb3.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::Stretch(1.0f));
        /// @end Spacer

        /// @begin Text
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushStyleColor(ImGuiCol_Text, 0xff0000ff);
        ImGui::TextUnformatted(ImRad::Format("{}", error).c_str());
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        hb3.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::ItemSize);
        ImGui::PopStyleColor();
        /// @end Text

        /// @begin Spacer
        hb4.BeginLayout();
        ImRad::Dummy({ hb4.GetSize(), 20*dp });
        vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, 20*dp);
        hb4.AddSize(0 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::Stretch(1.0f));
        /// @end Spacer

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(error!="");
        if (ImGui::Button("OK", { 100*dp, 30*dp }))
        {
            ClosePopup(ImRad::Ok);
        }
        vb1.UpdateSize(0, 30*dp);
        hb4.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 100*dp);
        ImGui::EndDisabled();
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere(-1);
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Cancel", { 100*dp, 30*dp }) ||
            ImGui::Shortcut(ImGuiKey_Escape))
        {
            ClosePopup(ImRad::Cancel);
        }
        vb1.UpdateSize(0, 30*dp);
        hb4.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 100*dp);
        /// @end Button

        /// @separator
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    /// @end TopWindow
}

void ConfigurationDlg::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    vb1.Reset();
    hb3.Reset();
    hb4.Reset();
}

void ConfigurationDlg::AddButton_Change()
{
    Config& cfg = configs.emplace_back();
    cfg.name = "New";
    cfg.style = defaultStyle;
    cfg.unit = defaultUnit;
    selRow = (int)configs.size() - 1;
    Check();

    /*newStyleDlg.title = "New Configuration";
    newStyleDlg.name = "";
    newStyleDlg.source = "";
    newStyleDlg.sources.clear();
    for (const auto& cfg : configs) {
        newStyleDlg.sources.push_back(cfg.name=="" ? DEFAULT_CFG_NAME : cfg.name);
    }
    newStyleDlg.OpenPopup([this, row = curRow](ImRad::ModalResult) {
        Config cfg;
        cfg.name = newStyleDlg.name;
        cfg.style = defaultStyle;
        cfg.unit = defaultUnit;
        cfg.id =
        configs.push_back(cfg);
        selRow = (int)configs.size() - 1;
        Check();
    });*/
}

void ConfigurationDlg::RemoveButton_Change()
{
    if (selRow < 0 || selRow >= configs.size())
        return;
    messageBox.title = "Remove";
    messageBox.icon = MessageBox::Warning;
    messageBox.message = "Remove configuration \"" + configs[selRow].name + "\" ?";
    messageBox.buttons = ImRad::Yes | ImRad::Cancel;
    messageBox.OpenPopup([&](ImRad::ModalResult mr) {
        if (mr != ImRad::Yes)
            return;
        configs.erase(configs.begin() + selRow);
        if (selRow == configs.size())
            --selRow;
        Check();
        });
}

void ConfigurationDlg::Name_Change()
{
    Check();
}

void ConfigurationDlg::Check()
{
    error = "";
    std::vector<std::string> names;
    for (const auto& cfg : configs)
    {
        bool invalid = stx::count_if(cfg.name, [](char c) {
            return !std::isalnum(c) && c != '_';
            });
        if (invalid)
            error = "\"" + cfg.name + "\" is not a valid id";

        size_t i = &cfg - configs.data();
        if (std::count_if(configs.begin() + i + 1, configs.end(),
            [&](const auto& c) { return c.name == cfg.name; }))
        {
            error = "Duplicated name '" + cfg.name + "' used";
        }
    }
}

void ConfigurationDlg::Input_Activated()
{
    selRow = curRow;
}

void ConfigurationDlg::StyleCombo_DrawItems()
{
    if (curRow < 0 || curRow >= configs.size())
        return;
    auto& cfg = configs[curRow];
    for (const std::string& st : styles)
    {
        bool stock = st == "Classic" || st == "Light" || st == "Dark";
        ImGui::PushStyleColor(ImGuiCol_Text, stock ? 0xff000000 : 0xff000080);
        if (ImGui::Selectable(st.c_str(), st == cfg.style))
            cfg.style = st;
        ImGui::PopStyleColor();
    }
}

void ConfigurationDlg::AddStyleButton_Change()
{
    newStyleDlg.title = "New Style";
    newStyleDlg.name = "";
    newStyleDlg.source = defaultStyle;
    newStyleDlg.sources = styles;
    newStyleDlg.OpenPopup([this, row = selRow](ImRad::ModalResult) {
        std::string error;
        if (!copyStyleFun(newStyleDlg.source, newStyleDlg.name, error)) {
            messageBox.title = "error";
            messageBox.icon = MessageBox::Warning;
            messageBox.message = error;
            messageBox.buttons = ImRad::Ok;
            messageBox.OpenPopup();
            return;
        }
        styles.push_back(newStyleDlg.name);
        if (row >= 0 && row < configs.size())
            configs[row].style = newStyleDlg.name;
        });
}
