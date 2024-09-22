#include "node2.h"
#include "stx.h"
#include "imrad.h"
#include "cppgen.h"
#include "binding_input.h"
#include "binding_field.h"
#include "ui_table_columns.h"
#include "ui_message_box.h"
#include <algorithm>
#include <array>


template <class F>
void TreeNodeProp(const char* name, const std::string& label, F&& f)
{
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    ImGui::Unindent();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, pad.y });
    if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_SpanAvailWidth)) {
        ImGui::PopStyleVar();
        ImGui::Indent();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { pad.x, 0 }); //row packing
        f();
        ImGui::PopStyleVar();
        ImGui::TreePop();
        ImGui::Unindent();
    }
    else
    {
        ImGui::PopStyleVar();
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        ImRad::Selectable(label.c_str(), false, ImGuiSelectableFlags_Disabled, { -ImGui::GetFrameHeight(), 0 });
    }
    ImGui::Indent();
}

bool DataLoopProp(const char* name, data_loop* val, UIContext& ctx)
{
    bool changed = false;
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_REF_CLR);
    ImGui::Unindent();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, pad.y });
    bool open = ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_SpanAvailWidth);
    ImGui::PopStyleVar();
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
    changed |= InputDataSize("##size", &val->limit, true, ctx);
    //changed = ImGui::InputText("##size", val->limit.access()); //allow empty
    ImGui::SameLine(0, 0);
    changed |= BindingButton(name, &val->limit, ctx);
    if (open)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_REF_CLR);
        ImGui::Indent();
        ImGui::AlignTextToFramePadding();
        ImGui::BeginDisabled(val->empty());
        ImGui::Text("index");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed |= InputFieldRef("##index", &val->index, true, ctx);
        ImGui::EndDisabled();
        ImGui::TreePop();
    }
    else
    {
        ImGui::Indent();
    }
    return changed;
}

//----------------------------------------------------------------------------

TopWindow::TopWindow(UIContext& ctx)
    : kind(Kind(ctx.kind))
{
    if (kind == Activity) {
        title = "MyActivity";
        size_x = 400;
        size_y = 700;
    }
    else {
        size_x = 640;
        size_y = 480;
    }

    flags.prefix("ImGuiWindowFlags_");
    flags.add$(ImGuiWindowFlags_NoTitleBar);
    flags.add$(ImGuiWindowFlags_NoResize);
    flags.add$(ImGuiWindowFlags_NoMove);
    flags.add$(ImGuiWindowFlags_NoScrollbar);
    flags.add$(ImGuiWindowFlags_NoScrollWithMouse);
    flags.add$(ImGuiWindowFlags_NoCollapse);
    flags.add$(ImGuiWindowFlags_AlwaysAutoResize);
    flags.add$(ImGuiWindowFlags_NoBackground);
    flags.add$(ImGuiWindowFlags_NoSavedSettings);
    flags.add$(ImGuiWindowFlags_MenuBar);
    flags.add$(ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    flags.add$(ImGuiWindowFlags_AlwaysVerticalScrollbar);
    flags.add$(ImGuiWindowFlags_NoNavInputs);
    flags.add$(ImGuiWindowFlags_NoNavFocus);
    flags.add$(ImGuiWindowFlags_NoDocking);

    placement.add(" ", None);
    placement.add$(Left);
    placement.add$(Right);
    placement.add$(Top);
    placement.add$(Bottom);
    placement.add$(Center);
    placement.add$(Maximize);
}

void TopWindow::Draw(UIContext& ctx)
{
    ctx.unit = ctx.unit == "px" ? "" : ctx.unit;
    ctx.root = this;
    ctx.isAutoSize = flags & ImGuiWindowFlags_AlwaysAutoResize;
    ctx.prevLayoutHash = ctx.layoutHash;
    ctx.layoutHash = ctx.isAutoSize;
    bool dimAll = ctx.activePopups.size();
    ctx.activePopups.clear();
    ctx.parents = { this };
    ctx.hovered = nullptr;
    ctx.snapParent = nullptr;
    ctx.kind = kind;
    ctx.contextMenus.clear();
    
    std::string cap = title.value();
    cap += "###TopWindow" + std::to_string((size_t)this); //don't clash 
    int fl = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;
    if (ctx.mode != UIContext::NormalSelection)
        fl |= ImGuiWindowFlags_NoResize; //so that window resizing doesn't interfere with snap
    if (kind == MainWindow)
        fl |= ImGuiWindowFlags_NoCollapse;
    else if (kind == Popup)
        fl |= ImGuiWindowFlags_NoTitleBar;
    else if (kind == Activity) //only allow resizing here to test layout behavior
        fl |= ImGuiWindowFlags_NoTitleBar /*| ImGuiWindowFlags_NoResize*/ | ImGuiWindowFlags_NoCollapse;
    fl |= flags;

    if (style_font.has_value())
        ImGui::PushFont(ImRad::GetFontByName(style_font.eval(ctx)));
    if (style_padding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style_padding.eval_px(ctx));
    if (style_spacing.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style_spacing.eval_px(ctx));
    if (style_borderSize.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, style_borderSize.eval_px(ctx));
    if (style_rounding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, style_rounding.eval_px(ctx));
    if (style_scrollbarSize.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, style_scrollbarSize.eval_px(ctx));
    if (!style_bg.empty())
        ImGui::PushStyleColor(ImGuiCol_WindowBg, style_bg.eval(ImGuiCol_WindowBg, ctx));
    if (!style_menuBg.empty())
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, style_menuBg.eval(ImGuiCol_MenuBarBg, ctx));

    ImGui::SetNextWindowScroll({ 0, 0 }); //click on a child causes scrolling which doesn't go back
    ImGui::SetNextWindowPos(ctx.designAreaMin + ImVec2{ 20, 20 }); // , ImGuiCond_Always, { 0.5, 0.5 });
    
    if (kind == MainWindow && placement == Maximize) 
    {
        ImGui::SetNextWindowSize(ctx.designAreaMax - ctx.designAreaMin - ImVec2{ 40, 40 });
    }
    else if (fl & ImGuiWindowFlags_AlwaysAutoResize)
    {
        //prevent autosized window to collapse 
        ImGui::SetNextWindowSizeConstraints({ 30, 30 }, { FLT_MAX, FLT_MAX });
    }
    else
    {
        float w = size_x.eval_px(ImGuiAxis_X, ctx);
        if (!w)
            w = 640;
        float h = size_y.eval_px(ImGuiAxis_Y, ctx);
        if (!h)
            h = 480;
        ImGui::SetNextWindowSize({ w, h });
    }
    
    if (style_titlePadding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, style_titlePadding.eval_px(ctx));
    
    bool tmp;
    ImGui::Begin(cap.c_str(), &tmp, fl);
    
    if (style_titlePadding.has_value())
        ImGui::PopStyleVar();
    
    ImGui::SetWindowFontScale(ctx.zoomFactor);

    ctx.rootWin = ImGui::FindWindowByName(cap.c_str());
    assert(ctx.rootWin);
    for (int i = 0; i < 4; ++i) 
    {
        if (ImGui::GetActiveID() == ImGui::GetWindowResizeCornerID(ctx.rootWin, i) ||
            ImGui::GetActiveID() == ImGui::GetWindowResizeBorderID(ctx.rootWin, (ImGuiDir)i))
            ctx.beingResized = true;
    }
    if (ctx.beingResized && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        ctx.beingResized = false;
        ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event so random widget won't get selected
    }
    if (ctx.beingResized)
        ctx.selected = { this }; //reset selection so there is no visual noise

    if (placement == Left)
        ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersLeft);
    else if (placement == Right)
        ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersRight);
    else if (placement == Top)
        ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersTop);
    else if (placement == Bottom)
        ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersBottom);

    ImGui::GetCurrentContext()->NavDisableMouseHover = true;
    ImGui::GetCurrentContext()->NavDisableHighlight = true;
    if (dimAll)
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);

    for (size_t i = 0; i < children.size(); ++i)
        children[i]->Draw(ctx);
    
    if (dimAll)
        ImGui::PopStyleVar();
    ImGui::GetCurrentContext()->NavDisableHighlight = false;
    ImGui::GetCurrentContext()->NavDisableMouseHover = false;

    for (size_t i = 0; i < children.size(); ++i)
        children[i]->DrawExtra(ctx);
    
    //use all client area to allow snapping close to the border
    auto pad = ImGui::GetStyle().WindowPadding - ImVec2(3+1, 3);
    auto mi = ImGui::GetWindowContentRegionMin() - pad;
    auto ma = ImGui::GetWindowContentRegionMax() + pad;
    cached_pos = ImGui::GetWindowPos() + mi;
    cached_size = ma - mi;

    if (!ImGui::GetTopMostAndVisiblePopupModal() && ctx.activePopups.size() &&
        ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        ctx.selected = { ctx.parents[0] };
    }
    bool allowed = !ImGui::GetTopMostAndVisiblePopupModal() && ctx.activePopups.empty();
    if (allowed && ctx.mode == UIContext::NormalSelection)
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            ctx.selStart = ctx.selEnd = ImGui::GetMousePos();
        }
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            Norm(ImGui::GetMousePos() - ctx.selStart) > 5)
        {
            ctx.mode = UIContext::RectSelection;
            ctx.selEnd = ImGui::GetMousePos();
        }
    }
    if (ctx.mode == UIContext::NormalSelection &&
        ImGui::IsWindowHovered() && //includes !GetTopMostVisibleModal
        ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
            ; //don't participate in group selection
        else
            ctx.selected = { this };
        ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event
    }
    if (ctx.mode == UIContext::RectSelection)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            ctx.selEnd = ImGui::GetMousePos();
        else {
            ImVec2 a{ std::min(ctx.selStart.x, ctx.selEnd.x), std::min(ctx.selStart.y, ctx.selEnd.y) };
            ImVec2 b{ std::max(ctx.selStart.x, ctx.selEnd.x), std::max(ctx.selStart.y, ctx.selEnd.y) };
            auto sel = FindInRect(ImRect(a, b));
            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
                for (auto s : sel)
                    if (!stx::count(ctx.selected, s))
                        ctx.selected.push_back(s);
            }
            else
                ctx.selected = sel;
            ctx.mode = UIContext::NormalSelection; //todo
        }
    }
    if (allowed && ctx.mode == UIContext::PickPoint &&
        !ctx.snapParent &&
        ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
    {
        ctx.snapParent = this;
        ctx.snapIndex = children.size();
    }
    
    if (ctx.mode == UIContext::Snap && !ctx.snapParent && ImGui::IsWindowHovered())
    {
        DrawSnap(ctx);
    }
    if (ctx.mode == UIContext::PickPoint && ctx.snapParent == this)
    {
        DrawInteriorRect(ctx);
    }
    if (ctx.mode == UIContext::RectSelection)
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 a{ std::min(ctx.selStart.x, ctx.selEnd.x), std::min(ctx.selStart.y, ctx.selEnd.y) };
        ImVec2 b{ std::max(ctx.selStart.x, ctx.selEnd.x), std::max(ctx.selStart.y, ctx.selEnd.y) };
        dl->PushClipRect(ctx.designAreaMin, ctx.designAreaMax);
        dl->AddRectFilled(a, b, ctx.colors[UIContext::Hovered]);
        dl->PopClipRect();
    }

    ImGui::End();

    if (!style_bg.empty())
        ImGui::PopStyleColor();
    if (!style_menuBg.empty())
        ImGui::PopStyleColor();
    if (style_scrollbarSize.has_value())
        ImGui::PopStyleVar();
    if (style_rounding.has_value())
        ImGui::PopStyleVar();
    if (style_borderSize.has_value())
        ImGui::PopStyleVar();
    if (style_spacing.has_value())
        ImGui::PopStyleVar();
    if (style_padding.has_value())
        ImGui::PopStyleVar();
    if (style_font.has_value())
        ImGui::PopFont();
}

void TopWindow::Export(std::ostream& os, UIContext& ctx)
{
    ctx.varCounter = 1;
    ctx.parents = { this };
    ctx.kind = kind;
    ctx.errors.clear();
    ctx.unit = ctx.unit == "px" ? "" : ctx.unit;
    
    ctx.codeGen->RemovePrefixedVars(std::string(ctx.codeGen->HBOX_NAME));
    ctx.codeGen->RemovePrefixedVars(std::string(ctx.codeGen->VBOX_NAME));

    //todo: put before ///@ params
    if (userCodeBefore != "")
        os << userCodeBefore << "\n";

    //provide stable ID when title changes
    bindable<std::string> titleId = title;
    *titleId.access() += "###" + ctx.codeGen->GetName();
    std::string tit = titleId.to_arg();
    bool hasMB = children.size() && dynamic_cast<MenuBar*>(children[0].get());
    bool autoSize = flags & ImGuiWindowFlags_AlwaysAutoResize;

    os << ctx.ind << "/// @begin TopWindow\n";
    os << ctx.ind << "auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;\n";

    if (ctx.unit == "dp")
    {
        os << ctx.ind << "const float dp = ioUserData->dpiScale;\n";
    }
    
    if (kind == Popup || kind == ModalPopup)
    {
        os << ctx.ind << "ID = ImGui::GetID(\"###" << ctx.codeGen->GetName() << "\");\n";
    }
    else if (kind == Activity)
    {
        if (initialActivity) 
        {
            os << ctx.ind << "if (ioUserData->activeActivity == \"\")\n";
            ctx.ind_up();
            os << ctx.ind << "Open();\n";
            ctx.ind_down();
        }
        os << ctx.ind << "if (ioUserData->activeActivity != \"" << ctx.codeGen->GetName() << "\")\n";
        ctx.ind_up();
        os << ctx.ind << "return;\n";
        ctx.ind_down();
    }

    if (!style_font.empty())
    {
        os << ctx.ind << "ImGui::PushFont(" << style_font.to_arg() << ");\n";
    }
    if (!style_bg.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(" <<
            ((kind == Popup || kind == ModalPopup) ? "ImGuiCol_PopupBg" : "ImGuiCol_WindowBg") <<
            ", " << style_bg.to_arg() << ");\n";
    }
    if (!style_menuBg.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_MenuBarBg, " << style_menuBg.to_arg() << ");\n";
    }
    if (style_padding.has_value())
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, "
            << style_padding.to_arg(ctx.unit) << ");\n";
    }
    if (style_spacing.has_value())
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, "
            << style_spacing.to_arg(ctx.unit) << ");\n";
    }
    if (style_rounding.has_value())
    {
        os << ctx.ind << "ImGui::PushStyleVar(";
        //ModalPopup uses WindowRounding
        os << (kind == Popup ? "ImGuiStyleVar_PopupRounding" : "ImGuiStyleVar_WindowRounding");
        os << ", " << style_rounding.to_arg(ctx.unit) << ");\n";
    }
    if (style_borderSize.has_value() && kind != Activity && kind != MainWindow)
    {
        os << ctx.ind << "ImGui::PushStyleVar(";
        os << ((kind == Popup || kind == ModalPopup) ? "ImGuiStyleVar_PopupBorderSize" : "ImGuiStyleVar_WindowBorderSize");
        os << ", " << style_borderSize.to_arg(ctx.unit) << ");\n";
    }
    if (style_scrollbarSize.has_value())
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, "
            << style_scrollbarSize.to_arg(ctx.unit) << ");\n";
    }

    if (kind == MainWindow)
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);\n";
        os << ctx.ind << "glfwSetWindowTitle(window, " << title.to_arg() << ");\n";
        os << ctx.ind << "ImGui::SetNextWindowPos({ 0, 0 });\n";
        if (!autoSize) 
        {
            os << ctx.ind << "int tmpWidth, tmpHeight;\n";
            os << ctx.ind << "glfwGetWindowSize(window, &tmpWidth, &tmpHeight);\n";
            os << ctx.ind << "ImGui::SetNextWindowSize({ (float)tmpWidth, (float)tmpHeight });\n";
        }
        
        flags_helper fl = flags;
        fl |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings;
        os << ctx.ind << "bool tmpOpen;\n";
        os << ctx.ind << "if (ImGui::Begin(\"###" << ctx.codeGen->GetName() << "\", &tmpOpen, " << fl.to_arg() << "))\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();
        
        if (autoSize)
        {
            os << ctx.ind << "if (ImGui::IsWindowAppearing())\n" << ctx.ind << "{\n";
            ctx.ind_up();
            //to prevent edge at right/bottom border. Not sure why ImGui needs it
            os << ctx.ind << "ImGui::GetStyle().DisplaySafeAreaPadding = { 0, 0 };\n";
            os << ctx.ind << "glfwSetWindowAttrib(window, GLFW_RESIZABLE, false);\n";
            os << ctx.ind << "glfwSetWindowAttrib(window, GLFW_DECORATED, "
                << std::boolalpha << !(flags & ImGuiWindowFlags_NoTitleBar) << ");\n";
            ctx.ind_down();
            os << ctx.ind << "}\n";
            //glfwSetWindowSize only when SizeFull is determined
            //alternatively calculate it from ContentSize + padding
            os << ctx.ind << "else\n" << ctx.ind << "{\n";
            ctx.ind_up();
            os << ctx.ind << "ImVec2 size = ImGui::GetCurrentWindow()->SizeFull;\n";
            os << ctx.ind << "glfwSetWindowSize(window, (int)size.x, (int)size.y);\n";
            ctx.ind_down();
            os << ctx.ind << "}\n";
        }
        else
        {
            os << ctx.ind << "if (ImGui::IsWindowAppearing())\n" << ctx.ind << "{\n";
            ctx.ind_up();
            if (placement == Maximize)
                os << ctx.ind << "glfwMaximizeWindow(window);\n";
            else
                os << ctx.ind << "glfwSetWindowSize(window, " << size_x.to_arg(ctx.unit)
                << ", " << size_y.to_arg(ctx.unit) << ");\n";
            
            os << ctx.ind << "glfwSetWindowAttrib(window, GLFW_RESIZABLE, "
                << std::boolalpha << !(flags & ImGuiWindowFlags_NoResize) << ");\n";
            os << ctx.ind << "glfwSetWindowAttrib(window, GLFW_DECORATED, "
                << std::boolalpha << !(flags & ImGuiWindowFlags_NoTitleBar) << ");\n";
            ctx.ind_down();
            os << ctx.ind << "}\n";
        }
    }
    else if (kind == Activity)
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);\n";
        os << ctx.ind << "ImGui::SetNextWindowPos(ioUserData->WorkRect().Min);\n";
        os << ctx.ind << "ImGui::SetNextWindowSize(ioUserData->WorkRect().GetSize());";
        //signal designed size
        os << " //{ " << size_x.to_arg(ctx.unit) << ", " << size_y.to_arg(ctx.unit) << " }\n";
        flags_helper fl = flags;
        fl |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        os << ctx.ind << "bool tmpOpen;\n";
        os << ctx.ind << "if (ImGui::Begin(\"###" << ctx.codeGen->GetName() << "\", &tmpOpen, " << fl.to_arg() << "))\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();

        if (!onBackButton.empty()) {
            os << ctx.ind << "if (ImGui::IsKeyPressed(ImGuiKey_AppBack))\n";
            ctx.ind_up();
            os << ctx.ind << onBackButton.to_arg() << "();\n";
            ctx.ind_down();
        }
    }
    else if (kind == Window)
    {
        //pos
        if (placement == Left || placement == Top)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos(ioUserData->WorkRect().Min);";
            os << (placement == Left ? " //Left\n" : " //Top\n");
        }
        else if (placement == Right || placement == Bottom)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos(ioUserData->WorkRect().Max, 0, { 1, 1 });";
            os << (placement == Right ? " //Right\n" : " //Bottom\n");
        }

        //size
        os << ctx.ind << "ImGui::SetNextWindowSize({ ";

        if (placement == Top || placement == Bottom)
            os << "ioUserData->WorkRect().GetWidth()";
        else if (autoSize)
            os << "0";
        else
            os << size_x.to_arg(ctx.unit);

        os << ", ";

        if (placement == Left || placement == Right)
            os << "ioUserData->WorkRect().GetHeight()";
        else if (autoSize)
            os << "0";
        else
            os << size_y.to_arg(ctx.unit);

        os << " }";
        if (!autoSize && placement != Left && placement != Right && placement != Top && placement != Bottom)
            os << ", ImGuiCond_FirstUseEver";
        os << ");";
        //signal designed size
        os << " //{ " << size_x.to_arg(ctx.unit) << ", " << size_y.to_arg(ctx.unit) << " }\n";
        
        if (style_titlePadding.has_value())
        {
            os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, " <<
                style_titlePadding.to_arg(ctx.unit) << ");\n";
        }
        os << ctx.ind << "if (isOpen && ImGui::Begin(" << tit << ", &isOpen, " << flags.to_arg() << "))\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();
        if (style_titlePadding.has_value())
        {
            os << ctx.ind << "ImGui::PopStyleVar();\n";
            os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding);\n";
        }
    }
    else if (kind == Popup || kind == ModalPopup)
    {
        if (placement == Left || placement == Top)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos(";
            if (animate)
                os << "{ ioUserData->WorkRect().Min.x + animPos.x, ioUserData->WorkRect().Min.y + animPos.y }";
            else
                os << "ioUserData->WorkRect().Min";
            os << "); " << (placement == Left ? " //Left\n" : " //Top\n");
        }
        else if (placement == Right || placement == Bottom)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos(";
            if (animate)
                os << "{ ioUserData->WorkRect().Max.x - animPos.x, ioUserData->WorkRect().Max.y - animPos.y }, 0, { 1, 1 }";
            else
                os << "ioUserData->WorkRect().Max, 0, { 1, 1 }";
            os << "); " << (placement == Right ? " //Right\n" : " //Bottom\n");
        }
        else if (placement == Center)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos(";
            if (animate)
                os << "{ ioUserData->WorkRect().GetCenter().x, ioUserData->WorkRect().GetCenter().y + animPos.y }, 0, { 0.5f, 0.5f }";
            else
                os << "ioUserData->WorkRect().GetCenter(), 0, { 0.5f, 0.5f }";
            os << "); //Center\n";
        }

        //size
        os << ctx.ind << "ImGui::SetNextWindowSize({ ";

        if (placement == Top || placement == Bottom)
            os << "ioUserData->WorkRect().GetWidth()";
        else if (autoSize)
            os << "0";
        else
            os << size_x.to_arg(ctx.unit);

        os << ", ";

        if (placement == Left || placement == Right)
            os << "ioUserData->WorkRect().GetHeight()";
        else if (autoSize)
            os << "0";
        else
            os << size_y.to_arg(ctx.unit);

        os << " }";
        if (!autoSize && placement != Left && placement != Right && placement != Top && placement != Bottom)
            os << ", ImGuiCond_FirstUseEver";
        os << ");";
        //signal designed size
        os << " //{ " << size_x.to_arg(ctx.unit) << ", " << size_y.to_arg(ctx.unit) << " }\n";

        //begin
        if (kind == ModalPopup)
        {
            if (style_titlePadding.has_value())
            {
                os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, " <<
                    style_titlePadding.to_arg(ctx.unit) << ");\n";
            }
            os << ctx.ind << "bool tmpOpen = true;\n";
            os << ctx.ind << "if (ImGui::BeginPopupModal(" << tit << ", &tmpOpen, " << flags.to_arg() << "))\n";
            os << ctx.ind << "{\n";
            ctx.ind_up();
            if (style_titlePadding.has_value())
            {
                os << ctx.ind << "ImGui::PopStyleVar();\n";
                os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding);\n";
            }
        }
        else
        {
            os << ctx.ind << "if (ImGui::BeginPopup(" << tit << ", " << flags.to_arg() << "))\n";
            os << ctx.ind << "{\n";
            ctx.ind_up();
        }
        
        //animation
        if (animate)
        {
            os << ctx.ind << "animator.Tick();\n";
            if (placement == Left || placement == Right || placement == Top || placement == Bottom)
            {
                os << ctx.ind << "if (!ImRad::MoveWhenDragging(";
                if (placement == Left)
                    os << "ImGuiDir_Left";
                else if (placement == Right)
                    os << "ImGuiDir_Right";
                else if (placement == Top)
                    os << "ImGuiDir_Up";
                else if (placement == Bottom)
                    os << "ImGuiDir_Down";
                os << ", animPos, ioUserData->dimBgRatio))\n";
                ctx.ind_up();
                os << ctx.ind << "ClosePopup();\n";
                ctx.ind_down();
            }
        }

        //back button
        if (!onBackButton.empty())
        {
            os << ctx.ind << "if (ImGui::IsKeyPressed(ImGuiKey_AppBack))\n";
            ctx.ind_up();
            os << ctx.ind << onBackButton.to_arg() << "();\n";
            ctx.ind_down();
        }

        //rendering
        if (kind == ModalPopup)
        {
            os << ctx.ind << "if (ioUserData->activeActivity != \"\")\n";
            ctx.ind_up();
            os << ctx.ind << "ImRad::RenderDimmedBackground(ioUserData->WorkRect(), ioUserData->dimBgRatio);\n";
            ctx.ind_down();
        }
        if (style_rounding &&
            (placement == Left || placement == Right || placement == Top || placement == Bottom))
        {
            os << ctx.ind << "ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCorners";
            if (placement == Left)
                os << "Left";
            else if (placement == Right)
                os << "Right";
            else if (placement == Top)
                os << "Top";
            else if (placement == Bottom)
                os << "Bottom";
            os << ");\n";
        }

        //CloseCurrentPopup
        os << ctx.ind << "if (modalResult != ImRad::None";
        if (animate)
            os << " && animator.IsDone()";
        os << ")\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();
        os << ctx.ind << "ImGui::CloseCurrentPopup();\n";
        //callback is called after CloseCurrentPopup so it is able to open another dialog
        //without calling Draw from it
        if (kind == ModalPopup)
        {
            os << ctx.ind << "if (modalResult != ImRad::Cancel)\n";
            ctx.ind_up();
            os << ctx.ind << "callback(modalResult);\n";
            ctx.ind_down();
        }
        ctx.ind_down();
        os << ctx.ind << "}\n";

        if (closeOnEscape)
        {
            os << ctx.ind << "if (ImGui::Shortcut(ImGuiKey_Escape))\n";
            ctx.ind_up();
            os << ctx.ind << "ClosePopup();\n";
            ctx.ind_down();
        }
    }
    
    if (!onWindowAppearing.empty())
    {
        os << ctx.ind << "if (ImGui::IsWindowAppearing())\n";
        ctx.ind_up();
        os << ctx.ind << onWindowAppearing.to_arg() << "();\n";
        ctx.ind_down();
    }

    os << ctx.ind << "/// @separator\n\n";
    
    //at next import comment becomes userCode
    /*if (userCodeMid != "")
        os << userCodeMid << "\n";*/
    if (children.empty() || children[0]->userCodeBefore.empty())
        os << ctx.ind << "// TODO: Add Draw calls of dependent popup windows here\n\n";

    for (const auto& ch : children)
        ch->Export(os, ctx);
        
    os << ctx.ind << "/// @separator\n";
    
    if (kind == Popup || kind == ModalPopup)
    {
        os << ctx.ind << "ImGui::EndPopup();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    else
    {
        os << ctx.ind << "ImGui::End();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }

    if (style_titlePadding.has_value() && (kind == Window || kind == ModalPopup))
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_scrollbarSize.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_borderSize.has_value() || kind == Activity || kind == MainWindow)
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_rounding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_spacing.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_padding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_menuBg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_font.empty())
        os << ctx.ind << "ImGui::PopFont();\n";

    os << ctx.ind << "/// @end TopWindow\n";

    if (userCodeAfter != "")
        os << userCodeAfter << "\n";
}

void TopWindow::Import(cpp::stmt_iterator& sit, UIContext& ctx)
{
    ctx.errors.clear();
    bool tmpCreateDeps = ctx.createVars;
    ctx.createVars = false;
    ctx.importState = 2; //1 - inside TopWindow block, 2 - before TopWindow or outside @begin/end/separator, 3 - after @end TopWindow
    ctx.userCode = "";
    ctx.root = this;
    ctx.parents = { this };
    ctx.kind = kind = Window;
    bool hasGlfw = false;
    bool windowAppearingBlock = false;
    
    while (sit != cpp::stmt_iterator())
    {
        bool parseCommentSize = false;
        bool parseCommentPos = false;
        
        if (sit->kind == cpp::Comment && !sit->line.compare(0, 20, "/// @begin TopWindow"))
        {
            ctx.importState = 1;
            sit.enable_parsing(true);
            userCodeBefore = ctx.userCode;
            ctx.userCode = "";
        }
        else if (sit->kind == cpp::Comment && !sit->line.compare(0, 11, "/// @begin "))
        {
            ctx.importState = 1;
            sit.enable_parsing(true);
            std::string type = sit->line.substr(11);
            if (auto node = Widget::Create(type, ctx))
            {
                children.push_back(std::move(node));
                children.back()->Import(++sit, ctx); //after insertion
                ctx.importState = 2;
                ctx.userCode = "";
                sit.enable_parsing(false);
            }
        }
        else if (sit->kind == cpp::Comment && !sit->line.compare(0, 18, "/// @end TopWindow"))
        {
            ctx.importState = 3;
            ctx.userCode = "";
            sit.enable_parsing(false);
        }
        else if (sit->kind == cpp::Comment && !sit->line.compare(0, 14, "/// @separator"))
        {
            if (ctx.importState == 1) { //separator at begin
                ctx.importState = 2;
                ctx.userCode = "";
                sit.enable_parsing(false);
            }
            else { //separator at end
                if (!children.empty())
                    children.back()->userCodeAfter = ctx.userCode;
                else
                    userCodeMid = ctx.userCode;
                ctx.userCode = "";
                ctx.importState = 1;
                sit.enable_parsing(true);
            }
        }
        else if ((ctx.importState == 2 || ctx.importState == 3) && 
                (sit->kind != cpp::Comment || sit->line.compare(0, 5, "/// @")))
        {
            if (ctx.importState == 3 && sit->line.size() && sit->line.back() == '}') { //reached end of Draw
                sit.base().stream().putback('}');
                sit.enable_parsing(true);
                break;
            }
            if (ctx.userCode != "")
                ctx.userCode += "\n";
            ctx.userCode += sit->line;
        }
        else if (sit->kind == cpp::IfStmt && sit->cond == "ioUserData->activeActivity==\"\"")
        {
            initialActivity = true;
        }
        else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::IsWindowAppearing")
        {
            windowAppearingBlock = true;
        }
        else if (sit->kind == cpp::Other && sit->line == "else" && 
            sit->level == ctx.importLevel + 1)
        {
            windowAppearingBlock = false;
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushFont")
        {
            if (sit->params.size())
                style_font.set_from_arg(sit->params[0]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
        {
            if (sit->params[0] == "ImGuiCol_WindowBg" || sit->params[0] == "ImGuiCol_PopupBg")
                style_bg.set_from_arg(sit->params[1]);
            else if (sit->params[0] == "ImGuiCol_MenuBarBg")
                style_menuBg.set_from_arg(sit->params[1]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar" && sit->params.size() == 2)
        {
            if (sit->params[0] == "ImGuiStyleVar_WindowPadding")
                style_padding.set_from_arg(sit->params[1]);
            else if (sit->params[0] == "ImGuiStyleVar_ItemSpacing")
                style_spacing.set_from_arg(sit->params[1]);
            else if (sit->params[0] == "ImGuiStyleVar_WindowRounding" || sit->params[0] == "ImGuiStyleVar_PopupRounding")
                style_rounding.set_from_arg(sit->params[1]);
            else if (sit->params[0] == "ImGuiStyleVar_WindowBorderSize" || sit->params[0] == "ImGuiStyleVar_PopupBorderSize")
                style_borderSize.set_from_arg(sit->params[1]);
            else if (sit->params[0] == "ImGuiStyleVar_ScrollbarSize")
                style_scrollbarSize.set_from_arg(sit->params[1]);
            else if (sit->params[0] == "ImGuiStyleVar_FramePadding" && style_titlePadding.empty())
                style_titlePadding.set_from_arg(sit->params[1]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextWindowPos")
        {
            parseCommentPos = true;
            animate = sit->line.find("animPos") != std::string::npos;
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextWindowSize")
        {
            parseCommentSize = true;
            if (sit->params.size())
            {
                auto size = cpp::parse_size(sit->params[0]);
                size_x.set_from_arg(size.first);
                size_y.set_from_arg(size.second);
            }
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "glfwSetWindowTitle")
        {
            hasGlfw = true;
            if (sit->params.size() >= 2)
                title.set_from_arg(sit->params[1]);
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "glfwSetWindowSize" && 
            windowAppearingBlock)
        {
            hasGlfw = true;
            if (sit->params.size() >= 3) {
                size_x.set_from_arg(sit->params[1]);
                size_y.set_from_arg(sit->params[2]);
            }
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "glfwMaximizeWindow")
        {
            hasGlfw = true;
            placement = Maximize;
        }
        else if (sit->kind == cpp::CallExpr && sit->callee == "glfwSetWindowAttrib"
            && sit->params.size() >= 3)
        {
            hasGlfw = true;
            if (sit->params[1] == "GLFW_RESIZABLE") {
                bool resizable = sit->params[2] != "false";
                if (!resizable && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
                    flags |= ImGuiWindowFlags_NoResize;
            }
            else if (sit->params[1] == "GLFW_DECORATED") {
                bool decorated = sit->params[2] != "false";
                if (!decorated)
                    flags |= ImGuiWindowFlags_NoTitleBar;
            }
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::IsKeyPressed(ImGuiKey_AppBack)")
        {
            onBackButton.set_from_arg(sit->callee2);
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->cond == "ImGui::Shortcut(ImGuiKey_Escape)" &&
            sit->callee2 == "ClosePopup")
        {
            closeOnEscape = true;
        }
        else if (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::IsWindowAppearing")
        {
            if (sit->params.empty() && sit->params2.empty())
                onWindowAppearing.set_from_arg(sit->callee2);
        }
        else if ((sit->kind == cpp::IfCallBlock || sit->kind == cpp::CallExpr) &&
            sit->callee == "ImGui::BeginPopupModal")
        {
            ctx.kind = kind = ModalPopup;
            title.set_from_arg(sit->params[0]);
            size_t i = title.access()->rfind("###");
            if (i != std::string::npos)
                title.access()->resize(i);

            if (sit->params.size() >= 3)
                flags.set_from_arg(sit->params[2]);
        }
        else if ((sit->kind == cpp::IfCallBlock || sit->kind == cpp::CallExpr) &&
            sit->callee == "ImGui::BeginPopup")
        {
            ctx.kind = kind = Popup;
            title.set_from_arg(sit->params[0]);
            size_t i = title.access()->rfind("###");
            if (i != std::string::npos)
                title.access()->resize(i);

            if (sit->params.size() >= 2)
                flags.set_from_arg(sit->params[1]);
        }
        else if (sit->kind == cpp::IfCallBlock && sit->callee == "isOpen&&ImGui::Begin")
        {
            ctx.kind = kind = Window;
            title.set_from_arg(sit->params[0]);
            size_t i = title.access()->rfind("###");
            if (i != std::string::npos)
                title.access()->resize(i);

            if (sit->params.size() >= 3)
                flags.set_from_arg(sit->params[2]);
        }
        else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::Begin")
        {
            ctx.importLevel = sit->level;
            if (sit->params.size() >= 3)
                flags.set_from_arg(sit->params[2]);
            
            if (hasGlfw) {
                ctx.kind = kind = MainWindow;
                //reset sizes from earlier SetNextWindowSize
                TopWindow def(ctx);
                size_x = def.size_x;
                size_y = def.size_y;
                flags &= ~(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
            }
            else {
                ctx.kind = kind = Activity;
                placement = None;
                flags &= ~(ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
            }
        }
        
        ++sit;

        if (sit->kind == cpp::Comment && parseCommentSize)
        {
            auto size = cpp::parse_size(sit->line.substr(2));
            size_x.set_from_arg(size.first);
            size_y.set_from_arg(size.second);
        }
        else if (sit->kind == cpp::Comment && parseCommentPos)
        {
            if (sit->line == "//Left")
                placement = Left;
            else if (sit->line == "//Right")
                placement = Right;
            else if (sit->line == "//Top")
                placement = Top;
            else if (sit->line == "//Bottom")
                placement = Bottom;
            else if (sit->line == "//Center")
                placement = Center;
            else if (sit->line == "//Maximize")
                placement = Maximize;
        }
    }

    userCodeAfter = ctx.userCode;
    ctx.createVars = tmpCreateDeps;
}

void TopWindow::TreeUI(UIContext& ctx)
{
    static const char* NAMES[]{ "MainWindow", "Window", "Popup", "ModalPopup", "Activity" };

    ctx.parents = { this };
    std::string str = ctx.codeGen->GetName();
    bool selected = stx::count(ctx.selected, this) || ctx.snapParent == this;
    if (selected)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
    ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    if (ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_SpanAllColumns))
    {
        if (selected)
            ImGui::PopStyleColor();
        bool activated = ImGui::IsItemClicked(); //todo || ImGui::IsItemActivated();
        ImGui::SameLine(0, 0);
        ImGui::TextDisabled(" : %s", NAMES[kind]);
        if (activated)
        {
            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
                ; // don't participate in group selection toggle(ctx.selected, this);
            else
                ctx.selected = { this };
        }
        for (const auto& ch : children)
            ch->TreeUI(ctx);

        ImGui::TreePop();
    }
    else {
        if (selected)
            ImGui::PopStyleColor();
        ImGui::SameLine(0, 0);
        ImGui::TextDisabled(" : %s", NAMES[kind]);
    }
}

std::vector<UINode::Prop>
TopWindow::Properties()
{
    return {
        { "top.kind", nullptr },
        { "top.@style.bg", &style_bg },
        { "top.@style.menuBg", &style_menuBg },
        { "top.@style.padding", &style_padding },
        { "top.@style.titlePadding", &style_titlePadding },
        { "top.@style.rounding", &style_rounding },
        { "top.@style.borderSize", &style_borderSize },
        { "top.@style.scrollbarSize", &style_scrollbarSize },
        { "top.@style.spacing", &style_spacing },
        { "top.@style.font", &style_font },
        { "top.flags", nullptr },
        { "title", &title, true },
        { "placement", &placement },
        { "size_x", &size_x },
        { "size_y", &size_y },
        { "closeOnEscape", &closeOnEscape },
        { "initialActivity", &initialActivity },
        { "animate", &animate },
    };
}

bool TopWindow::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
    {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(164, 164, 164, 255));
        ImGui::Text("kind");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        int tmp = (int)kind;
        if (ImGui::Combo("##kind", &tmp, "Main Window (GLFW)\0Window\0Popup\0Modal Popup\0Activity (Android)\0"))
        {
            changed = true;
            kind = (Kind)tmp;
        }
        break;
    }
    case 1:
    {
        ImGui::Text("bg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        int clr = (kind == Popup || kind == ModalPopup) ? ImGuiCol_PopupBg : ImGuiCol_WindowBg;
        changed = InputBindable("##bg", &style_bg, clr, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_bg, ctx);
        break;
    }
    case 2:
        ImGui::Text("menuBarBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##menuBarBg", &style_menuBg, ImGuiCol_MenuBarBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("menuBarBg", &style_menuBg, ctx);
        break;
    case 3:
    {
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##style_padding", &style_padding, ctx);
        break;
    }
    case 4:
    {
        ImGui::Text("titlePadding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##style_titlePadding", &style_titlePadding, ctx);
        break;
    }
    case 5:
    {
        ImGui::Text("rounding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##style_rounding", &style_rounding, ctx);
        break;
    }
    case 6:
    {
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##style_border", &style_borderSize, ctx);
        break;
    }
    case 7:
    {
        ImGui::Text("scrollbarSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##style_scrbs", &style_scrollbarSize, ctx);
        break;
    }
    case 8:
    {
        ImGui::Text("spacing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##style_spacing", &style_spacing, ctx);
        break;
    }
    case 9:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##font", &style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 10:
        TreeNodeProp("flags", "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            bool hasAutoResize = flags & ImGuiWindowFlags_AlwaysAutoResize;
            bool hasMB = children.size() && dynamic_cast<MenuBar*>(children[0].get());
            changed = CheckBoxFlags(&flags);
            bool flagsMB = flags & ImGuiWindowFlags_MenuBar;
            if (flagsMB && !hasMB)
                children.insert(children.begin(), std::make_unique<MenuBar>(ctx));
            else if (!flagsMB && hasMB)
                children.erase(children.begin());
            });
        break;
    case 11:
        ImGui::Text("title");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##title", &title, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("title", &title, ctx);
        break;
    case 12:
    {
        ImGui::BeginDisabled(kind == Activity);
        ImGui::Text("placement");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        auto tmp = placement;
        bool isPopup = kind == Popup || kind == ModalPopup;
        bool isPopupOrWindow = isPopup || kind == Window;
        if (ImGui::BeginCombo("##placement", placement.get_id().c_str()))
        {
            if (ImGui::Selectable("None", placement == None))
                placement = None;
            if (isPopupOrWindow && ImGui::Selectable("Left", placement == Left))
                placement = Left;
            if (isPopupOrWindow && ImGui::Selectable("Right", placement == Right))
                placement = Right;
            if (isPopupOrWindow && ImGui::Selectable("Top", placement == Top))
                placement = Top;
            if (isPopupOrWindow && ImGui::Selectable("Bottom", placement == Bottom))
                placement = Bottom;
            if (isPopup && ImGui::Selectable("Center", placement == Center))
                placement = Center;
            if (kind == MainWindow && ImGui::Selectable("Maximize", placement == Maximize))
                placement = Maximize;

            ImGui::EndCombo();
        }
        changed = placement != tmp;
        ImGui::EndDisabled();
        break;
    }
    case 13:
        ImGui::Text("size_x");
        ImGui::TableNextColumn();
        //sometimes too many props are disabled so disable only value here to make it look better
        ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || (kind == MainWindow && placement == Maximize));
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##size_x", &size_x, {}, 0, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_x", &size_x, ctx);
        ImGui::EndDisabled();
        break;
    case 14:
        ImGui::Text("size_y");
        ImGui::TableNextColumn();
        //sometimes too many props are disabled so disable only value here to make it look better
        ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || (kind == MainWindow && placement == Maximize));
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##size_y", &size_y, {}, 0, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_y", &size_y, ctx);
        ImGui::EndDisabled();
        break;
    case 15:
        ImGui::BeginDisabled(kind != Popup && kind != ModalPopup);
        ImGui::Text("closeOnEscape");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##cesc", &closeOnEscape, ctx);
        ImGui::EndDisabled();
        break;
    case 16:
        ImGui::BeginDisabled(kind != Activity);
        ImGui::Text("initialActivity");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##initact", &initialActivity, ctx);
        ImGui::EndDisabled();
        break;
    case 17:
        ImGui::BeginDisabled(kind != Popup && kind != ModalPopup);
        ImGui::Text("animate");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##animate", &animate, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return false;
    }
    return changed;
}

std::vector<UINode::Prop>
TopWindow::Events()
{
    return {
        { "OnWindowAppearing", &onWindowAppearing },
        { "OnBackButton", &onBackButton },
    };
}

bool TopWindow::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    int sat = (i & 1) ? 202 : 164;
    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, sat, 255));
    switch (i)
    {
    case 0:
        ImGui::Text("OnWindowAppearing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent("##OnWindowAppearing", &onWindowAppearing, ctx);
        break;
    case 1:
        ImGui::Text("OnBackButton");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent("##OnBackButton", &onBackButton, ctx);
        break;
    }
    return changed;
}

//---------------------------------------------------

Table::ColumnData::ColumnData()
{
    flags.prefix("ImGuiTableColumnFlags_");
    flags.add$(ImGuiTableColumnFlags_Disabled);
    flags.add$(ImGuiTableColumnFlags_WidthStretch);
    flags.add$(ImGuiTableColumnFlags_WidthFixed);
    flags.add$(ImGuiTableColumnFlags_NoResize);
    flags.add$(ImGuiTableColumnFlags_NoHide);
    flags.add$(ImGuiTableColumnFlags_NoSort);
    flags.add$(ImGuiTableColumnFlags_NoHeaderWidth);
    flags.add$(ImGuiTableColumnFlags_AngledHeader);
}

Table::Table(UIContext& ctx)
{
    size_x = -1; //here 0 has the same effect as -1 but -1 works with our sizing visualization
    size_y = 0;

    flags.prefix("ImGuiTableFlags_");
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
        { "@style.headerBg", &style_headerBg },
        { "@style.rowBg", &style_rowBg },
        { "@style.rowBgAlt", &style_rowBgAlt },
        { "@style.childBg", &style_childBg },
        { "@style.cellPadding", &style_cellPadding },
        { "table.flags", &flags },
        { "table.columns", nullptr },
        { "table.header", &header },
        { "table.rowCount", &itemCount },
        { "table.rowHeight", &rowHeight },
        { "table.rowFilter", &rowFilter },
        { "table.scrollWhenDragging", &scrollWhenDragging },
        { "size_x", &size_x },
        { "size_y", &size_y },
        });
    return props;
}

bool Table::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("headerBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##headerbg", &style_headerBg, ImGuiCol_TableHeaderBg, ctx);
        break;
    case 1:
        ImGui::Text("rowBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##rowbg", &style_rowBg, ImGuiCol_TableRowBg, ctx);
        break;
    case 2:
        ImGui::Text("rowBgAlt");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##rowbgalt", &style_rowBgAlt, ImGuiCol_TableRowBgAlt, ctx);
        break;
    case 3:
        ImGui::Text("childBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##bg", &style_childBg, ImGuiCol_ChildBg, ctx);
        break;
    case 4:
        ImGui::Text("cellPadding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##style_cellPadding", &style_cellPadding, ctx);
        break;
    case 5:
        TreeNodeProp("flags", "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            changed = CheckBoxFlags(&flags);
            });
        break;
    case 6:
    {
        ImGui::Text("columns");
        ImGui::TableNextColumn();
        if (ImGui::Selectable(ICON_FA_PEN_TO_SQUARE, false, 0, { ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
        {
            changed = true;
            tableColumns.columnData = columnData;
            tableColumns.target = &columnData;
            tableColumns.defaultFont = ctx.defaultFont;
            tableColumns.OpenPopup();
        }
        break;
    }
    case 7:
        ImGui::Text("header");
        ImGui::TableNextColumn();
        changed = ImGui::Checkbox("##header", header.access());
        break;
    case 8:
        changed = DataLoopProp("rowCount", &itemCount, ctx);
        break;
    case 9:
        ImGui::Text("rowHeight");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##rowh", &rowHeight, {}, false, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("rowHeight", &rowHeight, ctx);
        break;
    case 10:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("rowFilter");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = ImGui::InputText("##rowFilter", rowFilter.access());
        ImGui::SameLine(0, 0);
        changed |= BindingButton("rowFilter", &rowFilter, ctx);
        ImGui::EndDisabled();
        break;
    case 11:
        ImGui::Text("scrollWhenDragging");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##swd", &scrollWhenDragging, ctx);
        break;
    case 12:
        ImGui::Text("size_x");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_x", &size_x, ctx);
        break;
    case 13:
        ImGui::Text("size_y");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##size_y", &size_y, {}, true, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_y", &size_y, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 14, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
Table::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "onBeginRow", &onBeginRow },
        { "onEndRow", &onEndRow }, 
        });
    return props;
}

bool Table::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("OnBeginRow");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent("##OnBeginRow", &onBeginRow, ctx);
        break;
    case 1:
        ImGui::Text("OnEndRow");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent("##OnEndRow", &onEndRow, ctx);
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

    flags.prefix("ImGuiChildFlags_");
    flags.add$(ImGuiChildFlags_Border);
    flags.add$(ImGuiChildFlags_AlwaysUseWindowPadding);
    flags.add$(ImGuiChildFlags_AlwaysAutoResize);
    flags.add$(ImGuiChildFlags_AutoResizeX);
    flags.add$(ImGuiChildFlags_AutoResizeY);
    flags.add$(ImGuiChildFlags_FrameStyle);
    flags.add$(ImGuiChildFlags_NavFlattened);

    wflags.prefix("ImGuiWindowFlags_");
    wflags.add$(ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    wflags.add$(ImGuiWindowFlags_AlwaysVerticalScrollbar);
    wflags.add$(ImGuiWindowFlags_NoBackground);
    wflags.add$(ImGuiWindowFlags_NoScrollbar);
    wflags.add$(ImGuiWindowFlags_NoSavedSettings);
    wflags.add$(ImGuiWindowFlags_NoNavInputs);
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
        ImRad::HashCombine(ctx.layoutHash, true);
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
    if (!style_outer_padding)
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

    if (!style_outer_padding) 
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
    if (!style_outer_padding)
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

    if (!style_outer_padding)
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
        style_outer_padding = false;
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
        { "@style.color", &style_bg },
        { "@style.border", &style_border },
        { "@style.padding", &style_padding },
        { "@style.spacing", &style_spacing },
        { "@style.rounding", &style_rounding },
        { "@style.borderSize", &style_borderSize },
        { "@style.outer_padding", &style_outer_padding },
        { "child.flags", &flags },
        { "child.wflags", &wflags },
        { "child.column_count", &columnCount },
        { "child.column_border", &columnBorder },
        { "child.item_count", &itemCount },
        { "scrollWhenDragging", &scrollWhenDragging },
        { "size_x", &size_x },
        { "size_y", &size_y },
        });
    return props;
}

bool Child::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("color");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##color", &style_bg, ImGuiCol_ChildBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_bg, ctx);
        break;
    case 1:
        ImGui::Text("border");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##border", &style_border, ImGuiCol_Border, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("border", &style_border, ctx);
        break;
    case 2:
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##padding", &style_padding, ctx);
        break;
    case 3:
        ImGui::Text("spacing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##spacing", &style_spacing, ctx);
        break;
    case 4:
        ImGui::Text("rounding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##rounding", &style_rounding, ctx);
        break;
    case 5:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##borderSize", &style_borderSize, ctx);
        break;
    case 6:
        ImGui::Text("outerPadding");
        ImGui::TableNextColumn();
        changed = InputDirectVal("##outerPadding", &style_outer_padding, ctx);
        break;
    case 7:
        TreeNodeProp("flags", "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            int ch = CheckBoxFlags(&flags);
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
    case 8:
        TreeNodeProp("windowFlags", "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            changed = CheckBoxFlags(&wflags);
            });
        break;
    case 9:
        ImGui::Text("columnCount");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##columnCount", &columnCount, 1, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("columnCount", &columnCount, ctx);
        break;
    case 10:
        ImGui::Text("columnBorder");
        ImGui::TableNextColumn();
        changed = InputDirectVal("##columnBorder", &columnBorder, ctx);
        break;
    case 11:
        changed = DataLoopProp("itemCount", &itemCount, ctx);
        break;
    case 12:
        ImGui::Text("scrollWhenDragging");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##swd", &scrollWhenDragging, ctx);
        break;
    case 13:
        ImGui::Text("size_x");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        ImGui::BeginDisabled((flags & ImGuiChildFlags_AlwaysAutoResize) && (flags & ImGuiChildFlags_AutoResizeX));
        changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_x", &size_x, ctx);
        ImGui::EndDisabled();
        break;
    case 14:
        ImGui::Text("size_y");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        ImGui::BeginDisabled((flags & ImGuiChildFlags_AlwaysAutoResize) && (flags & ImGuiChildFlags_AutoResizeY));
        changed = InputBindable("##size_y", &size_y, {}, InputBindable_StretchButton, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_y", &size_y, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::PropertyUI(i - 15, ctx);
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
        { "@style.bg", &style_bg },
        { "@style.active", &style_active },
        { "splitter.position", &position },
        { "splitter.min1", &min_size1 },
        { "splitter.min2", &min_size2 },
        { "size_x", &size_x },
        { "size_y", &size_y },
        });
    return props;
}

bool Splitter::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("bg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##bg", &style_bg, ImGuiCol_ChildBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_bg, ctx);
        break;
    case 1:
        ImGui::Text("active");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##active", &style_active, ImGuiCol_SeparatorActive, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("active", &style_active, ctx);
        break;
    case 2:
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_REF_CLR);
        ImGui::Text("position");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef("##position", &position, false, ctx);
        break;
    case 3:
        ImGui::Text("minSize1");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = ImGui::InputFloat("##min_size1", min_size1.access());
        break;
    case 4:
        ImGui::Text("minSize2");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = ImGui::InputFloat("##min_size2", min_size2.access());
        break;
    case 5:
        ImGui::Text("size_x");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##size_x", &size_x, {}, InputBindable_StretchButton, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_x", &size_x, ctx);
        break;
    case 6:
        ImGui::Text("size_y");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##size_y", &size_y, {}, true, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_y", &size_y, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 7, ctx);
    }
    return changed;
}

//---------------------------------------------------------

CollapsingHeader::CollapsingHeader(UIContext& ctx)
{
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
    if (ImGui::CollapsingHeader(DRAW_STR(label)))
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
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    cached_pos.x -= pad.x;
    cached_size.x = ImGui::GetContentRegionAvail().x + 2 * pad.x;
    cached_size.y = ImGui::GetCursorPosY() - p1.y - pad.y;
}

void CollapsingHeader::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!style_header.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Header, " << style_header.to_arg() << ");\n";
    if (!style_hovered.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_HeaderHovered, " << style_hovered.to_arg() << ");\n";
    if (!style_active.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_HeaderActive, " << style_active.to_arg() << ");\n";

    os << ctx.ind << "ImGui::SetNextItemOpen(";
    if (open.has_value())
        os << open.to_arg() << ", ImGuiCond_Appearing";
    else
        os << open.to_arg();
    os << ");\n";

    os << ctx.ind << "if (ImGui::CollapsingHeader(" << label.to_arg() << "))\n"
        << ctx.ind << "{\n";
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
    }
}

std::vector<UINode::Prop>
CollapsingHeader::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "@style.text", &style_text },
        { "@style.header", &style_header },
        { "@style.hovered", &style_hovered },
        { "@style.active", &style_active },
        { "@style.font", &style_font },
        { "label", &label, true },
        { "open", &open }
        });
    return props;
}

bool CollapsingHeader::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##color", &style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("header");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##header", &style_header, ImGuiCol_Header, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("header", &style_header, ctx);
        break;
    case 2:
        ImGui::Text("hovered");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##hovered", &style_hovered, ImGuiCol_HeaderHovered, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("hovered", &style_hovered, ctx);
        break;
    case 3:
        ImGui::Text("active");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##active", &style_active, ImGuiCol_HeaderActive, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("active", &style_active, ctx);
        break;
    case 4:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##font", &style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 5:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##label", &label, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 6:
        ImGui::Text("open");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##open", &open, true, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("open", &open, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 7, ctx);
    }
    return changed;
}

//---------------------------------------------------------

TreeNode::TreeNode(UIContext& ctx)
{
    flags.prefix("ImGuiTreeNodeFlags_");
    //flags.add$(ImGuiTreeNodeFlags_Framed);
    flags.add$(ImGuiTreeNodeFlags_FramePadding);
    flags.add$(ImGuiTreeNodeFlags_Leaf);
    flags.add$(ImGuiTreeNodeFlags_Bullet);
    flags.add$(ImGuiTreeNodeFlags_OpenOnArrow);
    flags.add$(ImGuiTreeNodeFlags_OpenOnDoubleClick);
    flags.add$(ImGuiTreeNodeFlags_SpanAvailWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanFullWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanTextWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanAllColumns);
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
    cached_size = ImGui::GetItemRectSize();
    ImVec2 sp = ImGui::GetStyle().ItemSpacing;
    
    if (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth))
    {
        cached_size.x = ImGui::GetContentRegionAvail().x - p1.x + sp.x;
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
    if (open.has_value())
        os << ctx.ind << "ImGui::SetNextItemOpen(" << open.to_arg() << ", ImGuiCond_Appearing);\n";
    else if (!open.empty())
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
        { "@style.text", &style_text },
        { "@style.font", &style_font },
        { "flags", &flags },
        { "label", &label, true },
        { "open", &open },
        });
    return props;
}

bool TreeNode::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##color", &style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##font", &style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 2:
        TreeNodeProp("flags", "...", [&]{
            ImGui::TableNextColumn();
            ImGui::Spacing();
            changed = CheckBoxFlags(&flags);
            });
        break;
    case 3:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##label", &label, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 4:
        ImGui::Text("open");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##open", &open, true, ctx);
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
    flags.prefix("ImGuiTabBarFlags_");
    flags.add$(ImGuiTabBarFlags_Reorderable);
    flags.add$(ImGuiTabBarFlags_TabListPopupButton);
    flags.add$(ImGuiTabBarFlags_NoTabListScrollingButtons);
    flags.add$(ImGuiTabBarFlags_DrawSelectedOverline);
    flags.add$(ImGuiTabBarFlags_FittingPolicyResizeDown);
    flags.add$(ImGuiTabBarFlags_FittingPolicyScroll);

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
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    cached_pos.x -= pad.x;
    cached_size.x = ImGui::GetContentRegionAvail().x + 2 * pad.x;
    cached_size.y = ImGui::GetCursorPosY() - p1.y;
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
        { "@style.text", &style_text },
        { "@style.tab", &style_tab },
        { "@style.hovered", &style_hovered },
        { "@style.selected", &style_selected },
        { "@style.overline", &style_overline },
        { "@style.regularWidth", &style_regularWidth },
        { "@style.padding", &style_framePadding },
        { "@style.font", &style_font },
        { "flags", &flags },
        { "tabCount", &itemCount },
        { "activeTab", &activeTab },
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
        changed = InputBindable("##color", &style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("tab");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##bg", &style_tab, ImGuiCol_Tab, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_tab, ctx);
        break;
    case 2:
        ImGui::Text("hovered");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##hovered", &style_hovered, ImGuiCol_TabHovered, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("hovered", &style_hovered, ctx);
        break;
    case 3:
        ImGui::Text("selected");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##selected", &style_selected, ImGuiCol_TabSelected, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("selected", &style_selected, ctx);
        break;
    case 4:
        ImGui::Text("overline");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##overline", &style_overline, ImGuiCol_TabSelectedOverline, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("overline", &style_overline, ctx);
        break;
    case 5:
        ImGui::Text("regularWidth");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##regu", &style_regularWidth, ctx);
        break;
    case 6:
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##padding", &style_framePadding, ctx);
        break;
    case 7:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable("##font", &style_font, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_font, ctx);
        break;
    case 8:
        TreeNodeProp("flags", "...", [&] {
            ImGui::TableNextColumn();
            ImGui::Spacing();
            changed = CheckBoxFlags(&flags);
            });
        break;
    case 9:
        changed = DataLoopProp("tabCount", &itemCount, ctx);
        break;
    case 10:
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_REF_CLR);
        ImGui::Text("activeTab");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef("##activeTab", &activeTab, true, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 11, ctx);
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

void TabItem::DoDrawExtra(UIContext& ctx)
{
    if (ctx.parents.empty())
        return;

    ImGui::PushFont(nullptr);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing);
    assert(ctx.parents.back() == this);
    auto* parent = ctx.parents[ctx.parents.size() - 2];
    size_t idx = stx::find_if(parent->children, [this](const auto& ch) { return ch.get() == this; })
        - parent->children.begin();

    ImGui::SetNextWindowPos(cached_pos, 0, { 0, 1.f });
    ImGui::Begin("extra", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);

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
    ImGui::PopStyleVar(2);
    ImGui::PopFont();
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
        { "label", &label, true },
        { "closeButton", &closeButton }
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
        changed = InputBindable("##label", &label, ctx);
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
        { "onClose", &onClose },
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
        ImGui::Text("OnClose");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent("##onClose", &onClose, ctx);
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
    shortcut.set_global(true);
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

void MenuIt::DoDrawExtra(UIContext& ctx)
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
    ImGui::PushFont(nullptr);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing);

    const ImVec2 bsize{ 30, 0 };
    ImVec2 pos = cached_pos;
    if (vertical) {
        pos = ImGui::GetWindowPos();
        //pos.x -= ImGui::GetStyle().ItemSpacing.x;
    }
    ImGui::SetNextWindowPos(pos, 0, vertical ? ImVec2{ 0, 1.f } : ImVec2{ 0, 1.f });
    ImGui::Begin("extra", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

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
    ImGui::PopStyleVar(2);
    ImGui::PopFont();
}

void MenuIt::CalcSizeEx(ImVec2 x1, UIContext& ctx)
{
    assert(ctx.parents.back() == this);
    const UINode* par = ctx.parents[ctx.parents.size() - 2];
    bool mbm = dynamic_cast<const MenuBar*>(par);
    
    ImVec2 sp = ImGui::GetStyle().ItemSpacing;
    const ImGuiMenuColumns* mc = &ImGui::GetCurrentWindow()->DC.MenuColumns;
    cached_pos.x += mc->OffsetLabel;
    cached_size = ImGui::CalcTextSize(label.c_str(), nullptr, true);
    cached_size.x += sp.x;
    if (contextMenu)
    {
        cached_pos = ctx.rootWin->Pos;
    }
    else if (mbm)
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
        { "@style.padding", &style_padding },
        { "@style.spacing", &style_spacing },
        { "@style.rounding", &style_rounding },
        { "ownerDraw", &ownerDraw },
        { "label", &label, true },
        { "shortcut", &shortcut },
        { "checked", &checked },
        { "separator", &separator },
        });
    return props;
}

bool MenuIt::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::BeginDisabled(!contextMenu);
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##padding", &style_padding, ctx);
        ImGui::EndDisabled();
        break;
    case 1:
        ImGui::BeginDisabled(!contextMenu);
        ImGui::Text("spacing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##spacing", &style_spacing, ctx);
        ImGui::EndDisabled();
        break;
    case 2:
        ImGui::BeginDisabled(!contextMenu);
        ImGui::Text("rounding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##rounding", &style_rounding, ctx);
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
        changed = InputDirectVal("##label", &label, ctx);
        ImGui::EndDisabled();
        break;
    case 5:
        ImGui::BeginDisabled(ownerDraw || contextMenu || children.size());
        ImGui::Text("shortcut");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal("##shortcut", &shortcut, false, ctx);
        ImGui::EndDisabled();
        break;
    case 6:
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, FIELD_REF_CLR);
        ImGui::BeginDisabled(ownerDraw || contextMenu || children.size());
        ImGui::Text("checked");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef("##checked", &checked, true, ctx);
        ImGui::EndDisabled();
        break;
    case 7:
        ImGui::BeginDisabled(contextMenu);
        ImGui::Text("separator");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = ImGui::Checkbox("##separator", separator.access());
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
        { "onChange", &onChange },
        });
    return props;
}

bool MenuIt::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::BeginDisabled(children.size());
        ImGui::Text(ownerDraw ? "OnDraw" : "OnChange");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent("##onChange", &onChange, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}
