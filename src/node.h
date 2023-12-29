#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <sstream>
#include <iomanip>
#include <imgui.h>
#include "binding.h"
#include "cpp_parser.h"
#include "imrad.h"
#include "IconsFontAwesome6.h"

struct UINode;
class CppGen;


struct UIContext
{
	//set from outside
	enum Mode { NormalSelection, RectSelection, Snap, PickPoint, ItemDragging };
	Mode mode = NormalSelection;
	std::vector<UINode*> selected;
	CppGen* codeGen = nullptr;
	ImVec2 wpos, wpos2;
	std::string workingDir;
	enum Color { Hovered, Selected, Snap1, Snap2, Snap3, Snap4, Snap5, COUNT };
	std::array<ImU32, Color::COUNT> colors;
	std::vector<std::string> fontNames;
	ImFont* defaultFont = nullptr;
	std::string unit; //for dimension export
	bool createVars = true; //create variables etc. during contructor/Clone calls
	ImGuiStyle style;

	//snap result
	UINode* snapParent = nullptr;
	size_t snapIndex;
	bool snapSameLine;
	int snapNextColumn;
	bool snapBeginGroup;
	bool snapSetNextSameLine;
	bool snapClearNextNextColumn;
	
	//recursive info
	int importState = 0; //0 - no import, 1 - within begin/end/separator, 2 - user code import
	bool selUpdated;
	UINode* hovered = nullptr;
	UINode* dragged = nullptr;
	int groupLevel = 0;
	int importLevel;
	std::string userCode;
	UINode* root = nullptr;
	ImGuiWindow* rootWin = nullptr;
	std::vector<ImGuiWindow*> activePopups;
	std::vector<UINode*> parents;
	std::vector<std::string> contextMenus;
	int kind = 0;
	float unitFactor = 1; //for dimension value scaling
	ImVec2 selStart, selEnd;
	std::string ind;
	int varCounter;
	std::vector<std::string> errors;

	//convenience
	void ind_up();
	void ind_down();
};

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
	};

	UINode() {}
	UINode(const UINode&) {} //shallow copy
	virtual ~UINode() {}
	virtual void Draw(UIContext& ctx) = 0;
	virtual void TreeUI(UIContext& ctx) = 0;
	virtual auto Properties()->std::vector<Prop> = 0;
	virtual auto Events()->std::vector<Prop> = 0;
	virtual bool PropertyUI(int, UIContext& ctx) = 0;
	virtual bool EventUI(int, UIContext& ctx) = 0;
	virtual void Export(std::ostream&, UIContext& ctx) = 0;
	virtual void Import(cpp::stmt_iterator& sit, UIContext& ctx) = 0;
	virtual int SnapBehavior() = 0;

	void DrawInteriorRect(UIContext& ctx);
	void DrawSnap(UIContext& ctx);
	void RenameFieldVars(const std::string& oldn, const std::string& newn);
	void ScaleDimensions(float scale);
	auto FindChild(const UINode*) -> std::optional<std::pair<UINode*, int>>;
	auto FindInRect(const ImRect& r) -> std::vector<UINode*>;
	auto GetAllChildren() -> std::vector<UINode*>;
	void CloneChildrenFrom(const UINode& node, UIContext& ctx);

	ImVec2 cached_pos;
	ImVec2 cached_size;
	std::vector<std::unique_ptr<Widget>> children;
};

struct Widget : UINode
{
	direct_val<bool> sameLine = false;
	direct_val<bool> beginGroup = false;
	direct_val<bool> endGroup = false;
	direct_val<int> nextColumn = 0;
	direct_val<bool> hasPos = false;
	direct_val<dimension> pos_x = 0;
	direct_val<dimension> pos_y = 0;
	direct_val<bool> allowOverlap = false;
	bindable<bool> visible = true;
	bindable<bool> disabled = false;
	direct_val<int> indent = 0;
	direct_val<int> spacing = 0;
	bindable<std::string> tooltip = "";
	direct_val<int> cursor = ImGuiMouseCursor_Arrow;
	direct_val<std::string> style_font = "";
	bindable<color32> style_text;
	bindable<color32> style_frameBg;
	direct_val<pzdimension> style_frameRounding;
	direct_val<std::string> contextMenu = "";
	event<> onItemClicked;
	event<> onItemDoubleClicked;
	event<> onItemHovered;
	event<> onItemFocused;
	event<> onItemActivated;
	event<> onItemDeactivated;
	event<> onItemDeactivatedAfterEdit;
	event<> onItemContextMenuClicked;

	std::string userCode;

	static std::unique_ptr<Widget> Create(const std::string& s, UIContext& ctx);

	void InitDimensions(UIContext& ctx);
	void Draw(UIContext& ctx);
	void Export(std::ostream& os, UIContext& ctx);
	void Import(cpp::stmt_iterator& sit, UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	auto Events() -> std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void TreeUI(UIContext& ctx);
	bool EventUI(int, UIContext& ctx);
	int SnapBehavior();
	virtual std::unique_ptr<Widget> Clone(UIContext& ctx) = 0;
	virtual void DoDraw(UIContext& ctx) = 0;
	virtual void DrawExtra(UIContext& ctx) {}
	virtual void DoExport(std::ostream& os, UIContext& ctx) = 0;
	virtual void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx) = 0;
	virtual void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	virtual const char* GetIcon() const { return ""; }
};

struct Separator : Widget
{
	bindable<std::string> label;
	direct_val<pzdimension> style_thickness;
	direct_val<bool> style_outer_padding = true;

	Separator(UIContext&);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int, UIContext& ctx);
	void DoExport(std::ostream&, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_WINDOW_MINIMIZE; }// "-- "; }
	void ScaleDimensions(float scale);
};

struct Text : Widget
{
	bindable<std::string> text = "text";
	direct_val<bool> alignToFrame = false;
	direct_val<bool> wrap = false;
	
	Text(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
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
	bindable<dimension> size_x = 0.f;
	bindable<dimension> size_y = 0.f;
	event<> onChange;

	Selectable(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_AUDIO_DESCRIPTION; }
};

struct Button : Widget
{
	bindable<std::string> label = "OK";
	direct_val<ImGuiDir> arrowDir = ImGuiDir_None;
	direct_val<bool> small = false;
	bindable<dimension> size_x = 0.f;
	bindable<dimension> size_y = 0.f;
	direct_val<ImRad::ModalResult> modalResult = ImRad::None;
	direct_val<std::string> shortcut = "";
	bindable<color32> style_button;
	bindable<color32> style_hovered;
	event<> onChange;

	Button(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events() ->std::vector<Prop>;
	bool EventUI(int, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_CIRCLE_PLAY; }
};

struct CheckBox : Widget
{
	bindable<std::string> label = "label";
	field_ref<bool> fieldName;
	event<> onChange;

	CheckBox(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
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
	field_ref<int> fieldName;

	RadioButton(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_CIRCLE_DOT; }
};

struct Input : Widget
{
	field_ref<> fieldName;
	direct_val<std::string> label = "";
	direct_val<std::string> type = "std::string";
	direct_val<std::string> hint = "";
	direct_val<int> imeType = ImRad::ImeText;
	direct_val<float> step = 1;
	direct_val<std::string> format = "%.3f";
	bindable<dimension> size_x = 200.f;
	bindable<dimension> size_y = 100.f;
	flags_helper flags = 0;
	direct_val<bool> initialFocus = false;
	field_ref<bool> forceFocus;
	event<> onChange;
	event<> onImeAction;

	static flags_helper _imeClass, _imeAction;
	
	Input(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return "[ab]"; }
};

struct Combo : Widget
{
	direct_val<std::string> label = "";
	field_ref<int> fieldName;
	bindable<std::vector<std::string>> items;
	bindable<dimension> size_x = 200.f;
	event<> onChange;

	Combo(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
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
	bindable<dimension> size_x = 200.f;
	event<> onChange;

	Slider(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_SLIDERS; }
};

struct ProgressBar : Widget
{
	direct_val<bool> indicator = true;
	field_ref<float> fieldName;
	bindable<dimension> size_x = 200.f;
	bindable<dimension> size_y = 0.f;
	bindable<color32> style_color;

	ProgressBar(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_BATTERY_HALF; }
};

struct ColorEdit : Widget
{
	field_ref<> fieldName;
	direct_val<std::string> label = "";
	direct_val<std::string> type = "color3";
	flags_helper flags = ImGuiColorEditFlags_None;
	bindable<dimension> size_x = 200.f;
	event<> onChange;

	ColorEdit(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_CIRCLE_HALF_STROKE; }
};

struct Image : Widget
{
	bindable<std::string> fileName = "";
	bindable<dimension> size_x = 0.f;
	bindable<dimension> size_y = 0.f;
	field_ref<ImRad::Texture> fieldName;
	ImRad::Texture tex;

	Image(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void RefreshTexture(UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_IMAGE; }
};

struct CustomWidget : Widget
{
	bindable<dimension> size_x = 0.f;
	bindable<dimension> size_y = 0.f;
	event<ImRad::CustomWidgetArgs> onDraw;
	
	CustomWidget(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	void DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
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
	bindable<dimension> size_x = 0.f;
	bindable<dimension> size_y = 0.f;
	std::vector<ColumnData> columnData;
	direct_val<bool> header = true;
	data_loop rowCount;
	bindable<bool> rowFilter;
	bindable<dimension> rowHeight = 0;
	direct_val<bool> scrollWhenDragging = false;
	direct_val<pzdimension2> style_cellPadding;
	bindable<color32> style_headerBg;
	bindable<color32> style_rowBg;
	bindable<color32> style_rowBgAlt;
	event<> onBeginRow;
	event<> onEndRow;

	Table(UIContext&);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int SnapBehavior() { return SnapSides | SnapInterior; }
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_TABLE_CELLS_LARGE; }
	void ScaleDimensions(float scale);
};

struct Child : Widget
{
	flags_helper flags = ImGuiChildFlags_AlwaysUseWindowPadding;
	flags_helper wflags = ImGuiWindowFlags_NoSavedSettings;
	bindable<dimension> size_x = 20.f; //zero size will be rendered wrongly
	bindable<dimension> size_y = 20.f;
	bindable<int> columnCount = 1;
	direct_val<bool> columnBorder = true;
	data_loop itemCount;
	direct_val<bool> scrollWhenDragging = false;
	direct_val<pzdimension2> style_padding;
	direct_val<pzdimension2> style_spacing;
	direct_val<bool> style_outer_padding = true;
	direct_val<pzdimension> style_rounding;
	bindable<color32> style_bg;
	bindable<color32> style_border;

	Child(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int SnapBehavior() { return SnapSides | SnapInterior; }
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_SQUARE_FULL; }
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	bool IsFirstItem(UIContext& ctx);
};

struct CollapsingHeader : Widget
{
	bindable<std::string> label = "label";
	bindable<bool> open = true;
	
	CollapsingHeader(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int SnapBehavior() { return SnapSides | SnapInterior; }
	void DoDraw(UIContext& ctx);
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
	data_loop tabCount;
	field_ref<int> activeTab;

	TabBar(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int SnapBehavior() { return SnapSides; }
	void DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
};

struct TabItem : Widget
{
	bindable<std::string> label = "TabItem";
	direct_val<bool> closeButton = false;
	event<> onClose;

	TabItem(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int SnapBehavior() { return SnapInterior | SnapGrandparentClip; }
	void DoDraw(UIContext& ctx);
	void DrawExtra(UIContext& ctx);
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
	int SnapBehavior() { return SnapInterior | SnapSides; }
	void DoDraw(UIContext& ctx);
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
	int SnapBehavior() { return NoOverlayPos; }
	void DoDraw(UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
};

struct MenuIt : Widget
{
	bool contextMenu = false;
	direct_val<bool> ownerDraw = false;
	direct_val<std::string> label = "Item";
	direct_val<std::string> shortcut = "";
	direct_val<bool> separator = false;
	field_ref<bool> checked;
	event<> onChange;

	MenuIt(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int SnapBehavior() { return NoContextMenu | NoOverlayPos; }
	void DoDraw(UIContext& ctx);
	void DrawExtra(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void ExportShortcut(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_LIST; }
};

struct Splitter : Widget
{
	direct_val<dimension> min_size1 = 10;
	direct_val<dimension> min_size2 = 10;
	field_ref<dimension> position;
	bindable<dimension> size_x = -1;
	bindable<dimension> size_y = -1;
	bindable<color32> style_active;
	bindable<color32> style_bg;
	
	Splitter(UIContext& ctx);
	auto Clone(UIContext& ctx)->std::unique_ptr<Widget>;
	int SnapBehavior() { return SnapInterior | SnapSides; }
	void DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	//void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_ARROWS_LEFT_RIGHT_TO_LINE; }
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
	direct_val<std::string> style_font = "";
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
	int SnapBehavior() { return SnapInterior | NoOverlayPos; }
	void ScaleDimensions(float);
};

