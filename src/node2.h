#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <sstream>
#include <iomanip>
#include <imgui.h>
#include "node.h"

struct Table : Widget
{
	struct ColumnData 
	{
		std::string label;
		flags_helper flags = 0;
		float width = 0;
		ColumnData();
		ColumnData(std::string_view l, int f, float w = 0) : ColumnData() {
			label = l; flags = f; width = w; 
		}
	};
	flags_helper flags = ImGuiTableFlags_Borders;
	std::vector<ColumnData> columnData;
	direct_val<bool> header = true;
	bindable<bool> rowFilter;
	bindable<dimension> rowHeight = 0;
	direct_val<bool> scrollWhenDragging = false;
	direct_val<pzdimension2> style_cellPadding;
	bindable<color32> style_headerBg;
	bindable<color32> style_rowBg;
	bindable<color32> style_rowBgAlt;
	bindable<color32> style_childBg;
	event<> onBeginRow;
	event<> onEndRow;

	Table(UIContext&);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int Behavior() { return Widget::Behavior() | SnapInterior | HasSizeX | HasSizeY; }
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_TABLE_CELLS_LARGE; }
};

struct Child : Widget
{
	flags_helper flags = ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened;
	flags_helper wflags = ImGuiWindowFlags_NoSavedSettings;
	bindable<int> columnCount = 1;
	direct_val<bool> columnBorder = true;
	direct_val<bool> scrollWhenDragging = false;
	direct_val<pzdimension2> style_padding;
	direct_val<pzdimension2> style_spacing;
	direct_val<bool> style_outer_padding = true;
	direct_val<pzdimension> style_rounding;
	direct_val<pzdimension> style_borderSize;
	bindable<color32> style_bg;
	
	Child(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int Behavior();
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_SQUARE_FULL; }
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
};

struct CollapsingHeader : Widget
{
	bindable<std::string> label = "label";
	bindable<bool> open = true;
	bindable<color32> style_header;
	bindable<color32> style_hovered;
	bindable<color32> style_active;

	CollapsingHeader(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int Behavior() { return SnapSides | SnapInterior; }
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_ARROW_DOWN_WIDE_SHORT; }
};

struct TabBar : Widget
{
	flags_helper flags = 0;
	field_ref<int> activeTab;
	bindable<color32> style_tab;
	bindable<color32> style_tabDimmed;
	bindable<color32> style_hovered;
	bindable<color32> style_selected;
	bindable<color32> style_dimmedSelected;
	bindable<color32> style_overline;
	direct_val<bool> style_regularWidth = false;

	TabBar(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int Behavior() { return NoOverlayPos | SnapSides; }
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_FOLDER_CLOSED; }
	float CalcRegularWidth();
};

struct TabItem : Widget
{
	bindable<std::string> label = "TabItem";
	direct_val<bool> closeButton = false;
	event<> onClose;

	TabItem(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int Behavior() { return SnapInterior | SnapGrandparentClip | NoOverlayPos; }
	ImDrawList* DoDraw(UIContext& ctx);
	void DoDrawExtra(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_FOLDER; }
};

struct TreeNode : Widget
{
	flags_helper flags = ImGuiTreeNodeFlags_None;
	bindable<std::string> label = "Node";
	bindable<bool> open = true;
	bool lastOpen;

	TreeNode(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int Behavior() { return Widget::Behavior() | SnapInterior; }
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_SITEMAP; }
};

struct MenuBar : Widget
{
	MenuBar(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int Behavior() { return NoOverlayPos; }
	ImDrawList* DoDraw(UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_ELLIPSIS; }
};

struct MenuIt : Widget
{
	bool contextMenu = false;
	direct_val<bool> ownerDraw = false;
	direct_val<std::string> label = "Item";
	direct_val<shortcut_> shortcut = "";
	direct_val<bool> separator = false;
	field_ref<bool> checked;
	direct_val<pzdimension2> style_padding;
	direct_val<pzdimension2> style_spacing;
	direct_val<pzdimension> style_rounding;
	event<> onChange;

	MenuIt(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int Behavior() { return NoOverlayPos | NoContextMenu; }
	ImDrawList* DoDraw(UIContext& ctx);
	void DoDrawExtra(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void ExportShortcut(std::ostream& os, UIContext& ctx);
	void ExportAllShortcuts(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return contextMenu ? ICON_FA_MESSAGE : ICON_FA_BARS; }
};

struct Splitter : Widget
{
	direct_val<dimension> min_size1 = 10;
	direct_val<dimension> min_size2 = 10;
	field_ref<dimension> position;
	bindable<color32> style_active;
	bindable<color32> style_bg;
	
	Splitter(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int Behavior() { return Widget::Behavior() | SnapInterior | HasSizeX | HasSizeY | NoOverlayPos; }
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	//void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_TABLE_COLUMNS; }
};

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
	direct_val<pzdimension> style_borderSize;
	direct_val<pzdimension> style_rounding;
	direct_val<pzdimension> style_scrollbarSize;
	direct_val<pzdimension2> style_titlePadding;
	bindable<color32> style_bg;
	bindable<color32> style_menuBg;
	direct_val<Placement> placement = None;
	direct_val<bool> closeOnEscape = false;
	direct_val<bool> animate = false;
	direct_val<bool> initialActivity = false;

	event<> onBackButton;
	event<> onWindowAppearing;

	std::string userCodeBefore, userCodeAfter, userCodeMid;

	TopWindow(UIContext& ctx);
	void Draw(UIContext& ctx);
	void DrawExtra(UIContext& ctx) {}
	void TreeUI(UIContext& ctx);
	bool EventUI(int, UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	auto Events() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void Export(std::ostream& os, UIContext& ctx);
	void Import(cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior() { return SnapInterior; }
};

