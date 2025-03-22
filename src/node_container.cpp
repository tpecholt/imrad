#include "node_container.h"
#include "node_window.h"
#include "stx.h"
#include "imrad.h"
#include "cppgen.h"
#include "binding_input.h"
#include "binding_field.h"
#include "ui_table_columns.h"
#include "ui_message_box.h"
#include <algorithm>
#include <array>

Table::ColumnData::ColumnData()
{
    flags.add$(ImGuiTableColumnFlags_AngledHeader);
    flags.add$(ImGuiTableColumnFlags_Disabled);
    flags.add$(ImGuiTableColumnFlags_NoHeaderWidth);
    flags.add$(ImGuiTableColumnFlags_NoHide);
    flags.add$(ImGuiTableColumnFlags_NoResize);
    flags.add$(ImGuiTableColumnFlags_NoSort);
    //flags.add$(ImGuiTableColumnFlags_WidthStretch);
    //flags.add$(ImGuiTableColumnFlags_WidthFixed);
}

Table::Table(UIContext& ctx)
{
    size_x = -1; //here 0 has the same effect as -1 but -1 works with our sizing visualization
    size_y = 0;

    flags.add$(ImGuiTableFlags_Resizable);
    flags.add$(ImGuiTableFlags_Reorderable);
    flags.add$(ImGuiTableFlags_Hideable);
    flags.add$(ImGuiTableFlags_Sortable);
    flags.add$(ImGuiTableFlags_ContextMenuInBody);
    flags.separator();
    flags.add$(ImGuiTableFlags_RowBg);
    flags.add$(ImGuiTableFlags_BordersInnerH);
    flags.add$(ImGuiTableFlags_BordersInnerV);
    flags.add$(ImGuiTableFlags_BordersOuterH);
    flags.add$(ImGuiTableFlags_BordersOuterV);
    //flags.add$(ImGuiTableFlags_NoBordersInBody);
    //flags.add$(ImGuiTableFlags_NoBordersInBodyUntilResize);
    flags.separator();
    flags.add$(ImGuiTableFlags_SizingFixedFit);
    flags.add$(ImGuiTableFlags_SizingFixedSame);
    flags.add$(ImGuiTableFlags_SizingStretchProp);
    flags.add$(ImGuiTableFlags_SizingStretchSame);
    flags.separator();
    flags.add$(ImGuiTableFlags_PadOuterX);
    flags.add$(ImGuiTableFlags_NoPadOuterX);
    flags.add$(ImGuiTableFlags_NoPadInnerX);
    flags.add$(ImGuiTableFlags_ScrollX);
    flags.add$(ImGuiTableFlags_ScrollY);
    flags.add$(ImGuiTableFlags_HighlightHoveredColumn);

    columnData.resize(3);
    for (size_t i = 0; i < columnData.size(); ++i)
        columnData[i].label = char('A' + i);
}

std::unique_ptr<Widget> Table::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Table>(*this);
    //rowCount can be shared
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* Table::DoDraw(UIContext& ctx)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (style_cellPadding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, style_cellPadding.eval_px(ctx));
    if (!style_headerBg.empty())
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, style_headerBg.eval(ImGuiCol_TableHeaderBg, ctx));
    if (!style_rowBg.empty())
        ImGui::PushStyleColor(ImGuiCol_TableRowBg, style_rowBg.eval(ImGuiCol_TableRowBg, ctx));
    if (!style_rowBgAlt.empty())
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, style_rowBgAlt.eval(ImGuiCol_TableRowBgAlt, ctx));
    if (!style_childBg.empty())
        ImGui::PushStyleColor(ImGuiCol_ChildBg, style_childBg.eval(ImGuiCol_ChildBg, ctx));

    int n = std::max(1, (int)columnData.size());
    ImVec2 size{ size_x.eval_px(ImGuiAxis_X, ctx), size_y.eval_px(ImGuiAxis_Y, ctx) };
    float rh = rowHeight.eval_px(ImGuiAxis_Y, ctx);
    int fl = flags;
    if (stx::count(ctx.selected, this)) //force columns at design time
        fl |= ImGuiTableFlags_BordersInner;
    std::string name = "table" + std::to_string((uint64_t)this);
    if (ImGui::BeginTable(name.c_str(), n, fl, size))
    {
        //need to override drawList because when table is in a Child mode its drawList will be drawn on top
        drawList = ImGui::GetWindowDrawList();

        for (size_t i = 0; i < (int)columnData.size(); ++i)
            ImGui::TableSetupColumn(columnData[i].label.c_str(), columnData[i].flags, columnData[i].width);
        if (header)
            ImGui::TableHeadersRow();

        ImGui::TableNextRow(0, rh);
        ImGui::TableSetColumnIndex(0);
        
        for (const auto& child : child_iterator(children, false))
            child->Draw(ctx);
        
        int n = itemCount.limit.value();
        for (int r = ImGui::TableGetRowIndex() + 1; r < header + n; ++r)
            ImGui::TableNextRow(0, rh);

        if (child_iterator(children, true))
        {
            ImGuiWindow* win = ImGui::GetCurrentWindow();
            win->SkipItems = false;
            //auto mpos = win->DC.CursorMaxPos;
            ImGui::PushClipRect(win->InnerRect.Min, win->InnerClipRect.Max, false);
            for (const auto& child : child_iterator(children, true))
                child->Draw(ctx);
            ImGui::PopClipRect();
            //win->DC.CursorMaxPos = mpos;
        }

        ImGui::EndTable();
    }

    if (!style_childBg.empty())
        ImGui::PopStyleColor();
    if (!style_headerBg.empty())
        ImGui::PopStyleColor();
    if (!style_rowBg.empty())
        ImGui::PopStyleColor();
    if (!style_rowBgAlt.empty())
        ImGui::PopStyleColor();
    if (style_cellPadding.has_value())
        ImGui::PopStyleVar();

    return drawList;
}

std::vector<UINode::Prop>
Table::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.headerBg", &style_headerBg },
        { "appearance.rowBg", &style_rowBg },
        { "appearance.rowBgAlt", &style_rowBgAlt },
        { "appearance.childBg", &style_childBg },
        { "appearance.cellPadding", &style_cellPadding },
        { "appearance.header##table", &header },
        { "appearance.rowHeight##table", &rowHeight },
        { "behavior.flags##table", &flags },
        { "behavior.columns##table", nullptr },
        { "behavior.rowCount##table", &itemCount.limit },
        { "behavior.rowFilter##table", &rowFilter },
        { "behavior.scrollWhenDragging", &scrollWhenDragging },
        { "bindings.rowIndex##1", &itemCount.index }
        });
    return props;
}

bool Table::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("headerBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_headerBg, ImGuiCol_TableHeaderBg, ctx);
        break;
    case 1:
        ImGui::Text("rowBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_rowBg, ImGuiCol_TableRowBg, ctx);
        break;
    case 2:
        ImGui::Text("rowBgAlt");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_rowBgAlt, ImGuiCol_TableRowBgAlt, ctx);
        break;
    case 3:
        ImGui::Text("childBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_childBg, ImGuiCol_ChildBg, ctx);
        break;
    case 4:
        ImGui::Text("cellPadding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_cellPadding, ctx);
        break;
    case 5:
        ImGui::Text("header");
        ImGui::TableNextColumn();
        fl = header != Defaults().header ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&header, fl, ctx);
        break;
    case 6:
        ImGui::Text("rowHeight");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = rowHeight != Defaults().rowHeight ? InputBindable_Modified : 0;
        changed = InputBindable(&rowHeight, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("rowHeight", &rowHeight, ctx);
        break;
    case 7:
        TreeNodeProp("flags", ctx.pgbFont, "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            changed = CheckBoxFlags(&flags, Defaults().flags);
            });
        break;
    case 8:
    {
        ImGui::Text("columns");
        ImGui::TableNextColumn();
        ImGui::PushFont(ctx.pgbFont);
        std::string label = "[" + std::to_string(columnData.size()) + "]";
        if (ImRad::Selectable(label.c_str(), false, 0, { -ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
        {
            changed = true;
            tableColumns.columnData = columnData;
            tableColumns.target = &columnData;
            tableColumns.font = ctx.defaultStyleFont;
            tableColumns.OpenPopup();
        }
        ImGui::PopFont();
        break;
    }
    case 9:
        ImGui::Text("rowCount");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDataSize(&itemCount.limit, true, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("rowCount", &itemCount.limit, ctx);
        break;
    case 10:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("rowFilter");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = rowFilter != Defaults().rowFilter ? InputBindable_Modified : 0;
        changed = InputBindable(&rowFilter, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("rowFilter", &rowFilter, ctx);
        ImGui::EndDisabled();
        break;
    case 11:
        ImGui::Text("scrollWhenDragging");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&scrollWhenDragging, 0, ctx);
        break;
    case 12:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("rowIndex");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&itemCount.index, true, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::PropertyUI(i - 13, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
Table::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "table.beginRow", &onBeginRow },
        { "table.endRow", &onEndRow }, 
        });
    return props;
}

bool Table::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("BeginRow");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_BeginRow", &onBeginRow, 0, ctx);
        break;
    case 1:
        ImGui::Text("EndRow");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_EndRow", &onEndRow, 0, ctx);
        break;
    default:
        return Widget::EventUI(i - 2, ctx);
    }
    return changed;
}

void Table::DoExport(std::ostream& os, UIContext& ctx)
{
    if (style_cellPadding.has_value())
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, "
            << style_cellPadding.to_arg(ctx.unit) << ");\n";
    }
    if (!style_headerBg.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, "
            << style_headerBg.to_arg() << ");\n";
    }
    if (!style_rowBg.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TableRowBg, "
            << style_rowBg.to_arg() << ");\n";
    }
    if (!style_rowBgAlt.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, "
            << style_rowBgAlt.to_arg() << ");\n";
    }
    if (!style_childBg.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, "
            << style_childBg.to_arg() << ");\n";
    }
    if (scrollWhenDragging)
    {
        os << ctx.ind << "ImRad::PushInvisibleScrollbar();\n";
    }

    os << ctx.ind << "if (ImGui::BeginTable("
        << "\"table" << ctx.varCounter << "\", " 
        << columnData.size() << ", " 
        << flags.to_arg() << ", "
        << "{ " << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", " << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }"
        << "))\n"
        << ctx.ind << "{\n";
    ctx.ind_up();

    if (scrollWhenDragging)
    {
        os << ctx.ind << "ImRad::ScrollWhenDragging(true);\n";
    }

    for (const auto& cd : columnData)
    {
        os << ctx.ind << "ImGui::TableSetupColumn(\"" << cd.label << "\", "
            << cd.flags.to_arg() << ", ";
        if (cd.flags & ImGuiTableColumnFlags_WidthFixed) {
            direct_val<dimension> dim(cd.width);
            os << dim.to_arg(ctx.unit);
        }
        else
            os << cd.width;
        os << ");\n";
    }
    if (header)
        os << ctx.ind << "ImGui::TableHeadersRow();\n";

    if (!itemCount.empty())
    {
        os << "\n" << ctx.ind << itemCount.to_arg(ctx.codeGen->FOR_VAR_NAME) << "\n" << ctx.ind << "{\n";
        ctx.ind_up();

        if (!rowFilter.empty())
        {
            os << ctx.ind << "if (!(" << rowFilter.to_arg() << "))\n";
            ctx.ind_up();
            os << ctx.ind << "continue;\n";
            ctx.ind_down();
        }
        
        os << ctx.ind << "ImGui::PushID(" << itemCount.index_name_or(ctx.codeGen->FOR_VAR_NAME) << ");\n";
    }
    
    os << ctx.ind << "ImGui::TableNextRow(0, ";
    if (!rowHeight.empty())
        os << rowHeight.to_arg(ctx.unit);
    else
        os << "0";
    os << ");\n";
    os << ctx.ind << "ImGui::TableSetColumnIndex(0);\n";
    
    if (!onBeginRow.empty())
        os << ctx.ind << onBeginRow.to_arg() << "();\n";

    os << ctx.ind << "/// @separator\n\n";

    for (auto& child : child_iterator(children, false))
        child->Export(os, ctx);

    os << "\n" << ctx.ind << "/// @separator\n";
    
    if (!onEndRow.empty())
        os << ctx.ind << onEndRow.to_arg() << "();\n";

    if (!itemCount.empty()) {
        os << ctx.ind << "ImGui::PopID();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }

    if (child_iterator(children, true))
    {
        //draw overlay children at the end so they are visible,
        //inside table's child because ItemOverlap works only between items in same window
        os << ctx.ind << "ImGui::GetCurrentWindow()->SkipItems = false;\n";
        os << ctx.ind << "ImGui::PushClipRect(ImGui::GetCurrentWindow()->InnerRect.Min, ImGui::GetCurrentWindow()->InnerRect.Max, false);\n";
        os << ctx.ind << "/// @separator\n\n";

        for (auto& child : child_iterator(children, true))
            child->Export(os, ctx);
    
        os << ctx.ind << "/// @separator\n";
        os << ctx.ind << "ImGui::PopClipRect();\n";
    }

    os << ctx.ind << "ImGui::EndTable();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_rowBg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_rowBgAlt.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_childBg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_headerBg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (style_cellPadding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (scrollWhenDragging)
        os << ctx.ind << "ImRad::PopInvisibleScrollbar();\n";

    ++ctx.varCounter;
}

void Table::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
    {
        if (sit->params.size() && sit->params[0] == "ImGuiStyleVar_CellPadding")
            style_cellPadding.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() && sit->params[0] == "ImGuiCol_TableHeaderBg")
            style_headerBg.set_from_arg(sit->params[1]);
        if (sit->params.size() && sit->params[0] == "ImGuiCol_TableRowBg")
            style_rowBg.set_from_arg(sit->params[1]);
        if (sit->params.size() && sit->params[0] == "ImGuiCol_TableRowBgAlt")
            style_rowBgAlt.set_from_arg(sit->params[1]);
        if (sit->params.size() && sit->params[0] == "ImGuiCol_ChildBg")
            style_childBg.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTable")
    {
        header = false;
        columnData.clear();
        ctx.importLevel = sit->level;

        /*if (sit->params.size() >= 2) {
            std::istringstream is(sit->params[1]);
            size_t n = 3;
            is >> n;
        }*/

        if (sit->params.size() >= 3)
            flags.set_from_arg(sit->params[2]);

        if (sit->params.size() >= 4) {
            auto size = cpp::parse_size(sit->params[3]);
            size_x.set_from_arg(size.first);
            size_y.set_from_arg(size.second);
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::ScrollWhenDragging")
    {
        scrollWhenDragging = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableSetupColumn")
    {
        ColumnData cd;
        cd.label = cpp::parse_str_arg(sit->params[0]);
        
        if (sit->params.size() >= 2)
            cd.flags.set_from_arg(sit->params[1]);
        
        if (sit->params.size() >= 3) {
            std::istringstream is(sit->params[2]);
            is >> cd.width;
        }
        
        columnData.push_back(std::move(cd));
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableHeadersRow")
    {
        header = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableNextRow")
    {
        if (sit->params.size() >= 2)
            rowHeight.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::ForBlock)
    {
        itemCount.set_from_arg(sit->line);
    }
    else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel + 1 + !itemCount.empty() &&
        columnData.size() &&
        sit->callee.compare(0, 7, "ImGui::") &&
        sit->callee.compare(0, 7, "ImRad::"))
    {
        if (!onBeginRow.empty() || children.size()) //todo
            onEndRow.set_from_arg(sit->callee);
        else
            onBeginRow.set_from_arg(sit->callee);
    }
    else if (sit->kind == cpp::IfStmt && sit->level == ctx.importLevel + 1 + !itemCount.empty() &&
        columnData.size() &&
        !sit->cond.compare(0, 2, "!(") && sit->cond.back() == ')')
    {
        rowFilter.set_from_arg(sit->cond.substr(2, sit->cond.size() - 3));
    }
}

//---------------------------------------------------------

Child::Child(UIContext& ctx)
{
    //zero size will be rendered wrongly?
    //it seems 0 is equivalent to -1 but only if children exist which is confusing
    size_x = size_y = 20;

    flags.add$(ImGuiChildFlags_Borders);
    flags.add$(ImGuiChildFlags_AlwaysUseWindowPadding);
    flags.add$(ImGuiChildFlags_AlwaysAutoResize);
    flags.add$(ImGuiChildFlags_AutoResizeX);
    flags.add$(ImGuiChildFlags_AutoResizeY);
    flags.add$(ImGuiChildFlags_FrameStyle);
    flags.add$(ImGuiChildFlags_NavFlattened);

    wflags.add$(ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    wflags.add$(ImGuiWindowFlags_AlwaysVerticalScrollbar);
    wflags.add$(ImGuiWindowFlags_NoBackground);
    wflags.add$(ImGuiWindowFlags_NoNavInputs);
    wflags.add$(ImGuiWindowFlags_NoSavedSettings);
    wflags.add$(ImGuiWindowFlags_NoScrollbar);
}

std::unique_ptr<Widget> Child::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Child>(*this);
    //itemCount is shared
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

int Child::Behavior() 
{
    int fl = Widget::Behavior() | SnapInterior;
    if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) || !(flags & ImGuiChildFlags_AutoResizeX))
        fl |= HasSizeX;
    if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) || !(flags & ImGuiChildFlags_AutoResizeY))
        fl |= HasSizeY;
    return fl;
}

ImDrawList* Child::DoDraw(UIContext& ctx)
{
    if (flags & ImGuiWindowFlags_AlwaysAutoResize) 
    {
        ctx.isAutoSize = true;
        HashCombineData(ctx.layoutHash, true);
    }

    if (style_padding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style_padding.eval_px(ctx));
    if (style_rounding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, style_rounding.eval_px(ctx));
    if (style_borderSize.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, style_borderSize.eval_px(ctx));
    if (!style_bg.empty())
        ImGui::PushStyleColor(ImGuiCol_ChildBg, style_bg.eval(ImGuiCol_ChildBg, ctx));
    
    ImVec2 sz;
    sz.x = size_x.eval_px(ImGuiAxis_X, ctx);
    sz.y = size_y.eval_px(ImGuiAxis_Y, ctx);
    if (!sz.x && children.empty())
        sz.x = 30;
    if (!sz.y && children.empty())
        sz.y = 30;
    
    ImRad::IgnoreWindowPaddingData data;
    if (!style_outerPadding)
        ImRad::PushIgnoreWindowPadding(&sz, &data);
    
    //after calling BeginChild, win->ContentSize and scrollbars are updated and drawn
    //only way how to disable scrollbars when using !style_outer_padding is to use
    //ImGuiWindowFlags_NoScrollbar
    
    ImGui::SetNextWindowScroll({ 0, 0 }); //click on a child of child causes scrolling which doesn't go back
    ImGui::BeginChild("", sz, flags, wflags);

    if (style_spacing.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style_spacing);
    
    if (columnCount.has_value() && columnCount.value() >= 2)
        ImGui::Columns(columnCount.value(), "columns", columnBorder);

    for (size_t i = 0; i < children.size(); ++i)
    {
        children[i]->Draw(ctx);
    }
        
    if (style_spacing.has_value())
        ImGui::PopStyleVar();
    
    ImGui::EndChild();

    if (!style_outerPadding) 
        ImRad::PopIgnoreWindowPadding(data);

    if (!style_bg.empty())
        ImGui::PopStyleColor();
    if (style_rounding.has_value())
        ImGui::PopStyleVar();
    if (style_padding.has_value())
        ImGui::PopStyleVar();
    if (style_borderSize.has_value())
        ImGui::PopStyleVar();
    
    return ImGui::GetWindowDrawList();
}

void Child::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = ImGui::GetItemRectMin();
    cached_size = ImGui::GetItemRectSize();
}

void Child::DoExport(std::ostream& os, UIContext& ctx)
{
    std::string datavar, szvar;
    if (!style_outerPadding)
    {
        datavar = "_data" + std::to_string(ctx.varCounter);
        szvar = "_sz" + std::to_string(ctx.varCounter);
        os << ctx.ind << "ImVec2 " << szvar << "{ "
            << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
            << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " };\n";
        os << ctx.ind << "ImRad::IgnoreWindowPaddingData " << datavar << ";\n";
        os << ctx.ind << "ImRad::PushIgnoreWindowPadding(&" << szvar << ", &" << datavar << ");\n";
    }

    if (style_padding.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, " << style_padding.to_arg(ctx.unit) << ");\n";
    if (style_rounding.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, " << style_rounding.to_arg(ctx.unit) << ");\n";
    if (style_borderSize.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, " << style_borderSize.to_arg(ctx.unit) << ");\n";
    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, " << style_bg.to_arg() << ");\n";
    
    os << ctx.ind << "ImGui::BeginChild(\"child" << ctx.varCounter << "\", ";
    if (szvar != "")
        os << szvar << ", ";
    else {
        os << "{ " << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
            << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }, ";
    }
    os << flags.to_arg() << ", " << wflags.to_arg() << ");\n";

    os << ctx.ind << "{\n";
    ctx.ind_up();
    ++ctx.varCounter;

    if (scrollWhenDragging)
        os << ctx.ind << "ImRad::ScrollWhenDragging(false);\n";
    if (style_spacing.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, " << style_spacing.to_arg(ctx.unit) << ");\n";

    bool hasColumns = !columnCount.has_value() || columnCount.value() >= 2;
    if (hasColumns) {
        os << ctx.ind << "ImGui::Columns(" << columnCount.to_arg() << ", \"\", "
            << columnBorder.to_arg() << ");\n";
        //for (size_t i = 0; i < columnsWidths.size(); ++i)
            //os << ctx.ind << "ImGui::SetColumnWidth(" << i << ", " << columnsWidths[i].c_str() << ");\n";
    }

    if (!itemCount.empty()) {
        os << ctx.ind << itemCount.to_arg(ctx.codeGen->FOR_VAR_NAME) << "\n" << ctx.ind << "{\n";
        ctx.ind_up();
    }
    
    os << ctx.ind << "/// @separator\n\n";

    for (auto& child : child_iterator(children, false))
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";

    if (!itemCount.empty()) {
        if (hasColumns)
            os << ctx.ind << "ImGui::NextColumn();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }

    if (style_spacing.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    
    if (child_iterator(children, true)) {
        os << ctx.ind << "/// @separator\n\n";

        for (auto& child : child_iterator(children, true))
            child->Export(os, ctx);

        os << ctx.ind << "/// @separator\n";
    }

    os << ctx.ind << "ImGui::EndChild();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (style_rounding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_padding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_borderSize.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";

    if (!style_outerPadding)
        os << ctx.ind << "ImRad::PopIgnoreWindowPadding(" << datavar << ");\n";

}

void Child::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_ChildBg")
            style_bg.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
    {
        if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_WindowPadding")
            style_padding.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ItemSpacing")
            style_spacing.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ChildRounding")
            style_rounding.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ChildBorderSize")
            style_borderSize.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::Other && 
        (!sit->line.compare(0, 10, "ImVec2 _sz") || !sit->line.compare(0, 9, "ImVec2 sz"))) //sz for compatibility
    {
        style_outerPadding = false;
        size_t i = sit->line.find('{');
        if (i != std::string::npos) {
            auto size = cpp::parse_size(sit->line.substr(i));
            size_x.set_from_arg(size.first);
            size_y.set_from_arg(size.second);
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginChild")
    {
        if (sit->params.size() >= 2) {
            auto size = cpp::parse_size(sit->params[1]);
            if (size.first != "" && size.second != "") {
                size_x.set_from_arg(size.first);
                size_y.set_from_arg(size.second);
            }
        }

        if (sit->params.size() >= 3)
            flags.set_from_arg(sit->params[2]);
        if (sit->params.size() >= 4)
            wflags.set_from_arg(sit->params[3]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Columns")
    {
        if (sit->params.size())
            *columnCount.access() = sit->params[0];

        if (sit->params.size() >= 3)
            columnBorder = sit->params[2] == "true";
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::ScrollWhenDragging")
    {
        scrollWhenDragging = true;
    }
    else if (sit->kind == cpp::ForBlock)
    {
        itemCount.set_from_arg(sit->line);
    }
}


std::vector<UINode::Prop>
Child::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.color", &style_bg },
        { "appearance.border", &style_border },
        { "appearance.padding", &style_padding },
        { "appearance.spacing", &style_spacing },
        { "appearance.rounding", &style_rounding },
        { "appearance.borderSize", &style_borderSize },
        { "appearance.outer_padding", &style_outerPadding },
        { "appearance.column_border##child", &columnBorder },
        { "behavior.flags##child", &flags },
        { "behavior.wflags##child", &wflags },
        { "behavior.column_count##child", &columnCount },
        { "behavior.item_count##child", &itemCount.limit },
        { "behavior.scrollWhenDragging", &scrollWhenDragging },
        { "bindings.itemIndex##1", &itemCount.index },
        });
    return props;
}

bool Child::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("color");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_bg, ImGuiCol_ChildBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_bg, ctx);
        break;
    case 1:
        ImGui::Text("border");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_border, ImGuiCol_Border, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("border", &style_border, ctx);
        break;
    case 2:
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_padding, ctx);
        break;
    case 3:
        ImGui::Text("spacing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_spacing, ctx);
        break;
    case 4:
        ImGui::Text("rounding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_rounding, ctx);
        break;
    case 5:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_borderSize, ctx);
        break;
    case 6:
        ImGui::Text("outerPadding");
        ImGui::TableNextColumn();
        fl = style_outerPadding != Defaults().style_outerPadding ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&style_outerPadding, fl, ctx);
        break;
    case 7:
        ImGui::Text("columnBorder");
        ImGui::TableNextColumn();
        fl = columnBorder != Defaults().columnBorder ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&columnBorder, fl, ctx);
        break;
    case 8:
        TreeNodeProp("flags", ctx.pgbFont, "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            int ch = CheckBoxFlags(&flags, Defaults().flags);
            if (ch) {
                changed = true;
                //these flags are difficult to get right and there are asserts so fix it here
                if (ch == ImGuiChildFlags_AutoResizeX || ch == ImGuiChildFlags_AutoResizeY) {
                    if (flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY))
                        flags |= ImGuiChildFlags_AlwaysAutoResize;
                    else
                        flags &= ~ImGuiChildFlags_AlwaysAutoResize;
                }
                if (ch == ImGuiChildFlags_AlwaysAutoResize) {
                    if (flags & ImGuiChildFlags_AlwaysAutoResize) {
                        if (!(flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)))
                            flags |= ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
                    }
                    else
                        flags &= ~(ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
                }
                //
                if ((flags & ImGuiChildFlags_AlwaysAutoResize) && (flags & ImGuiChildFlags_AutoResizeX))
                    size_x = 0;
                if ((flags & ImGuiChildFlags_AlwaysAutoResize) && (flags & ImGuiChildFlags_AutoResizeY))
                    size_y = 0;
            }
            });
        break;
    case 9:
        TreeNodeProp("windowFlags", ctx.pgbFont, "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            changed = CheckBoxFlags(&wflags, Defaults().flags);
            });
        break;
    case 10:
        ImGui::Text("columnCount");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = columnCount != Defaults().columnCount  ? InputBindable_Modified : 0;
        changed = InputBindable(&columnCount, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("columnCount", &columnCount, ctx);
        break;
    case 11:
        ImGui::Text("itemCount");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDataSize(&itemCount.limit, true, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("itemCount", &itemCount.limit, ctx);
        break;
    case 12:
        ImGui::Text("scrollWhenDragging");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = scrollWhenDragging != Defaults().scrollWhenDragging ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&scrollWhenDragging, fl, ctx);
        break;
    case 13:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("itemIndex");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&itemCount.index, true, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::PropertyUI(i - 14, ctx);
    }
    return changed;
}

//---------------------------------------------------------

Splitter::Splitter(UIContext& ctx)
{
    size_x = size_y = -1;

    if (ctx.createVars) 
        position.set_from_arg(ctx.codeGen->CreateVar("float", "100", CppGen::Var::Interface));
}

std::unique_ptr<Widget> Splitter::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Splitter>(*this);
    if (!position.empty() && ctx.createVars) {
        sel->position.set_from_arg(ctx.codeGen->CreateVar("float", "100", CppGen::Var::Interface));
    }
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* Splitter::DoDraw(UIContext& ctx)
{
    ImVec2 size;
    size.x = size_x.eval_px(ImGuiAxis_X, ctx);
    size.y = size_y.eval_px(ImGuiAxis_Y, ctx);
    
    if (!style_bg.empty())
        ImGui::PushStyleColor(ImGuiCol_ChildBg, style_bg.eval(ImGuiCol_ChildBg, ctx));
    ImGui::BeginChild("splitter", size);
    
    ImGuiAxis axis = ImGuiAxis_X;
    float th = 0, pos = 0;
    if (children.size() == 2) {
        axis = children[1]->sameLine ? ImGuiAxis_X : ImGuiAxis_Y;
        pos = children[0]->cached_pos[axis] + children[0]->cached_size[axis];
        th = children[1]->cached_pos[axis] - children[0]->cached_pos[axis] - children[0]->cached_size[axis];
    }
    
    ImGui::PushStyleColor(ImGuiCol_Separator, 0x00000000);
    ImRad::Splitter(axis == ImGuiAxis_X, th, &pos, min_size1, min_size2);
    ImGui::PopStyleColor();

    for (size_t i = 0; i < children.size(); ++i)
        children[i]->Draw(ctx);
    
    ImGui::EndChild();
    if (!style_bg.empty())
        ImGui::PopStyleColor();

    return ImGui::GetWindowDrawList();
}

void Splitter::DoExport(std::ostream& os, UIContext& ctx)
{
    if (children.size() != 2)
        ctx.errors.push_back("Splitter: need exactly 2 children");
    if (position.empty())
        ctx.errors.push_back("Splitter: position is unassigned");

    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, " << style_bg.to_arg() << ");\n";
    
    os << ctx.ind << "ImGui::BeginChild(\"splitter" << ctx.varCounter << "\", { "
        << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", " 
        << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1])
        << " });\n" << ctx.ind << "{\n";
    ctx.ind_up();
    ++ctx.varCounter;
    
    os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Separator, 0x00000000);\n";
    os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, 0x00000000);\n";
    if (!style_active.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_SeparatorActive, " << style_active.to_arg() << ");\n";

    bool axisX = true;
    direct_val<dimension> th = ImGui::GetStyle().ItemSpacing.x;
    if (children.size() == 2) {
        axisX = children[1]->sameLine;
        th = children[1]->cached_pos[!axisX] - children[0]->cached_pos[!axisX] - children[0]->cached_size[!axisX];
    }

    os << ctx.ind << "ImRad::Splitter(" 
        << std::boolalpha << axisX 
        << ", " << th.to_arg(ctx.unit)
        << ", &" << position.to_arg()
        << ", " << min_size1.to_arg(ctx.unit)
        << ", " << min_size2.to_arg(ctx.unit)
        << ");\n";

    os << ctx.ind << "ImGui::PopStyleColor();\n";
    os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_active.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";

    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";
    os << ctx.ind << "ImGui::EndChild();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void Splitter::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginChild")
    {
        if (sit->params.size() >= 2) {
            auto sz = cpp::parse_size(sit->params[1]);
            size_x.set_from_arg(sz.first);
            size_y.set_from_arg(sz.second);
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::Splitter")
    {
        if (sit->params.size() >= 3 && !sit->params[2].compare(0, 1, "&"))
            position.set_from_arg(sit->params[2].substr(1));
        if (sit->params.size() >= 4)
            min_size1.set_from_arg(sit->params[3]);
        if (sit->params.size() >= 5)
            min_size2.set_from_arg(sit->params[4]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_ChildBg")
            style_bg.set_from_arg(sit->params[1]);
        if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_SeparatorActive")
            style_active.set_from_arg(sit->params[1]);
    }
}


std::vector<UINode::Prop>
Splitter::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.bg", &style_bg },
        { "appearance.active", &style_active },
        { "behavior.min1##splitter", &min_size1 },
        { "behavior.min2##splitter", &min_size2 },
        { "bindings.sashPosition##1", &position },
        });
    return props;
}

bool Splitter::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("bg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_bg, ImGuiCol_ChildBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_bg, ctx);
        break;
    case 1:
        ImGui::Text("active");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_active, ImGuiCol_SeparatorActive, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("active", &style_active, ctx);
        break;
    case 2:
        ImGui::Text("minSize1");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = min_size1 != Defaults().min_size1 ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&min_size1, fl, ctx);
        break;
    case 3:
        ImGui::Text("minSize2");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = min_size2 != Defaults().min_size2 ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&min_size2, fl, ctx);
        break;
    case 4:
        ImGui::Text("sashPosition");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&position, false, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 5, ctx);
    }
    return changed;
}

//---------------------------------------------------------

CollapsingHeader::CollapsingHeader(UIContext& ctx)
{
    flags.add$(ImGuiTreeNodeFlags_Bullet);
    flags.add$(ImGuiTreeNodeFlags_DefaultOpen);
    //flags.add$(ImGuiTreeNodeFlags_Framed);
    flags.add$(ImGuiTreeNodeFlags_FramePadding);
    //flags.add$(ImGuiTreeNodeFlags_Leaf);
    //flags.add$(ImGuiTreeNodeFlags_NoTreePushOnOpen);
    flags.add$(ImGuiTreeNodeFlags_OpenOnArrow);
    flags.add$(ImGuiTreeNodeFlags_OpenOnDoubleClick);
    flags.add$(ImGuiTreeNodeFlags_SpanAllColumns);
    flags.add$(ImGuiTreeNodeFlags_SpanAvailWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanFullWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanTextWidth);
}

std::unique_ptr<Widget> CollapsingHeader::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<CollapsingHeader>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* CollapsingHeader::DoDraw(UIContext& ctx)
{
    if (!style_header.empty())
        ImGui::PushStyleColor(ImGuiCol_Header, style_header.eval(ImGuiCol_Header, ctx));
    if (!style_hovered.empty())
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, style_hovered.eval(ImGuiCol_HeaderHovered, ctx));
    if (!style_active.empty())
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, style_active.eval(ImGuiCol_HeaderActive, ctx));
    
    //automatically expand to show selected child and collapse to save space
    //in the window and avoid potential scrolling
    if (ctx.selected.size() == 1)
    {
        ImGui::SetNextItemOpen((bool)FindChild(ctx.selected[0]));
    }
    if (ImGui::CollapsingHeader(DRAW_STR(label), flags))
    {
        for (size_t i = 0; i < children.size(); ++i)
        {
            children[i]->Draw(ctx);
        }
    }

    if (!style_header.empty())
        ImGui::PopStyleColor();
    if (!style_hovered.empty())
        ImGui::PopStyleColor();
    if (!style_active.empty())
        ImGui::PopStyleColor();

    return ImGui::GetWindowDrawList();
}

void CollapsingHeader::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = p1;
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    cached_pos.x -= pad.x;
    cached_size.x = ImGui::GetContentRegionAvail().x + 2 * pad.x;
    cached_size.y = ImGui::GetCursorScreenPos().y - p1.y - pad.y;
}

void CollapsingHeader::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!style_header.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Header, " << style_header.to_arg() << ");\n";
    if (!style_hovered.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_HeaderHovered, " << style_hovered.to_arg() << ");\n";
    if (!style_active.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_HeaderActive, " << style_active.to_arg() << ");\n";

    if (!open.empty())
        os << ctx.ind << "ImGui::SetNextItemOpen(" << open.to_arg() << ");\n";

    os << ctx.ind << "if (ImGui::CollapsingHeader(" << label.to_arg() << ", "
        << flags.to_arg() << "))\n";
    os << ctx.ind << "{\n";
    ctx.ind_up();

    os << ctx.ind << "/// @separator\n\n";

    for (auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";

    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_header.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_hovered.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_active.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void CollapsingHeader::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_Header")
            style_header.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_HeaderHovered")
            style_hovered.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_HeaderActive")
            style_active.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemOpen")
    {
        if (sit->params.size() >= 1)
            open.set_from_arg(sit->params[0]);
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::CollapsingHeader")
    {
        if (sit->params.size() >= 1)
            label.set_from_arg(sit->params[0]);
        if (sit->params.size() >= 2)
            flags.set_from_arg(sit->params[1]);
    }
}

std::vector<UINode::Prop>
CollapsingHeader::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.header", &style_header },
        { "appearance.hovered", &style_hovered },
        { "appearance.active", &style_active },
        { "appearance.font", &style_font },
        { "behavior.flags##coh", &flags },
        { "behavior.label", &label, true },
        { "behavior.open", &open }
        });
    return props;
}

bool CollapsingHeader::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("header");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_header, ImGuiCol_Header, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("header", &style_header, ctx);
        break;
    case 2:
        ImGui::Text("hovered");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_hovered, ImGuiCol_HeaderHovered, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("hovered", &style_hovered, ctx);
        break;
    case 3:
        ImGui::Text("active");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_active, ImGuiCol_HeaderActive, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("active", &style_active, ctx);
        break;
    case 4:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 5:
        TreeNodeProp("flags", ctx.pgbFont, "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            changed = CheckBoxFlags(&flags, Defaults().flags);
            });
        break;
    case 6:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 7:
        ImGui::Text("open");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = open != Defaults().open ? InputBindable_Modified : 0;
        changed = InputBindable(&open, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("open", &open, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 8, ctx);
    }
    return changed;
}

//---------------------------------------------------------

TreeNode::TreeNode(UIContext& ctx)
{
    flags.add$(ImGuiTreeNodeFlags_Bullet);
    flags.add$(ImGuiTreeNodeFlags_DefaultOpen);
    //flags.add$(ImGuiTreeNodeFlags_Framed);
    flags.add$(ImGuiTreeNodeFlags_FramePadding);
    flags.add$(ImGuiTreeNodeFlags_Leaf);
    flags.add$(ImGuiTreeNodeFlags_NoTreePushOnOpen);
    flags.add$(ImGuiTreeNodeFlags_OpenOnArrow);
    flags.add$(ImGuiTreeNodeFlags_OpenOnDoubleClick);
    flags.add$(ImGuiTreeNodeFlags_SpanAllColumns);
    flags.add$(ImGuiTreeNodeFlags_SpanAvailWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanFullWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanTextWidth);
}

std::unique_ptr<Widget> TreeNode::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<TreeNode>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* TreeNode::DoDraw(UIContext& ctx)
{
    if (ctx.selected.size() == 1)
    {
        //automatically expand to show selection and collapse to save space
        //in the window and avoid potential scrolling
        ImGui::SetNextItemOpen((bool)FindChild(ctx.selected[0]));
    }
    lastOpen = false;
    if (ImGui::TreeNodeEx(DRAW_STR(label), flags))
    {
        lastOpen = true;
        for (const auto& child : children)
            child->Draw(ctx);

        ImGui::TreePop();
    }

    return ImGui::GetWindowDrawList();
}

void TreeNode::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = p1;
    cached_size = ImGui::GetItemRectSize();
    ImVec2 sp = ImGui::GetStyle().ItemSpacing;
    
    if (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth))
    {
        //todo cached_size.x = ImGui::GetContentRegionAvail().x - p1.x + sp.x;
    }
    else
    {
        cached_size.x = ImGui::CalcTextSize(label.c_str(), 0, true).x + 4 * sp.x;
    }

    if (lastOpen)
    {
        for (const auto& child : children)
        {
            auto p2 = child->cached_pos + child->cached_size;
            cached_size.x = std::max(cached_size.x, p2.x - cached_pos.x);
            cached_size.y = std::max(cached_size.y, p2.y - cached_pos.y);
        }
    }
}

void TreeNode::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!open.empty())
        os << ctx.ind << "ImGui::SetNextItemOpen(" << open.to_arg() << ");\n";

    os << ctx.ind << "if (ImGui::TreeNodeEx(" << label.to_arg() << ", " << flags.to_arg() << "))\n";
    os << ctx.ind << "{\n";

    ctx.ind_up();
    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
    {
        child->Export(os, ctx);
    }

    os << ctx.ind << "/// @separator\n";
    os << ctx.ind << "ImGui::TreePop();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";
}

void TreeNode::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::TreeNodeEx")
    {
        if (sit->params.size() >= 1)
            label.set_from_arg(sit->params[0]);

        if (sit->params.size() >= 2)
            flags.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemOpen")
    {
        if (sit->params.size())
            open.set_from_arg(sit->params[0]);
    }
}

std::vector<UINode::Prop>
TreeNode::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.font", &style_font },
        { "behavior.flags", &flags },
        { "behavior.label", &label, true },
        { "behavior.open", &open },
        });
    return props;
}

bool TreeNode::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 2:
        TreeNodeProp("flags", ctx.pgbFont, "...", [&]{
            ImGui::TableNextColumn();
            ImGui::Spacing();
            changed = CheckBoxFlags(&flags, Defaults().flags);
            });
        break;
    case 3:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 4:
        ImGui::Text("open");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = open != Defaults().open ? InputBindable_Modified : 0;
        changed = InputBindable(&open, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("open", &open, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 5, ctx);
    }
    return changed;
}

//--------------------------------------------------------------

TabBar::TabBar(UIContext& ctx)
{
    flags.add$(ImGuiTabBarFlags_DrawSelectedOverline);
    flags.add$(ImGuiTabBarFlags_FittingPolicyResizeDown);
    flags.add$(ImGuiTabBarFlags_FittingPolicyScroll);
    flags.add$(ImGuiTabBarFlags_NoTabListScrollingButtons);
    flags.add$(ImGuiTabBarFlags_Reorderable);
    flags.add$(ImGuiTabBarFlags_TabListPopupButton);

    if (ctx.createVars)
        children.push_back(std::make_unique<TabItem>(ctx));
}

std::unique_ptr<Widget> TabBar::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<TabBar>(*this);
    //itemCount can be shared
    if (!activeTab.empty() && ctx.createVars) {
        sel->activeTab.set_from_arg(ctx.codeGen->CreateVar("int", "", CppGen::Var::Interface));
    }
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* TabBar::DoDraw(UIContext& ctx)
{
    if (!style_tab.empty())
        ImGui::PushStyleColor(ImGuiCol_Tab, style_tab.eval(ImGuiCol_Tab, ctx));
    if (!style_hovered.empty())
        ImGui::PushStyleColor(ImGuiCol_TabHovered, style_hovered.eval(ImGuiCol_TabHovered, ctx));
    if (!style_selected.empty())
        ImGui::PushStyleColor(ImGuiCol_TabSelected, style_selected.eval(ImGuiCol_TabSelected, ctx));
    if (!style_tabDimmed.empty())
        ImGui::PushStyleColor(ImGuiCol_TabDimmed, style_tab.eval(ImGuiCol_TabDimmed, ctx));
    if (!style_dimmedSelected.empty())
        ImGui::PushStyleColor(ImGuiCol_TabDimmedSelected, style_dimmedSelected.eval(ImGuiCol_TabDimmedSelected, ctx));
    if (!style_overline.empty())
        ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline, style_overline.eval(ImGuiCol_TabSelectedOverline, ctx));
    if (!style_framePadding.empty())
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, style_framePadding.eval_px(ctx));

    std::string id = "##" + std::to_string((uintptr_t)this);
    //add tab names in order so that when tab order changes in designer we get a 
    //different id which will force ImGui to recalculate tab positions and
    //render tabs correctly
    for (const auto& child : children)
        id += std::to_string(uintptr_t(child.get()) & 0xffff);

    if (ImGui::BeginTabBar(id.c_str(), flags))
    {
        for (const auto& child : children)
            child->Draw(ctx);
            
        ImGui::EndTabBar();
    }

    if (!style_framePadding.empty())
        ImGui::PopStyleVar();
    if (!style_tab.empty())
        ImGui::PopStyleColor();
    if (!style_hovered.empty())
        ImGui::PopStyleColor();
    if (!style_selected.empty())
        ImGui::PopStyleColor();
    if (!style_tabDimmed.empty())
        ImGui::PopStyleColor();
    if (!style_dimmedSelected.empty())
        ImGui::PopStyleColor();
    if (!style_overline.empty())
        ImGui::PopStyleColor();

    return ImGui::GetWindowDrawList();
}

void TabBar::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = p1;
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    cached_pos.x -= pad.x;
    cached_size.x = ImGui::GetContentRegionAvail().x + 2 * pad.x;
    cached_size.y = ImGui::GetCursorScreenPos().y - p1.y;
}

void TabBar::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!style_tab.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Tab, " << style_tab.to_arg() << ");\n";
    if (!style_hovered.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TabHovered, " << style_hovered.to_arg() << ");\n";
    if (!style_selected.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TabSelected, " << style_selected.to_arg() << ");\n";
    if (!style_overline.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline, " << style_overline.to_arg() << ");\n";

    os << ctx.ind << "if (ImGui::BeginTabBar(\"tabBar" << ctx.varCounter << "\", "
        << flags.to_arg() << "))\n";
    os << ctx.ind << "{\n";
    ++ctx.varCounter;
    ctx.ind_up();

    if (style_regularWidth)
    {
        os << ctx.ind << "int _nTabs = std::max(1, ImGui::GetCurrentTabBar()->ActiveTabs);\n"; //respects hidden tabs
        os << ctx.ind << "float _tabWidth = (ImGui::GetContentRegionAvail().x - (_nTabs - 1) * ImGui::GetStyle().ItemInnerSpacing.x) / _nTabs - 1;\n";
    }

    if (!itemCount.empty())
    {
        os << ctx.ind << itemCount.to_arg(ctx.codeGen->FOR_VAR_NAME) << "\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();
        //BeginTabBar does this
        //os << ctx.ind << "ImGui::PushID(" << itemCount.index_name_or(ctx.codeGen->DefaultForVarName()) << ");\n";
    }
    os << ctx.ind << "/// @separator\n\n";
    
    for (const auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";
    if (!itemCount.empty())
    {
        //os << ctx.ind << "ImGui::PopID();\n"; EndTabBar does this
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    
    os << ctx.ind << "ImGui::EndTabBar();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_tab.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_hovered.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_selected.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_overline.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void TabBar::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_Tab")
            style_tab.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_TabHovered")
            style_hovered.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_TabSelected")
            style_selected.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_TabSelectedOverline")
            style_overline.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTabBar")
    {
        ctx.importLevel = sit->level;

        if (sit->params.size() >= 2)
            flags.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::ForBlock && sit->level == ctx.importLevel + 1)
    {
        itemCount.set_from_arg(sit->line);
    }
}

std::vector<UINode::Prop>
TabBar::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.tab", &style_tab },
        { "appearance.hovered", &style_hovered },
        { "appearance.selected", &style_selected },
        { "appearance.overline", &style_overline },
        { "appearance.regularWidth", &style_regularWidth },
        { "appearance.padding", &style_framePadding },
        { "appearance.font", &style_font },
        { "behavior.flags", &flags },
        { "behavior.tabCount", &itemCount.limit },
        { "bindings.tabIndex##1", &itemCount.index },
        { "bindings.activeTab##1", &activeTab },
        });
    return props;
}

bool TabBar::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("tab");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_tab, ImGuiCol_Tab, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_tab, ctx);
        break;
    case 2:
        ImGui::Text("hovered");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_hovered, ImGuiCol_TabHovered, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("hovered", &style_hovered, ctx);
        break;
    case 3:
        ImGui::Text("selected");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_selected, ImGuiCol_TabSelected, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("selected", &style_selected, ctx);
        break;
    case 4:
        ImGui::Text("overline");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_overline, ImGuiCol_TabSelectedOverline, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("overline", &style_overline, ctx);
        break;
    case 5:
        ImGui::Text("regularWidth");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_regularWidth, 0, ctx);
        break;
    case 6:
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_framePadding, ctx);
        break;
    case 7:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 8:
        TreeNodeProp("flags", ctx.pgbFont, "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            changed = CheckBoxFlags(&flags, Defaults().flags);
            });
        break;
    case 9:
        ImGui::Text("tabCount");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDataSize(&itemCount.limit, true, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("tabCount", &itemCount.limit, ctx);
        break;
    case 10:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("tabIndex");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&itemCount.index, true, ctx);
        ImGui::EndDisabled();
        break;
    case 11:
        ImGui::Text("activeTab");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&activeTab, true, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 12, ctx);
    }
    return changed;
}

//---------------------------------------------------------

TabItem::TabItem(UIContext& ctx)
{
}

std::unique_ptr<Widget> TabItem::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<TabItem>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* TabItem::DoDraw(UIContext& ctx)
{
    auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);
    float tabw = 0;
    if (tb && tb->style_regularWidth) {
        tabw = (ImGui::GetContentRegionAvail().x - (tb->children.size() - 1) * ImGui::GetStyle().ItemInnerSpacing.x) / tb->children.size() - 1;
        ImGui::SetNextItemWidth(tabw);
    }
    //float padx = ImGui::GetCurrentTabBar()->FramePadding.x;
    if (tabw) { 
        float w = ImGui::CalcTextSize(DRAW_STR(label)).x;
        //hack: ImGui::GetCurrentTabBar()->FramePadding.x = std::max(0.f, (tabw - w) / 2);
    }
    
    bool sel = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
    bool tmp = true;
    if (ImGui::BeginTabItem(DRAW_STR(label), closeButton ? &tmp : nullptr, sel ? ImGuiTabItemFlags_SetSelected : 0))
    {
        //ImGui::GetCurrentTabBar()->FramePadding.x = padx;

        for (const auto& child : children)
            child->Draw(ctx);

        ImGui::EndTabItem();
    }
    /*else
        ImGui::GetCurrentTabBar()->FramePadding.x = padx;*/

    return ImGui::GetWindowDrawList();
}

void TabItem::DoDrawTools(UIContext& ctx)
{
    if (ctx.parents.empty())
        return;
    assert(ctx.parents.back() == this);
    auto* parent = ctx.parents[ctx.parents.size() - 2];
    size_t idx = stx::find_if(parent->children, [this](const auto& ch) { return ch.get() == this; })
        - parent->children.begin();

    ImGui::SetNextWindowPos(cached_pos, 0, { 0, 1.f });
    ImGui::Begin("extra", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoSavedSettings);

    ImGui::BeginDisabled(!idx);
    if (ImGui::Button(ICON_FA_ANGLE_LEFT)) {
        auto ptr = std::move(parent->children[idx]);
        parent->children.erase(parent->children.begin() + idx);
        parent->children.insert(parent->children.begin() + idx - 1, std::move(ptr));
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER_PLUS)) {
        parent->children.insert(parent->children.begin() + idx + 1, std::make_unique<TabItem>(ctx));
        if (ctx.selected.size() == 1 && ctx.selected[0] == this)
            ctx.selected[0] = (parent->children.begin() + idx + 1)->get();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(idx + 1 == parent->children.size());
    if (ImGui::Button(ICON_FA_ANGLE_RIGHT)) {
        auto ptr = std::move(parent->children[idx]);
        parent->children.erase(parent->children.begin() + idx);
        parent->children.insert(parent->children.begin() + idx + 1, std::move(ptr));
    }
    ImGui::EndDisabled();

    ImGui::End();
}

void TabItem::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    const ImGuiTabBar* tabBar = ImGui::GetCurrentTabBar();
    cached_pos = tabBar->BarRect.GetTL();
    cached_size.x = tabBar->BarRect.GetWidth();
    cached_size.y = tabBar->BarRect.GetHeight();
    int idx = tabBar->LastTabItemIdx;
    if (idx >= 0) {
        const ImGuiTabItem& tab = tabBar->Tabs[idx];
        cached_pos.x += tab.Offset;
        cached_size.x = tab.Width;
    }
}

void TabItem::DoExport(std::ostream& os, UIContext& ctx)
{
    assert(ctx.parents.back() == this);
    const auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);

    if (tb->style_regularWidth) {
        os << ctx.ind << "ImGui::SetNextItemWidth(_tabWidth);\n";
    }
    
    std::string var = "_open" + std::to_string(ctx.varCounter);
    if (closeButton) {
        os << ctx.ind << "bool " << var << " = true;\n";
        ++ctx.varCounter;
    }

    os << ctx.ind << "if (ImGui::BeginTabItem(" << label.to_arg() << ", ";
    if (closeButton)
        os << "&" << var << ", ";
    else
        os << "nullptr, ";
    std::string idx;
    if (tb && !tb->activeTab.empty())
    {
        if (!tb->itemCount.empty())
        {
            idx = tb->itemCount.index_name_or(ctx.codeGen->FOR_VAR_NAME);
        }
        else
        {
            size_t n = stx::find_if(tb->children, [this](const auto& ch) {
                return ch.get() == this; }) - tb->children.begin();
            idx = std::to_string(n);
        }
        os << tb->activeTab.to_arg() << " == " << idx << " ? ImGuiTabItemFlags_SetSelected : 0";
    }
    else
    {
        os << "ImGuiTabItemFlags_None";
    }
    os << "))\n";
    os << ctx.ind << "{\n";

    ctx.ind_up();
    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
    {
        child->Export(os, ctx);
    }

    os << ctx.ind << "/// @separator\n";
    os << ctx.ind << "ImGui::EndTabItem();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (closeButton && !onClose.empty())
    {
        os << ctx.ind << "if (!" << var << ")\n";
        ctx.ind_up();
        /*if (idx != "") no need to activate, user can check itemCount.index 
            os << ctx.ind << tb->activeTab.to_arg() << " = " << idx << ";\n";*/
        os << ctx.ind << onClose.to_arg() << "();\n";
        ctx.ind_down();
    }
    /*if (idx != "") user can add IsItemActivated event on his own and there is itemCount.index
    {
        os << ctx.ind << "if (ImGui::IsItemActivated())\n";
        ctx.ind_up();
        os << ctx.ind << tb->activeTab.to_arg() << " = " << idx << ";\n";
        ctx.ind_down();
    }*/
}

void TabItem::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    assert(ctx.parents.back() == this);
    auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);
    
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemWidth")
    {
        tb->style_regularWidth = true;
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTabItem")
    {
        if (sit->params.size() >= 1)
            label.set_from_arg(sit->params[0]);

        if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&"))
            closeButton = true;

        if (tb && sit->params.size() >= 3 && sit->params[2].size() > 32)
        {
            const auto& p = sit->params[2];
            size_t i = p.find("==");
            if (i != std::string::npos && p.substr(p.size() - 32, 32) == "?ImGuiTabItemFlags_SetSelected:0")
            {
                std::string var = p.substr(0, i); // i + 2, p.size() - 32 - i - 2);
                tb->activeTab.set_from_arg(var);
            }
        }
    }
    else if (sit->kind == cpp::IfBlock && !sit->cond.compare(0, 5, "!tmpOpen"))
    {
        ctx.importLevel = sit->level;
    }
    else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel)
    {
        onClose.set_from_arg(sit->callee);
    }
}

std::vector<UINode::Prop>
TabItem::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "behavior.label", &label, true },
        { "behavior.closeButton", &closeButton }
        });
    return props;
}

bool TabItem::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 1:
        ImGui::Text("closeButton");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = ImGui::Checkbox("##cb", closeButton.access());
        break;
    default:
        return Widget::PropertyUI(i - 2, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
TabItem::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "tabItem.close", &onClose },
        });
    return props;
}

bool TabItem::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::BeginDisabled(!closeButton);
        ImGui::Text("Closed");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Closed", &onClose, 0, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}

//---------------------------------------------------------

MenuBar::MenuBar(UIContext& ctx)
{
    if (ctx.createVars)
        children.push_back(std::make_unique<MenuIt>(ctx));
}

std::unique_ptr<Widget> MenuBar::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<MenuBar>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* MenuBar::DoDraw(UIContext& ctx)
{
    if (ImGui::BeginMenuBar())
    {
        //for (const auto& child : children) defend against insertion within the loop
        for (size_t i = 0; i < children.size(); ++i)
            children[i]->Draw(ctx);

        ImGui::EndMenuBar();
    }

    //menuBar is drawn from Begin anyways
    return ImGui::GetWindowDrawList();
}

void MenuBar::CalcSizeEx(ImVec2 x1, UIContext& ctx)
{
    cached_pos = ImGui::GetCurrentWindow()->MenuBarRect().GetTL();
    cached_size = ImGui::GetCurrentWindow()->MenuBarRect().GetSize();
}

void MenuBar::DoExport(std::ostream& os, UIContext& ctx)
{
    os << ctx.ind << "if (ImGui::BeginMenuBar())\n";
    os << ctx.ind << "{\n";
    ctx.ind_up();
    
    for (const auto& child : children) {
        MenuIt* it = dynamic_cast<MenuIt*>(child.get());
        if (it) it->ExportAllShortcuts(os, ctx);
    }

    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";
    os << ctx.ind << "ImGui::EndMenuBar();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";
}

void MenuBar::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    auto* win = dynamic_cast<TopWindow*>(ctx.root);
    if (win)
        win->flags |= ImGuiWindowFlags_MenuBar;
}

//---------------------------------------------------------

MenuIt::MenuIt(UIContext& ctx)
{
    //shortcut.set_global(true);
}

std::unique_ptr<Widget> MenuIt::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<MenuIt>(*this);
    if (!checked.empty() && ctx.createVars) {
        sel->checked.set_from_arg(ctx.codeGen->CreateVar("bool", "false", CppGen::Var::Interface));
    }
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* MenuIt::DoDraw(UIContext& ctx)
{
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
    
    if (separator)
        ImGui::Separator();
    
    if (contextMenu)
    {
        ctx.contextMenus.push_back(label);
        bool open = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
        if (open)
        {
            ImGui::SetNextWindowPos(cached_pos);
            if (style_padding.has_value())
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style_padding.eval_px(ctx));
            if (style_spacing.has_value())
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style_spacing.eval_px(ctx));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 
                style_rounding.empty() ? ImGui::GetStyle().PopupRounding : style_rounding.eval_px(ctx));

            std::string id = label + "##" + std::to_string((uintptr_t)this);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
            bool sel = stx::count(ctx.selected, this);
            if (sel)
                ImGui::PushStyleColor(ImGuiCol_Border, ctx.colors[UIContext::Selected]);
            ImGui::Begin(id.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
            {
                if (sel)
                    ImGui::PopStyleColor(); 
                ctx.activePopups.push_back(ImGui::GetCurrentWindow());
                //for (const auto& child : children) defend against insertions within the loop
                for (size_t i = 0; i < children.size(); ++i)
                    children[i]->Draw(ctx);

                ImGui::End();
            }
            ImGui::PopStyleVar();

            ImGui::PopStyleVar();
            if (style_spacing.has_value())
                ImGui::PopStyleVar();
            if (style_padding.has_value())
                ImGui::PopStyleVar();
        }
    }
    else if (children.size()) //normal menu
    {
        assert(ctx.parents.back() == this);
        const UINode* par = ctx.parents[ctx.parents.size() - 2];
        bool mbm = dynamic_cast<const MenuBar*>(par); //menu bar item

        ImGui::MenuItem(label.c_str());

        if (!mbm)
        {
            float w = ImGui::CalcTextSize(ICON_FA_ANGLE_RIGHT).x;
            ImGui::SameLine(0, 0);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - w);
            ImGui::PushFont(nullptr);
            ImGui::Text(ICON_FA_ANGLE_RIGHT);
            ImGui::PopFont();
        }
        bool open = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
        if (open)
        {
            ImVec2 pos = cached_pos;
            if (mbm)
                pos.y += ctx.rootWin->MenuBarHeight;
            else {
                ImVec2 sp = ImGui::GetStyle().ItemSpacing;
                ImVec2 pad = ImGui::GetStyle().WindowPadding;
                pos.x += ImGui::GetWindowSize().x - sp.x;
                pos.y -= pad.y;
            }
            ImGui::SetNextWindowPos(pos);
            std::string id = label + "##" + std::to_string((uintptr_t)this);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
            ImGui::Begin(id.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
            {
                ctx.activePopups.push_back(ImGui::GetCurrentWindow());
                //for (const auto& child : children) defend against insertions within the loop
                for (size_t i = 0; i < children.size(); ++i)
                    children[i]->Draw(ctx);
                ImGui::End();
            }
            ImGui::PopStyleVar();
        }
    }
    else if (ownerDraw)
    {
        std::string s = onChange.to_arg();
        if (s.empty())
            s = "OnDraw not set";
        else
            s += "()";
        ImGui::MenuItem(s.c_str(), nullptr, nullptr, false);
    }
    else //menuItem
    {
        bool check = !checked.empty();
        ImGui::MenuItem(DRAW_STR(label), shortcut.c_str(), check);
    }
    ImGui::PopStyleVar();
    return ImGui::GetWindowDrawList();
}

void MenuIt::DoDrawTools(UIContext& ctx)
{
    if (ctx.parents.empty())
        return;
    assert(ctx.parents.back() == this);
    auto* parent = ctx.parents[ctx.parents.size() - 2];
    if (dynamic_cast<TopWindow*>(parent)) //no helper for context menu
        return;
    bool vertical = !dynamic_cast<MenuBar*>(parent);
    size_t idx = stx::find_if(parent->children, [this](const auto& ch) { return ch.get() == this; })
        - parent->children.begin();

    //draw toolbox
    //currently we always sit it on top of the menu so that it doesn't overlap with submenus
    //no WindowFlags_StayOnTop
    const ImVec2 bsize{ 30, 0 };
    ImVec2 pos = cached_pos;
    if (vertical) {
        pos = ImGui::GetWindowPos();
        //pos.x -= ImGui::GetStyle().ItemSpacing.x;
    }
    ImGui::SetNextWindowPos(pos, 0, vertical ? ImVec2{ 0, 1.f } : ImVec2{ 0, 1.f });
    ImGui::Begin("extra", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoSavedSettings);

    ImGui::BeginDisabled(!idx);
    if (ImGui::Button(vertical ? ICON_FA_ANGLE_UP : ICON_FA_ANGLE_LEFT, bsize)) {
        auto ptr = std::move(parent->children[idx]);
        parent->children.erase(parent->children.begin() + idx);
        parent->children.insert(parent->children.begin() + idx - 1, std::move(ptr));
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button(vertical ? ICON_FA_PLUS ICON_FA_ANGLE_DOWN : ICON_FA_PLUS ICON_FA_ANGLE_RIGHT, bsize)) {
        parent->children.insert(parent->children.begin() + idx + 1, std::make_unique<MenuIt>(ctx));
        if (ctx.selected.size() == 1 && ctx.selected[0] == this)
            ctx.selected[0] = parent->children[idx + 1].get();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(children.size());
    if (ImGui::Button(vertical ? ICON_FA_PLUS ICON_FA_ANGLE_RIGHT : ICON_FA_PLUS ICON_FA_ANGLE_DOWN, bsize)) {
        children.push_back(std::make_unique<MenuIt>(ctx));
        if (ctx.selected.size() == 1 && ctx.selected[0] == this)
            ctx.selected[0] = children[0].get();
    }
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    ImGui::BeginDisabled(idx + 1 == parent->children.size());
    if (ImGui::Button(vertical ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT, bsize)) {
        auto ptr = std::move(parent->children[idx]);
        parent->children.erase(parent->children.begin() + idx);
        parent->children.insert(parent->children.begin() + idx + 1, std::move(ptr));
    }
    ImGui::EndDisabled();

    ImGui::End();
}

void MenuIt::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    if (contextMenu)
    {
        cached_pos = ctx.rootWin->InnerRect.Min;
        cached_size = { 0, 0 }; //won't rect-select
        return;
    }
    
    assert(ctx.parents.back() == this);
    const UINode* par = ctx.parents[ctx.parents.size() - 2];
    bool mbm = dynamic_cast<const MenuBar*>(par);
    ImVec2 sp = ImGui::GetStyle().ItemSpacing;
    cached_pos = p1;
    const ImGuiMenuColumns* mc = &ImGui::GetCurrentWindow()->DC.MenuColumns;
    cached_pos.x += mc->OffsetLabel;
    cached_size = ImGui::CalcTextSize(label.c_str(), nullptr, true);
    cached_size.x += sp.x;
    if (mbm)
    {
        ++cached_pos.y;
        cached_size.y = ctx.rootWin->MenuBarHeight - 2;
    }
    else
    {
        if (separator)
            cached_pos.y += sp.y;
    }
}

void MenuIt::DoExport(std::ostream& os, UIContext& ctx)
{
    assert(ctx.parents.back() == this);
    const UINode* par = ctx.parents[ctx.parents.size() - 2];
    
    if (separator)
        os << ctx.ind << "ImGui::Separator();\n";

    if (contextMenu)
    {
        ExportAllShortcuts(os, ctx);

        if (style_padding.has_value())
            os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, " << style_padding.to_arg(ctx.unit) << ");\n";
        if (style_rounding.has_value())
            os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, " << style_rounding.to_arg(ctx.unit) << ");\n";
        if (style_spacing.has_value())
            os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, " << style_spacing.to_arg(ctx.unit) << ");\n";
        
        os << ctx.ind << "if (ImGui::BeginPopup(" << label.to_arg() << ", "
            << "ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings"
            << "))\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();

        os << ctx.ind << "/// @separator\n\n";

        for (const auto& child : children)
            child->Export(os, ctx);

        os << ctx.ind << "/// @separator\n";
        os << ctx.ind << "ImGui::EndPopup();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";

        if (style_spacing.has_value())
            os << ctx.ind << "ImGui::PopStyleVar();\n";
        if (style_rounding.has_value())
            os << ctx.ind << "ImGui::PopStyleVar();\n";
        if (style_padding.has_value())
            os << ctx.ind << "ImGui::PopStyleVar();\n";
    }
    else if (children.size())
    {
        os << ctx.ind << "if (ImGui::BeginMenu(" << label.to_arg() << "))\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();
        os << ctx.ind << "/// @separator\n\n";

        for (const auto& child : children)
            child->Export(os, ctx);

        os << ctx.ind << "/// @separator\n";
        os << ctx.ind << "ImGui::EndMenu();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    else if (ownerDraw)
    {
        if (onChange.empty())
            ctx.errors.push_back("MenuIt: ownerDraw is set but onChange is empty!");
        os << ctx.ind << onChange.to_arg() << "();\n";
    }
    else
    {
        bool ifstmt = !onChange.empty();
        os << ctx.ind;
        if (ifstmt)
            os << "if (";

        os << "ImGui::MenuItem(" << label.to_arg() << ", \""
            << shortcut.c_str() << "\", ";
        if (checked.empty())
            os << "false";
        else
            os << "&" << checked.to_arg();
        os << ")";
        //ImGui::Shortcut is exported in ExportShortcut pass

        if (ifstmt) {
            os << ")\n";
            ctx.ind_up();
            os << ctx.ind << onChange.to_arg() << "();\n";
            ctx.ind_down();
        }
        else
            os << ";\n";
    }
}

//As of now ImGui still doesn't process shortcuts in unopened menus so
//separate Shortcut pass is needed
void MenuIt::ExportShortcut(std::ostream& os, UIContext& ctx)
{
    if (shortcut.empty() || ownerDraw)
        return;

    os << ctx.ind << "if (";
    if (!disabled.has_value() || disabled.value())
        os << "!(" << disabled.to_arg() << ") && ";
    //force RouteGlobal otherwise it won't get fired when menu popup is open
    os << "ImGui::Shortcut(" << shortcut.to_arg() << "))\n";
    ctx.ind_up();
    if (!onChange.empty())
        os << ctx.ind << onChange.to_arg() << "();\n";
    else
        os << ctx.ind << ";\n";
    ctx.ind_down();
}

void MenuIt::ExportAllShortcuts(std::ostream& os, UIContext& ctx)
{
    ExportShortcut(os, ctx);
    
    for (const auto& child : children) 
    {
        auto* it = dynamic_cast<MenuIt*>(child.get());
        if (it)    it->ExportAllShortcuts(os, ctx);
    }
}

void MenuIt::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if ((sit->kind == cpp::CallExpr && sit->callee == "ImGui::MenuItem") ||
        (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::MenuItem"))
    {
        if (sit->params.size())
            label.set_from_arg(sit->params[0]);
        if (sit->params.size() >= 2 && cpp::is_cstr(sit->params[1]))
            *shortcut.access() = sit->params[1].substr(1, sit->params[1].size() - 2);
        if (sit->params.size() >= 3 && !sit->params[2].compare(0, 1, "&"))
            checked.set_from_arg(sit->params[2].substr(1));

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginPopup")
    {
        contextMenu = true;
        if (sit->params.size())
            label.set_from_arg(sit->params[0]);
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginMenu")
    {
        if (sit->params.size())
            label.set_from_arg(sit->params[0]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Separator")
    {
        separator = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
    {
        if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_WindowPadding")
            style_padding.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ItemSpacing")
            style_spacing.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_PopupRounding")
            style_rounding.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee.compare(0, 7, "ImGui::"))
    {
        ownerDraw = true;
        onChange.set_from_arg(sit->callee);
    }
}

std::vector<UINode::Prop>
MenuIt::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.padding", &style_padding },
        { "appearance.spacing", &style_spacing },
        { "appearance.rounding", &style_rounding },
        { "appearance.ownerDraw", &ownerDraw },
        { "behavior.label", &label, true },
        { "behavior.shortcut", &shortcut },
        { "behavior.separator", &separator },
        { "bindings.checked##1", &checked },
        });
    return props;
}

bool MenuIt::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::BeginDisabled(!contextMenu);
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_padding, ctx);
        ImGui::EndDisabled();
        break;
    case 1:
        ImGui::BeginDisabled(!contextMenu);
        ImGui::Text("spacing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_spacing, ctx);
        ImGui::EndDisabled();
        break;
    case 2:
        ImGui::BeginDisabled(!contextMenu);
        ImGui::Text("rounding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_rounding, ctx);
        ImGui::EndDisabled();
        break;
    case 3:
        ImGui::BeginDisabled(contextMenu);
        ImGui::Text("ownerDraw");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = ImGui::Checkbox("##ownerDraw", ownerDraw.access());
        ImGui::EndDisabled();
        break;
    case 4:
        ImGui::BeginDisabled(ownerDraw);
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&label, InputDirectVal_Modified, ctx);
        ImGui::EndDisabled();
        break;
    case 5:
        ImGui::BeginDisabled(ownerDraw || contextMenu || children.size());
        ImGui::Text("shortcut");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = shortcut.empty() ? 0 : InputDirectVal_Modified;
        changed = InputDirectVal(&shortcut, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 6:
        ImGui::BeginDisabled(contextMenu);
        ImGui::Text("separator");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = ImGui::Checkbox("##separator", separator.access());
        ImGui::EndDisabled();
        break;
    case 7:
        //ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_REF_CLR);
        ImGui::BeginDisabled(ownerDraw || contextMenu || children.size());
        ImGui::Text("checked");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&checked, true, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::PropertyUI(i - 8, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
MenuIt::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "menuIt.change", &onChange },
        });
    return props;
}

bool MenuIt::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
    {
        ImGui::BeginDisabled(children.size());
        std::string name = ownerDraw ? "Draw" : "Change";
        ImGui::Text(("On" + name).c_str());
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_" + name, &onChange, 0, ctx);
        ImGui::EndDisabled();
        break;
    }
    default:
        ImGui::BeginDisabled(contextMenu);
        changed = Widget::EventUI(i - 1, ctx);
        ImGui::EndDisabled();
        break;
    }
    return changed;
}
