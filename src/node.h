#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <sstream>
#include <iomanip>
#include <imgui.h>
#include "uicontext.h"
#include "binding.h"
#include "cpp_parser.h"
#include "imrad.h"
#include "IconsFontAwesome6.h"

struct Widget;

struct UINode
{
	struct Prop {
		std::string_view name;
		property_base* property;
		bool kbdInput = false; //this property accepts keyboard input by default
	};
	enum SnapOptions {
		SnapSides = 0x1,
		SnapInterior = 0x2,
		SnapGrandparentClip = 0x4,
		NoContextMenu = 0x8,
		NoOverlayPos = 0x10,
		HasSizeX = 0x20,
		HasSizeY = 0x40,
	};

	UINode() {}
	UINode(const UINode&) {} //shallow copy
	virtual ~UINode() {}
	virtual void Draw(UIContext& ctx) = 0;
	virtual void DrawExtra(UIContext& ctx) = 0;
	virtual void TreeUI(UIContext& ctx) = 0;
	virtual auto Properties()->std::vector<Prop> = 0;
	virtual auto Events()->std::vector<Prop> = 0;
	virtual bool PropertyUI(int, UIContext& ctx) = 0;
	virtual bool EventUI(int, UIContext& ctx) = 0;
	virtual void Export(std::ostream&, UIContext& ctx) = 0;
	virtual void Import(cpp::stmt_iterator& sit, UIContext& ctx) = 0;
	virtual int Behavior() = 0;

	void DrawInteriorRect(UIContext& ctx);
	void DrawSnap(UIContext& ctx);
	void RenameFieldVars(const std::string& oldn, const std::string& newn);
	auto FindChild(const UINode*) -> std::optional<std::pair<UINode*, int>>;
	auto FindInRect(const ImRect& r) -> std::vector<UINode*>;
	auto GetAllChildren() -> std::vector<UINode*>;
	void CloneChildrenFrom(const UINode& node, UIContext& ctx);
	void ResetLayout();
	auto GetParentId(UIContext& ctx) -> std::string;

	ImVec2 cached_pos;
	ImVec2 cached_size;
	std::vector<std::unique_ptr<Widget>> children;
	std::vector<ImRad::VBox> vbox;
	std::vector<ImRad::HBox> hbox;
};

struct Widget : UINode
{
	direct_val<bool> sameLine = false;
	direct_val<int> nextColumn = 0;
	bindable<dimension> size_x = 0;
	bindable<dimension> size_y = 0;
	bindable<bool> visible = true;
	bindable<bool> disabled = false;
	bindable<std::string> tooltip = "";
	direct_val<std::string> contextMenu = "";
	direct_val<int> cursor = ImGuiMouseCursor_Arrow;
	direct_val<bool> tabStop = true;
	direct_val<bool> hasPos = false;
	direct_val<dimension> pos_x = 0;
	direct_val<dimension> pos_y = 0;
	direct_val<int> indent = 0;
	direct_val<int> spacing = 0;
	direct_val<bool> allowOverlap = false;
	data_loop itemCount;
	bindable<font_name> style_font;
	bindable<color32> style_text;
	bindable<color32> style_frameBg;
	bindable<color32> style_border;
	bindable<color32> style_button;
	bindable<color32> style_buttonHovered;
	bindable<color32> style_buttonActive;
	direct_val<pzdimension> style_frameRounding;
	direct_val<pzdimension2> style_framePadding;
	direct_val<pzdimension> style_frameBorderSize;
	event<> onItemClicked;
	event<> onItemDoubleClicked;
	event<> onItemHovered;
	event<> onItemFocused;
	event<> onItemActivated;
	event<> onItemDeactivated;
	event<> onItemDeactivatedAfterEdit;
	event<> onItemContextMenuClicked;
	std::string userCodeBefore, userCodeAfter;

	struct Layout 
	{
		enum { Topmost = 0x1, Leftmost = 0x2, Bottommost = 0x4, HLayout = 0x10, VLayout = 0x20 };
		int flags = 0;
		int colId = 0;
		int rowId = 0;
	};

	static std::unique_ptr<Widget> Create(const std::string& s, UIContext& ctx);

	void Draw(UIContext& ctx);
	void DrawExtra(UIContext& ctx);
	void Export(std::ostream& os, UIContext& ctx);
	void Import(cpp::stmt_iterator& sit, UIContext& ctx);
	auto Properties() -> std::vector<Prop>;
	auto Events() -> std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void TreeUI(UIContext& ctx);
	bool EventUI(int, UIContext& ctx);
	int Behavior();
	Layout GetLayout(UINode* parent);
	virtual std::unique_ptr<Widget> Clone(UIContext& ctx) = 0;
	virtual ImDrawList* DoDraw(UIContext& ctx) = 0;
	virtual void DoDrawExtra(UIContext& ctx) {}
	virtual void DoExport(std::ostream& os, UIContext& ctx) = 0;
	virtual void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx) = 0;
	virtual void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	virtual const char* GetIcon() const { return ""; }
};

struct Spacer : Widget
{
	Spacer(UIContext&);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int, UIContext& ctx);
	void DoExport(std::ostream&, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
	const char* GetIcon() const { return ICON_FA_LEFT_RIGHT; }
};

struct Separator : Widget
{
	bindable<std::string> label;
	direct_val<pzdimension> style_thickness;
	direct_val<bool> style_outer_padding = true;

	Separator(UIContext&);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int, UIContext& ctx);
	void DoExport(std::ostream&, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_MINUS; }
};

struct Text : Widget
{
	bindable<std::string> text = "text";
	direct_val<bool> alignToFrame = false;
	direct_val<bool> wrap = false;
	
	Text(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_FONT; }
};

struct Selectable : Widget
{
	bindable<std::string> label = "label";
	flags_helper flags = ImGuiSelectableFlags_DontClosePopups;
	direct_val<ImRad::Alignment> horizAlignment = ImRad::AlignLeft;
	direct_val<ImRad::Alignment> vertAlignment = ImRad::AlignTop;
	direct_val<bool> alignToFrame = false;
	direct_val<bool> readOnly = false;
	field_ref<bool> fieldName;
	bindable<bool> selected = false;
	bindable<color32> style_header;
	event<> onChange;

	Selectable(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
	const char* GetIcon() const { return ICON_FA_AUDIO_DESCRIPTION; }
};

struct Button : Widget
{
	bindable<std::string> label = "OK";
	direct_val<ImGuiDir> arrowDir = ImGuiDir_None;
	direct_val<bool> small = false;
	direct_val<ImRad::ModalResult> modalResult = ImRad::None;
	direct_val<shortcut_> shortcut = "";
	direct_val<std::string> dropDownMenu = "";
	event<> onChange;

	Button(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events() ->std::vector<Prop>;
	bool EventUI(int, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
	const char* GetIcon() const { return ICON_FA_CIRCLE_PLAY; }
};

struct CheckBox : Widget
{
	bindable<std::string> label = "label";
	field_ref<bool> fieldName;
	bindable<color32> style_check;
	event<> onChange;

	CheckBox(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_SQUARE_CHECK; }
};

struct RadioButton : Widget
{
	bindable<std::string> label = "label";
	direct_val<int> valueID = 0;
	bindable<color32> style_check;
	field_ref<int> fieldName;
	event<> onChange;

	RadioButton(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_CIRCLE_DOT; }
};

struct Input : Widget
{
	field_ref<> fieldName;
	direct_val<std::string> label = "";
	direct_val<std::string> type = "std::string";
	bindable<std::string> hint = "";
	direct_val<int> imeType = ImRad::ImeText;
	direct_val<float> step = 1;
	direct_val<std::string> format = "%.3f";
	flags_helper flags = 0;
	direct_val<bool> initialFocus = false;
	field_ref<bool> forceFocus;
	event<> onChange;
	event<> onImeAction;

	static flags_helper _imeClass, _imeAction;
	
	Input(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior();
	const char* GetIcon() const { return "|a_|"; }
};

struct Combo : Widget
{
	direct_val<std::string> label = "";
	field_ref<std::string> fieldName;
	bindable<std::vector<std::string>> items;
	flags_helper flags = ImGuiComboFlags_None;
	event<> onChange;

	Combo(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior() { return Widget::Behavior() | HasSizeX; }
	const char* GetIcon() const { return ICON_FA_SQUARE_CARET_DOWN; }
};

struct Slider : Widget
{
	direct_val<std::string> label = "";
	field_ref<> fieldName;
	direct_val<std::string> type = "float";
	direct_val<float> min = 0;
	direct_val<float> max = 1;
	direct_val<std::string> format = "";
	event<> onChange;

	Slider(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior() { return Widget::Behavior() | HasSizeX; }
	const char* GetIcon() const { return ICON_FA_SLIDERS; }
};

struct ProgressBar : Widget
{
	direct_val<bool> indicator = true;
	field_ref<float> fieldName;
	bindable<color32> style_color;

	ProgressBar(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
	const char* GetIcon() const { return ICON_FA_BATTERY_HALF; }
};

struct ColorEdit : Widget
{
	field_ref<> fieldName;
	direct_val<std::string> label = "";
	direct_val<std::string> type = "color3";
	flags_helper flags = ImGuiColorEditFlags_None;
	event<> onChange;

	ColorEdit(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior() { return Widget::Behavior() | HasSizeX; }
	const char* GetIcon() const { return ICON_FA_CIRCLE_HALF_STROKE; }
};

struct Image : Widget
{
	bindable<std::string> fileName = "";
	field_ref<ImRad::Texture> fieldName;
	enum StretchPolicy { None, Scale, FitIn, FitOut };
	direct_val<StretchPolicy> stretchPolicy = Scale;
	
	ImRad::Texture tex;

	Image(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void RefreshTexture(UIContext& ctx);
	bool PickFileName(UIContext& ctx);
	int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
	const char* GetIcon() const { return ICON_FA_IMAGE; }
};

struct CustomWidget : Widget
{
	event<ImRad::CustomWidgetArgs> onDraw;
	
	CustomWidget(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	ImDrawList* DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	int Behavior() { return Widget::Behavior() | HasSizeX | HasSizeY; }
	const char* GetIcon() const { return ICON_FA_EXPAND; }
};

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

