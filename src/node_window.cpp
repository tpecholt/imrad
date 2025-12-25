#include "node_window.h"
#include "node_container.h"
#include "node_standard.h"
#include "stx.h"
#include "imrad.h"
#include "cppgen.h"
#include "binding_input.h"
#include "binding_eval.h"
#include <algorithm>
#include <array>

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

    kind.add("Main Window (GLFW)", MainWindow);
    kind.add$(Window);
    kind.add$(Popup);
    kind.add$(ModalPopup);
    kind.add("Activity (Android)", Activity);

    flags.add$(ImGuiWindowFlags_AlwaysAutoResize);
    flags.add$(ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    flags.add$(ImGuiWindowFlags_AlwaysVerticalScrollbar);
    flags.add$(ImGuiWindowFlags_MenuBar);
    flags.add$(ImGuiWindowFlags_NoBackground);
    flags.add$(ImGuiWindowFlags_NoCollapse);
    flags.add$(ImGuiWindowFlags_NoDocking);
    flags.add$(ImGuiWindowFlags_NoFocusOnAppearing);
    flags.add$(ImGuiWindowFlags_NoMove);
    flags.add$(ImGuiWindowFlags_NoNavInputs);
    flags.add$(ImGuiWindowFlags_NoNavFocus);
    flags.add$(ImGuiWindowFlags_NoResize);
    flags.add$(ImGuiWindowFlags_NoSavedSettings);
    flags.add$(ImGuiWindowFlags_NoScrollbar);
    flags.add$(ImGuiWindowFlags_NoScrollWithMouse);
    flags.add$(ImGuiWindowFlags_NoTitleBar);

    placement.add$(None);
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
    bool dimAll = ctx.activePopups.size(); //from last frame
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

    if (!style_fontName.empty() || !style_fontSize.empty())
        ImGui::PushFont(style_fontName.eval(ctx), style_fontSize.eval(ctx));
    if (style_padding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style_padding.eval_px(ctx));
    if (style_spacing.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style_spacing.eval_px(ctx));
    if (style_innerSpacing.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, style_innerSpacing.eval_px(ctx));
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
        w = minSize_x.eval_px(ImGuiAxis_X, ctx);
        h = minSize_y.eval_px(ImGuiAxis_Y, ctx);
        ImGui::SetNextWindowSizeConstraints({ w, h }, { FLT_MAX, FLT_MAX });
    }

    if (style_titlePadding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, style_titlePadding.eval_px(ctx));

    bool tmp;
    ImGui::Begin(cap.c_str(), &tmp, fl);

    if (style_titlePadding.has_value())
        ImGui::PopStyleVar();

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

    ImVec2 gap = style_placementGap.has_value() ? (ImVec2)style_placementGap : ImVec2{ 0, 0 };
    if (placement == Left && !gap.x)
        ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersLeft);
    else if (placement == Right && !gap.x)
        ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersRight);
    else if (placement == Top && !gap.y)
        ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersTop);
    else if (placement == Bottom && !gap.y)
        ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCornersBottom);

    ImGui::GetCurrentContext()->NavHighlightItemUnderNav = true;
    ImGui::GetCurrentContext()->NavCursorVisible = false;
    if (dimAll)
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.2f);

    for (size_t i = 0; i < children.size(); ++i)
        children[i]->Draw(ctx);

    if (dimAll)
        ImGui::PopStyleVar();
    ImGui::GetCurrentContext()->NavCursorVisible = true;
    ImGui::GetCurrentContext()->NavHighlightItemUnderNav = false;

    for (size_t i = 0; i < children.size(); ++i)
        children[i]->DrawTools(ctx);

    //use all client area to allow snapping close to the border
    auto pad = ImGui::GetStyle().WindowPadding - ImVec2(3+1, 3);
    auto mi = ImGui::GetWindowContentRegionMin() - pad;
    auto ma = ImGui::GetWindowContentRegionMax() + pad;
    cached_pos = ImGui::GetWindowPos() + mi;
    cached_size = ma - mi;

    if (!ImGui::GetTopMostAndVisiblePopupModal() && ctx.activePopups.size() &&
        ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        ctx.selected = { this };
    }
    bool allowed = !ImGui::GetTopMostAndVisiblePopupModal() && ctx.activePopups.empty();
    if (allowed && ctx.mode == UIContext::NormalSelection)
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            if (stx::count(ctx.selected, ctx.hovered)) //selected widget might want to do SnapMove instead
                ctx.selStart.x = FLT_MAX;
            else
                ctx.selStart = ctx.selEnd = ImGui::GetMousePos();
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
            !ctx.beingResized &&
            ctx.selStart.x != FLT_MAX && //todo: initiate SnapMove, cursor is out of the widget already
            (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) || ImGui::IsWindowFocused())) //no RectSel during docking
        {
            ctx.mode = UIContext::RectSelection;
            ctx.selEnd = ImGui::GetMousePos();
        }
    }
    if (ctx.mode == UIContext::NormalSelection &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        ImRect(ctx.designAreaMin, ctx.designAreaMax).Contains(ImGui::GetMousePos()) &&
        (ImGui::IsWindowHovered() || //includes !GetTopMostAndVisiblePopupModal
        (!ImGui::GetTopMostAndVisiblePopupModal() && !ImGui::IsAnyItemHovered() && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        ))
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
            auto sel = FindInRect({ a, b });
            stx::erase(sel, this);
            if (sel.size()) {
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
                    for (auto s : sel)
                        if (!stx::count(ctx.selected, s))
                            ctx.selected.push_back(s);
                }
                else
                    ctx.selected = sel;
            }
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

    if ((ctx.mode == UIContext::SnapInsert || ctx.mode == UIContext::SnapMove) &&
        !ctx.snapParent && ImGui::IsWindowHovered())
    {
        DrawSnap(ctx);
    }
    if (ctx.snapParent && ctx.snapIndex == -1)
    {
        ctx.snapParent = nullptr;
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
    if (style_innerSpacing.has_value())
        ImGui::PopStyleVar();
    if (style_padding.has_value())
        ImGui::PopStyleVar();
    if (style_fontName.has_value() || style_fontSize.has_value())
        ImGui::PopFont();
}

void TopWindow::Export(std::ostream& os, UIContext& ctx)
{
    ctx.varCounter = 1;
    ctx.parents = { this };
    ctx.kind = kind;
    ctx.errors.clear();
    ctx.unit = ctx.unit == "px" ? "" : ctx.unit;

    ctx.codeGen->RemovePrefixedVars(ctx.codeGen->HBOX_NAME);
    ctx.codeGen->RemovePrefixedVars(ctx.codeGen->VBOX_NAME);

    //todo: put before ///@ params
    if (userCodeBefore != "")
        os << userCodeBefore << "\n";

    //provide stable ID when title changes
    bindable<std::string> titleId = title;
    if (titleId.access()->find("##") == std::string::npos)
        *titleId.access() += "###" + ctx.codeGen->GetName();
    std::string tit = titleId.to_arg();
    bool hasMB = children.size() && dynamic_cast<MenuBar*>(children[0].get());
    bool autoSize = flags & ImGuiWindowFlags_AlwaysAutoResize;

    os << ctx.ind << "/// @begin TopWindow\n";

    if (ctx.unit == "dp")
    {
        os << ctx.ind << "const float dp = ImRad::GetUserData().dpiScale;\n";
    }

    if (kind == Popup || kind == ModalPopup)
    {
        os << ctx.ind << "ID = ImGui::GetID(\"###" << ctx.codeGen->GetName() << "\");\n";
    }
    else if (kind == Activity)
    {
        if (initialActivity)
        {
            os << ctx.ind << "if (ImRad::GetUserData().activeActivity == \"\")\n";
            ctx.ind_up();
            os << ctx.ind << "Open();\n";
            ctx.ind_down();
        }
        os << ctx.ind << "if (ImRad::GetUserData().activeActivity != \"" << ctx.codeGen->GetName() << "\")\n";
        ctx.ind_up();
        os << ctx.ind << "return;\n";
        ctx.ind_down();
    }

    if (!style_fontName.empty() || !style_fontSize.empty())
    {
        os << ctx.ind << "ImGui::PushFont(" << style_fontName.to_arg() << ", "
            << style_fontSize.to_arg() << ");\n";
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
    if (style_innerSpacing.has_value())
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, "
            << style_innerSpacing.to_arg(ctx.unit) << ");\n";
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

        auto fl = flags;
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

            os << ctx.ind << "glfwSetWindowSizeLimits(window, " <<
                minSize_x.to_arg(ctx.unit) << ", " << minSize_y.to_arg(ctx.unit) <<
                ", GLFW_DONT_CARE, GLFW_DONT_CARE);\n";

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
        os << ctx.ind << "ImGui::SetNextWindowPos(ImRad::GetUserData().WorkRect().Min);\n";
        os << ctx.ind << "ImGui::SetNextWindowSize(ImRad::GetUserData().WorkRect().GetSize());";
        //signal designed size
        os << " //{ " << size_x.to_arg(ctx.unit) << ", " << size_y.to_arg(ctx.unit) << " }\n";
        auto fl = flags;
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
        ImVec2 gap = style_placementGap.has_value() ? (ImVec2)style_placementGap : ImVec2{ 0, 0 };

        //pos
        if (placement == Left || placement == Top)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos({ "
                << "ImRad::GetUserData().WorkRect().Min.x + " << gap.x << ", "
                << "ImRad::GetUserData().WorkRect().Min.y + " << gap.y << " });";
            os << (placement == Left ? " //Left" : " //Top");
            os << ", Gap={" << gap.x << "," << gap.y << "}\n";
        }
        else if (placement == Right || placement == Bottom)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos({ "
                << "ImRad::GetUserData().WorkRect().Max.x - " << gap.x << ", "
                << "ImRad::GetUserData().WorkRect().Max.y - " << gap.y << " }, "
                << "0, { 1, 1 });";
            os << (placement == Right ? " //Right" : " //Bottom");
            os << ", Gap={" << gap.x << "," << gap.y << "}\n";
        }

        //size
        os << ctx.ind << "ImGui::SetNextWindowSize({ ";

        if (placement == Top || placement == Bottom)
            os << "ImRad::GetUserData().WorkRect().GetWidth() - 2*" << gap.x;
        else if (autoSize)
            os << "0";
        else
            os << size_x.to_arg(ctx.unit);

        os << ", ";

        if (placement == Left || placement == Right)
            os << "ImRad::GetUserData().WorkRect().GetHeight() - 2*" << gap.y;
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

        if (!autoSize)
        {
            os << ctx.ind << "ImGui::SetNextWindowSizeConstraints({ " <<
                minSize_x.to_arg(ctx.unit) << ", " << minSize_y.to_arg(ctx.unit) <<
                " }, { FLT_MAX, FLT_MAX });\n";
        }

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
        ImVec2 gap = style_placementGap.has_value() ? (ImVec2)style_placementGap : ImVec2{ 0, 0 };

        if (placement == Left || placement == Top)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos({ ";
            os << "ImRad::GetUserData().WorkRect().Min.x + " << gap.x;
            if (animate)
                os << " + animPos.x";
            os << ", ImRad::GetUserData().WorkRect().Min.y + " << gap.y;
            if (animate)
                os << " + animPos.y";
            os << " });";
            os << (placement == Left ? " //Left" : " //Top");
            os << ",Gap={" << gap.x << "," << gap.y << "}\n";
        }
        else if (placement == Right || placement == Bottom)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos({ ";
            os << "ImRad::GetUserData().WorkRect().Max.x - " << gap.x;
            if (animate)
                os << " - animPos.x";
            os << ", ImRad::GetUserData().WorkRect().Max.y - " << gap.y;
            if (animate)
                os << " - animPos.y";
            os << " }, 0, { 1, 1 });";
            os << (placement == Right ? " //Right" : " //Bottom");
            os << ",Gap={" << gap.x << "," << gap.y << "}\n";
        }
        else if (placement == Center)
        {
            os << ctx.ind << "ImGui::SetNextWindowPos(";
            if (animate)
                os << "{ ImRad::GetUserData().WorkRect().GetCenter().x, ImRad::GetUserData().WorkRect().GetCenter().y + animPos.y }, 0, { 0.5f, 0.5f }";
            else
                os << "ImRad::GetUserData().WorkRect().GetCenter(), 0, { 0.5f, 0.5f }";
            os << "); //Center\n";
        }

        //size
        os << ctx.ind << "ImGui::SetNextWindowSize({ ";

        if (placement == Top || placement == Bottom)
            os << "ImRad::GetUserData().WorkRect().GetWidth() - 2*" << gap.x;
        else if (autoSize)
            os << "0";
        else
            os << size_x.to_arg(ctx.unit);

        os << ", ";

        if (placement == Left || placement == Right)
            os << "ImRad::GetUserData().WorkRect().GetHeight() - 2*" << gap.y;
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

        if (!autoSize)
        {
            os << ctx.ind << "ImGui::SetNextWindowSizeConstraints({ " <<
                minSize_x.to_arg(ctx.unit) << ", " << minSize_y.to_arg(ctx.unit) <<
                " }, { FLT_MAX, FLT_MAX });\n";
        }

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
                os << ", animPos, ImRad::GetUserData().dimBgRatio))\n";
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
            os << ctx.ind << "if (ImRad::GetUserData().activeActivity != \"\")\n";
            ctx.ind_up();
            os << ctx.ind << "ImRad::RenderDimmedBackground(ImRad::GetUserData().WorkRect(), ImRad::GetUserData().dimBgRatio);\n";
            ctx.ind_down();
        }
        if (style_rounding &&
            !gap.x &&
            (placement == Left || placement == Right))
        {
            os << ctx.ind << "ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCorners";
            if (placement == Left)
                os << "Left";
            else if (placement == Right)
                os << "Right";
            os << ");\n";
        }
        if (style_rounding &&
            !gap.y &&
            (placement == Top || placement == Bottom))
        {
            os << ctx.ind << "ImRad::RenderFilledWindowCorners(ImDrawFlags_RoundCorners";
            if (placement == Top)
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
    if (style_innerSpacing.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_padding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_menuBg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_fontName.empty() || !style_fontSize.empty())
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
        else if ((sit->kind == cpp::IfCallThenCall && sit->cond == "ImRad::GetUserData().activeActivity==\"\"") ||
            (sit->kind == cpp::IfStmt && sit->cond == "ioUserData->activeActivity==\"\"")) //compatibility
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
                style_fontName.set_from_arg(sit->params[0]);
            if (sit->params.size() >= 2)
                style_fontSize.set_from_arg(sit->params[1]);
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
            else if (sit->params[0] == "ImGuiStyleVar_ItemInnerSpacing")
                style_innerSpacing.set_from_arg(sit->params[1]);
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
        else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextWindowSizeConstraints")
        {
            if (sit->params.size())
            {
                auto size = cpp::parse_size(sit->params[0]);
                minSize_x.set_from_arg(size.first);
                minSize_y.set_from_arg(size.second);
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
        else if (sit->kind == cpp::CallExpr && sit->callee == "glfwSetWindowSizeLimits" &&
            windowAppearingBlock)
       {
            hasGlfw = true;
            if (sit->params.size() >= 3) {
                minSize_x.set_from_arg(sit->params[1]);
                minSize_y.set_from_arg(sit->params[2]);
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

            if (sit->params.size() >= 3) {
                if (!flags.set_from_arg(sit->params[2]))
                    PushError(ctx, "unrecognized flag in \"" + sit->params[2] + "\"");
            }
        }
        else if ((sit->kind == cpp::IfCallBlock || sit->kind == cpp::CallExpr) &&
            sit->callee == "ImGui::BeginPopup")
        {
            ctx.kind = kind = Popup;
            title.set_from_arg(sit->params[0]);
            size_t i = title.access()->rfind("###");
            if (i != std::string::npos)
                title.access()->resize(i);

            if (sit->params.size() >= 2) {
                if (!flags.set_from_arg(sit->params[1]))
                    PushError(ctx, "unrecognized flag in \"" + sit->params[1] + "\"");
            }
        }
        else if (sit->kind == cpp::IfCallBlock && sit->callee == "isOpen&&ImGui::Begin")
        {
            ctx.kind = kind = Window;
            title.set_from_arg(sit->params[0]);
            size_t i = title.access()->rfind("###");
            if (i != std::string::npos)
                title.access()->resize(i);

            if (sit->params.size() >= 3) {
                if (!flags.set_from_arg(sit->params[2]))
                    PushError(ctx, "unrecognized flag in \"" + sit->params[2] + "\"");
            }
        }
        else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::Begin")
        {
            ctx.importLevel = sit->level;
            if (sit->params.size() >= 3) {
                if (!flags.set_from_arg(sit->params[2]))
                    PushError(ctx, "unrecognized flag in \"" + sit->params[2] + "\"");
            }

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
            if (sit->line.find("Left") != std::string::npos)
                placement = Left;
            else if (sit->line.find("Right") != std::string::npos)
                placement = Right;
            else if (sit->line.find("Top") != std::string::npos)
                placement = Top;
            else if (sit->line.find("Bottom") != std::string::npos)
                placement = Bottom;
            else if (sit->line.find("Center") != std::string::npos)
                placement = Center;
            else if (sit->line.find("Maximize") != std::string::npos)
                placement = Maximize;

            size_t i = sit->line.find("Gap=");
            if (i != std::string::npos)
                style_placementGap.set_from_arg(sit->line.substr(i + 4));
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
        { "appearance.bg", &style_bg },
        { "appearance.menuBg", &style_menuBg },
        { "appearance.padding", &style_padding },
        { "appearance.titlePadding", &style_titlePadding },
        { "appearance.rounding", &style_rounding },
        { "appearance.borderSize", &style_borderSize },
        { "appearance.scrollbarSize", &style_scrollbarSize },
        { "appearance.spacing", &style_spacing },
        { "appearance.innerSpacing", &style_innerSpacing },
        { "appearance.placementGap", &style_placementGap },
        { "appearance.font.summary", nullptr },
        { "appearance.font.name", &style_fontName },
        { "appearance.font.size", &style_fontSize },
        { "behavior.flags", nullptr },
        { "behavior.kind", nullptr },
        { "behavior.title", &title, true },
        { "behavior.closeOnEscape", &closeOnEscape },
        { "behavior.initialActivity", &initialActivity },
        { "behavior.animate", &animate },
        { "layout.size.summary", nullptr },
        { "layout.size.size_x", &size_x },
        { "layout.size.size_y", &size_y },
        { "layout.minSize.summary", nullptr },
        { "layout.minSize.size_x", &minSize_x },
        { "layout.minSize.size_y", &minSize_y },
        { "layout.placement", &placement },
    };
}

bool TopWindow::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl = 0;
    switch (i)
    {
    case 0:
    {
        ImGui::Text("bg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        int clr = (kind == Popup || kind == ModalPopup) ? ImGuiCol_PopupBg : ImGuiCol_WindowBg;
        changed = InputBindable(&style_bg, clr, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_bg, ctx);
        break;
    }
    case 1:
        ImGui::Text("menuBarBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_menuBg, ImGuiCol_MenuBarBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("menuBarBg", &style_menuBg, ctx);
        break;
    case 2:
    {
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_padding, ctx);
        break;
    }
    case 3:
    {
        ImGui::Text("titlePadding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_titlePadding, ctx);
        break;
    }
    case 4:
    {
        ImGui::Text("rounding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_rounding, ctx);
        break;
    }
    case 5:
    {
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_borderSize, ctx);
        break;
    }
    case 6:
    {
        ImGui::Text("scrollbarSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_scrollbarSize, ctx);
        break;
    }
    case 7:
    {
        ImGui::Text("spacing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_spacing, ctx);
        break;
    }
    case 8:
    {
        ImGui::Text("innerSpacing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_innerSpacing, ctx);
        break;
    }
    case 9:
    {
        ImGui::Text("placementGap");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_placementGap, ctx);
        break;
    }
    case 10:
    {
        ImGui::Text("font");
        ImGui::TableNextColumn();
        fl = style_fontName != Defaults().style_fontName || style_fontSize != Defaults().style_fontSize;
        TextFontInfo(style_fontName, style_fontSize, fl, ctx);
        break;
    }
    case 11:
        ImGui::Text("name");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontName, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_fontName, ctx);
        break;
    case 12:
        ImGui::Text("size");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontSize, InputBindable_ParentStr, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font_size", &style_fontSize, ctx);
        break;
    case 13:
        ImGui::Text("kind");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = kind != Defaults().kind ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&kind, fl, ctx);
        break;
    case 14:
    {
        bool hasAutoResize = flags & ImGuiWindowFlags_AlwaysAutoResize;
        bool hasMB = children.size() && dynamic_cast<MenuBar*>(children[0].get());
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        if (changed)
        {
            bool flagsMB = flags & ImGuiWindowFlags_MenuBar;
            if (flagsMB && !hasMB)
                children.insert(children.begin(), std::make_unique<MenuBar>(ctx));
            else if (!flagsMB && hasMB)
                children.erase(children.begin());
        }
        break;
    }
    case 15:
        ImGui::Text("title");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&title, 0, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("title", &title, ctx);
        break;
    case 16:
        ImGui::BeginDisabled(kind != Popup && kind != ModalPopup);
        ImGui::Text("closeOnEscape");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = closeOnEscape != Defaults().closeOnEscape ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&closeOnEscape, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 17:
        ImGui::BeginDisabled(kind != Activity);
        ImGui::Text("initialActivity");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = initialActivity != Defaults().initialActivity ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&initialActivity, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 18:
        ImGui::BeginDisabled(kind != Popup && kind != ModalPopup);
        ImGui::Text("animate");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = animate != Defaults().animate ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&animate, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 19:
        ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || placement == Maximize);
        ImGui::Text(kind == Activity ? "designSize" : "size");
        ImGui::TableNextColumn();
        fl = size_x != Defaults().size_x || size_y != Defaults().size_y;
        ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && fl ?
            ctx.pgbFont : ctx.pgFont);
        ImGui::TextUnformatted((size_x.to_arg("") + ", " + size_y.to_arg("")).c_str());
        ImGui::PopFont();
        ImGui::EndDisabled();
        break;
    case 20:
        ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || placement == Maximize);
        ImGui::Text("size_x");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = size_x != Defaults().size_x ? InputBindable_Modified : 0;
        changed = InputBindable(&size_x, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_x", &size_x, ctx);
        ImGui::EndDisabled();
        break;
    case 21:
        ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || placement == Maximize);
        ImGui::Text("size_y");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = size_y != Defaults().size_y ? InputBindable_Modified : 0;
        changed = InputBindable(&size_y, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("size_y", &size_y, ctx);
        ImGui::EndDisabled();
        break;
    case 22:
        ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || kind == Activity);
        ImGui::Text("minimumSize");
        ImGui::TableNextColumn();
        fl = minSize_x != Defaults().minSize_x || minSize_y != Defaults().minSize_y;
        ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && fl ?
            ctx.pgbFont : ctx.pgFont);
        ImGui::TextUnformatted((minSize_x.to_arg("") + ", " + minSize_y.to_arg("")).c_str());
        ImGui::PopFont();
        ImGui::EndDisabled();
        break;
    case 23:
        ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || kind == Activity);
        ImGui::Text("size_x");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = minSize_x != Defaults().minSize_x ? InputBindable_Modified : 0;
        changed = InputBindable(&minSize_x, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("minSize_x", &minSize_x, ctx);
        ImGui::EndDisabled();
        break;
    case 24:
        ImGui::BeginDisabled((flags & ImGuiWindowFlags_AlwaysAutoResize) || kind == Activity);
        ImGui::Text("size_y");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = minSize_y != Defaults().minSize_y ? InputBindable_Modified : 0;
        changed = InputBindable(&minSize_y, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("minSize_y", &minSize_y, ctx);
        ImGui::EndDisabled();
        break;
    case 25:
    {
        ImGui::BeginDisabled(kind == Activity);
        ImGui::Text("placement");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());

        placement.clear();
        placement.add("None", None);
        if (kind == Popup || kind == ModalPopup || kind == Window) {
            placement.add$(Left);
            placement.add$(Right);
            placement.add$(Top);
            placement.add$(Bottom);
        }
        if (kind == Popup || kind == ModalPopup)
            placement.add$(Center);
        if (kind == MainWindow)
            placement.add$(Maximize);

        if (placement.get_id() == "") {
            //happens after kind change
            changed = true;
            placement = None;
        }
        fl = placement != Defaults().placement ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&placement, fl, ctx);
        ImGui::EndDisabled();
        break;
    }
    default:
        return false;
    }
    return changed;
}

std::vector<UINode::Prop>
TopWindow::Events()
{
    return {
        { "window.appearing", &onWindowAppearing },
        { "window.backButton", &onBackButton },
    };
}

bool TopWindow::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Appearing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent("Window_Appearing", &onWindowAppearing, 0, ctx);
        break;
    case 1:
        ImGui::Text("BackButton");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent("Window_BackButton", &onBackButton, 0, ctx);
        break;
    }
    return changed;
}
