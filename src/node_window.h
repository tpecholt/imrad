#pragma once
#include "node_standard.h"

struct TopWindow : UINode
{
    enum Kind { MainWindow, Window, Popup, ModalPopup, Activity };
    enum Placement { None, Left, Right, Top, Bottom, Center, Maximize };

    direct_val<ImGuiWindowFlags_> flags = 0;
    direct_val<Kind> kind = Window;
    bindable<std::string> title = "title";
    bindable<dimension_t> size_x = 640.f;
    bindable<dimension_t> size_y = 480.f;
    bindable<dimension_t> minSize_x = 0;
    bindable<dimension_t> minSize_y = 0;
    bindable<font_name_t> style_fontName;
    bindable<float> style_fontSize; //scaled by style.FontScaleDpi
    direct_val<pzdimension2_t> style_padding;
    direct_val<pzdimension2_t> style_spacing;
    direct_val<pzdimension2_t> style_innerSpacing;
    direct_val<pzdimension_t> style_borderSize;
    direct_val<pzdimension_t> style_rounding;
    direct_val<pzdimension_t> style_scrollbarSize;
    direct_val<pzdimension2_t> style_titlePadding;
    direct_val<pzdimension2_t> style_placementGap;
    bindable<color_t> style_bg;
    bindable<color_t> style_menuBg;
    direct_val<Placement> placement = None;
    direct_val<bool> closeOnEscape = false;
    direct_val<bool> animate = false;
    direct_val<bool> initialActivity = false;

    event<> onBackButton;
    event<> onWindowAppearing;

    std::string userCodeBefore, userCodeAfter, userCodeMid;

    TopWindow(UIContext& ctx);
    void Draw(UIContext& ctx);
    void DrawTools(UIContext& ctx) {}
    void TreeUI(UIContext& ctx);
    bool EventUI(int, UIContext& ctx);
    auto Properties() ->std::vector<Prop>;
    auto Events() ->std::vector<Prop>;
    bool PropertyUI(int i, UIContext& ctx);
    void Export(std::ostream& os, UIContext& ctx);
    void Import(cpp::stmt_iterator& sit, UIContext& ctx);
    int Behavior() { return SnapInterior; }
    int ColumnCount(UIContext& ctx) { return 0; }
    std::string GetTypeName() { return "Window"; }
    const TopWindow& Defaults() { static TopWindow node(UIContext::Defaults()); return node; }
};

