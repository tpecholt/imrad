#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <sstream>
#include <iomanip>
#include <imgui.h>
#include "binding.h"

struct TopWindow : UINode
{
	enum Kind { MainWindow, Window, Popup, ModalPopup, Activity };
	enum Placement { None, Left, Right, Top, Bottom, Center, Maximize };
	
	flags_helper flags = ImGuiWindowFlags_NoCollapse;
	Kind kind = Window;
	bindable<std::string> title = "title";
	bindable<dimension> size_x = 640.f;
	bindable<dimension> size_y = 480.f;
	bindable<font_name> style_font;
	direct_val<pzdimension2> style_padding;
	direct_val<pzdimension2> style_spacing;
	direct_val<pzdimension> style_border;
	direct_val<pzdimension> style_rounding;
	direct_val<pzdimension> style_scrollbarSize;
	bindable<color32> style_bg;
	bindable<color32> style_menuBg;
	direct_val<Placement> placement = None;
	direct_val<bool> animate = false;

	TopWindow(UIContext& ctx);
	void Draw(UIContext& ctx);
	void TreeUI(UIContext& ctx);
	bool EventUI(int, UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	auto Events() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void Export(std::ostream& os, UIContext& ctx);
	void Import(cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior() { return SnapInterior; }
	void ScaleDimensions(float);
};

