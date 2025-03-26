// Generated with ImRAD 0.8
// visit https://github.com/tpecholt/imrad

#include "ui_table_cols.h"
#include "ui_binding.h"

TableCols tableCols;


void TableCols::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 1.f;
    ImGui::OpenPopup(ID);
    Init();
}

void TableCols::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 0.f;
}

void TableCols::Init()
{
    sel = columns.size() ? 0 : -1;
}

void TableCols::Draw()
{
    /// @style imrad
    /// @unit px
    /// @begin TopWindow
    auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;
    ID = ImGui::GetID("###TableCols");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8, 5 });
    ImGui::SetNextWindowSize({ 550, 480 }, ImGuiCond_FirstUseEver); //{ 550, 480 }
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Table Columns###TableCols", &tmpOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavInputs))
    {
        if (ioUserData->activeActivity != "")
            ImRad::RenderDimmedBackground(ioUserData->WorkRect(), ioUserData->dimBgRatio);
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
        ImRad::Selectable("Columns:", false, ImGuiSelectableFlags_DontClosePopups, { 0, 0 });
        ImGui::PopStyleVar();
        vb1.AddSize(1, ImRad::VBox::ItemSize);
        ImGui::EndDisabled();
        /// @end Selectable

        // TODO: Add Draw calls of dependent popup windows here

        /// @begin Table
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 7, 5 });
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xffffffff);
        if (ImGui::BeginTable("table1", 1, ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_ScrollY, { 175, vb1.GetSize() }))
        {
            ImGui::TableSetupColumn("A", 0, 0);
            ImGui::TableSetupScrollFreeze(0, 0);

            for (int i = 0; i < columns.size(); ++i)
            {
                ImGui::PushID(i);
                ImGui::TableNextRow(0, 0);
                ImGui::TableSetColumnIndex(0);
                /// @separator

                /// @begin Selectable
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
                if (ImRad::Selectable(ImRad::Format("{}", columns[i].label.c_str()).c_str(), i==sel, ImGuiSelectableFlags_DontClosePopups, { 0, 0 }))
                    Selectable_Change();
                ImGui::PopStyleVar();
                /// @end Selectable


                /// @separator
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        vb1.AddSize(1, ImRad::VBox::Stretch(1));
        /// @end Table

        /// @begin CustomWidget
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        Properties_Draw({ -1, vb1.GetSize() });
        vb1.UpdateSize(0, ImRad::VBox::Stretch(1));
        /// @end CustomWidget

        /// @begin Table
        if (ImGui::BeginTable("table2", 3, ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, { 175, 0 }))
        {
            ImGui::TableSetupColumn("left-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableSetupColumn("right-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupScrollFreeze(0, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(0);
            /// @separator

        // TODO: Add Draw calls of dependent popup windows here

            /// @begin Button
            ImRad::TableNextColumn(1);
            if (ImGui::Button("\xef\x83\xbe", { 37, 0 }))
            {
                AddButton_Change();
            }
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
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Remove column");
            /// @end Button

            /// @begin Button
            ImGui::SameLine(0, 2 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::BeginDisabled(sel<=0);
            if (ImGui::ArrowButton("##1742563997520", ImGuiDir_Up))
            {
                UpButton_Change();
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Move column up");
            /// @end Button

            /// @begin Button
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::BeginDisabled(sel+1==columns.size());
            if (ImGui::ArrowButton("##1742564030000", ImGuiDir_Down))
            {
                DownButton_Change();
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Move column down");
            /// @end Button


            /// @separator
            ImGui::EndTable();
        }
        vb1.AddSize(1, ImRad::VBox::ItemSize);
        /// @end Table

        /// @begin Spacer
        hb4.BeginLayout();
        ImRad::Dummy({ hb4.GetSize(), 0 });
        vb1.AddSize(1, ImRad::VBox::ItemSize);
        hb4.AddSize(0, ImRad::HBox::Stretch(1));
        /// @end Spacer

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("OK", { 100, 30 }))
        {
            ClosePopup(ImRad::Ok);
        }
        vb1.UpdateSize(0, 30);
        hb4.AddSize(1, 100);
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Cancel", { 100, 30 }) ||
            ImGui::Shortcut(ImGuiKey_Escape))
        {
            ClosePopup(ImRad::Cancel);
        }
        vb1.UpdateSize(0, 30);
        hb4.AddSize(1, 100);
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
        
        /*ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, ImGui::GetStyle().FramePadding.y });
        ImGui::PushFont(ctx->pgbFont);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
            ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TableBorderLight)));
        ImGui::PushStyleColor(ImGuiCol_NavCursor, 0x0);
        ImGui::SetNextItemAllowOverlap();
        bool open = ImGui::TreeNodeEx("Behavior", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::PopStyleVar();*/
        ImGui::Indent();
        
        if (sel >= 0) // && open)
        {
            int n = (int)columns[sel].Properties().size();
            for (int i = 0; i < n; ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                columns[sel].PropertyUI(i, *ctx);
            }
        }
        /*if (open)
            ImGui::TreePop();*/

        ImGui::EndTable();
        pgHeight = ImGui::GetItemRectSize().y;
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

void TableCols::Table_SortSpecs(ImGuiTableSortSpecs& args)
{
}
