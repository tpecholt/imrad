// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#include "ui_table_cols.h"
#include "ui_binding.h"

TableCols tableCols;


void TableCols::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImRad::GetUserData().dimBgRatio = 1.f;
    ImGui::OpenPopup(ID);
    Init();
}

void TableCols::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    ImRad::GetUserData().dimBgRatio = 0.f;
}

void TableCols::Init()
{
    sel = columns.size() ? 0 : -1;
    CheckErrors();
}

void TableCols::Draw()
{
    /// @dpi-info 141.357,1.25
    /// @style imrad
    /// @unit px
    /// @begin TopWindow
    ID = ImGui::GetID("###TableCols");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8, 5 });
    ImGui::SetNextWindowSize({ 550, 480 }, ImGuiCond_FirstUseEver); //{ 550, 480 }
    ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Table Columns###TableCols", &tmpOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavInputs))
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
        bindingDlg.Draw();

        /// @begin Selectable
        vb1.BeginLayout();
        ImRad::Spacing(1);
        ImGui::BeginDisabled(true);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 1.f });
        ImRad::Selectable("Columns:", false, ImGuiSelectableFlags_NoAutoClosePopups, { 0, 0 });
        ImGui::PopStyleVar();
        vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
        ImGui::EndDisabled();
        /// @end Selectable

        /// @begin Splitter
        ImGui::BeginChild("splitter1", { -1, vb1.GetSize() });
        {
            ImGui::PushStyleColor(ImGuiCol_Separator, 0x00000000);
            ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, 0x00000000);
            ImRad::Splitter(true, 8, &sash, 10, 10);
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            /// @separator

            /// @begin Table
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 7, 5 });
            ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xffffffff);
            if (ImGui::BeginTable("table2", 1, ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_ScrollY, { sash, -1 }))
            {
                ImGui::TableSetupColumn("A", 0, 0);
                ImGui::TableSetupScrollFreeze(0, 0);

                for (int i = 0; i < columns.size(); ++i)
                {
                    auto& _item = columns[i];
                    ImGui::PushID(i);
                    ImGui::TableNextRow(0, 0);
                    ImGui::TableSetColumnIndex(0);
                    /// @separator

                    /// @begin Selectable
                    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
                    if (ImRad::Selectable(ImRad::Format("{}", columns[i].label.c_str()).c_str(), i==sel, ImGuiSelectableFlags_NoAutoClosePopups | ImGuiSelectableFlags_SpanAllColumns, { 0, 0 }))
                    {
                        Selectable_Change();
                    }
                    ImGui::PopStyleVar();
                    /// @end Selectable

                    /// @begin Text
                    ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    ImGui::TextUnformatted(ImRad::Format("{}", columns[i].SizingPolicyString()).c_str());
                    ImGui::PopStyleColor();
                    /// @end Text

                    /// @separator
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            /// @end Table

            /// @begin CustomWidget
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            Properties_Draw(ImRad::CustomWidgetArgs("", { -1, -1 }));
            /// @end CustomWidget

            /// @separator
            ImGui::EndChild();
        }
        vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::Stretch(1.0f));
        /// @end Splitter

        /// @begin Button
        if (ImGui::Button("\xef\x83\xbe", { 37, 0 }))
        {
            AddButton_Change();
        }
        vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Add new column");
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(sel<0);
        if (ImGui::Button("\xef\x8b\xad", { 37, 0 }))
        {
            RemoveButton_Change();
        }
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Remove column");
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 2 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(sel<=0);
        if (ImGui::ArrowButton("##button3", ImGuiDir_Up))
        {
            UpButton_Change();
        }
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Move column up");
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(sel+1==columns.size());
        if (ImGui::ArrowButton("##button4", ImGuiDir_Down))
        {
            DownButton_Change();
        }
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Move column down");
        /// @end Button

        /// @begin Selectable
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushStyleColor(ImGuiCol_Text, 0xff0000ff);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 1.f, 0 });
        ImRad::Selectable(ImRad::Format("{}##error", error).c_str(), false, ImGuiSelectableFlags_NoAutoClosePopups | ImGuiSelectableFlags_Disabled, { 0, 0 });
        ImGui::PopStyleVar();
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        ImGui::PopStyleColor();
        /// @end Selectable

        /// @begin Spacer
        hb4.BeginLayout();
        ImRad::Spacing(2);
        ImRad::Dummy({ hb4.GetSize(), 0 });
        vb1.AddSize(3 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
        hb4.AddSize(0 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::Stretch(1.0f));
        /// @end Spacer

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("OK", { 100, 30 }))
        {
            ClosePopup(ImRad::Ok);
        }
        vb1.UpdateSize(0, 30);
        hb4.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 100);
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Cancel", { 100, 30 }) ||
            ImGui::Shortcut(ImGuiKey_Escape))
        {
            ClosePopup(ImRad::Cancel);
        }
        vb1.UpdateSize(0, 30);
        hb4.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 100);
        /// @end Button

        /// @separator
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    /// @end TopWindow
}

void TableCols::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    vb1.Reset();
    hb4.Reset();
}

void TableCols::Selectable_Change()
{
    sel = ImGui::TableGetRowIndex();
}

void TableCols::AddButton_Change()
{
    columns.push_back(Table::ColumnData("New Column", ImGuiTableColumnFlags_None));
    sel = (int)columns.size() - 1;
}

void TableCols::RemoveButton_Change()
{
    if (sel >= 0)
        columns.erase(columns.begin() + sel);
    if (sel >= columns.size())
        --sel;
}

void TableCols::UpButton_Change()
{
    if (sel) {
        std::swap(columns[sel], columns[sel - 1]);
        --sel;
    }
}

void TableCols::DownButton_Change()
{
    if (sel + 1 < columns.size()) {
        std::swap(columns[sel], columns[sel + 1]);
        ++sel;
    }
}

void TableCols::Properties_Draw(const ImRad::CustomWidgetArgs& args)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xfffafafa);
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, 0xffe5e5e5);
    ImVec4 clr = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    clr.w *= 0.5f;
    ImGui::PushStyleColor(ImGuiCol_Button, clr);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImVec2 framePad = ImGui::GetStyle().FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { framePad.x * 0.5f, framePad.y * 0.5f });

    ImVec2 pgMin = ImGui::GetCursorScreenPos();
    static float pgHeight = 0;
    uint32_t borderClr = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TableBorderLight));
    ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("pg", 2, flags, args.size))
    {
        ImGui::GetWindowDrawList()->AddRectFilled(
            pgMin,
            { pgMin.x + ImGui::GetStyle().IndentSpacing + 1, pgMin.y + pgHeight },
            borderClr
        );
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, ImGui::GetStyle().FramePadding.y });
        ImGui::PushFont(ctx->pgbFont);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
            ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TableBorderLight)));
        ImGui::PushStyleColor(ImGuiCol_NavCursor, 0x0);
        ImGui::SetNextItemOpen(true);
        bool open = ImGui::TreeNodeEx("Behavior", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::PopStyleVar();
        //ImGui::Indent();

        if (sel >= 0 && open)
        {
            int n = (int)columns[sel].Properties().size();
            for (int i = 0; i < n; ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                if (columns[sel].PropertyUI(i, *ctx))
                    CheckErrors();
            }
        }
        if (open)
            ImGui::TreePop();

        ImGui::EndTable();
        pgHeight = ImGui::GetItemRectSize().y;
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

void TableCols::CheckErrors()
{
    error = "";
    int noPolicy = 0;
    for (const auto& cd : columns)
        if (!(cd.sizingPolicy & ImGuiTableColumnFlags_WidthMask_))
            ++noPolicy;
    if (noPolicy && noPolicy != columns.size())
        error = "specify sizingPolicy either for all columns or none";
}
