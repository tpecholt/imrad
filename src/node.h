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

inline const std::string_view FOR_VAR = "i";


struct UIContext
{
	//set from outside
	bool snapMode = false;
	std::vector<UINode*> selected;
	CppGen* codeGen = nullptr;
	int importState = 0; //0 - no import, 1 - within begin/end/separator, 2 - user code import
	ImVec2 wpos;
	std::string fname;
	enum Color { Hovered, Selected, Snap1, Snap2, Snap3, Snap4, Snap5, COUNT };
	std::array<ImU32, Color::COUNT> colors;
	std::vector<std::string> fontNames;
	ImFont* defaultFont = nullptr;

	//snap result
	UINode* snapParent = nullptr;
	size_t snapIndex;
	bool snapSameLine;
	bool snapNextColumn;
	bool snapBeginGroup;
	bool snapSetNextSameLine;
	
	//recursive info
	bool selUpdated;
	UINode* hovered = nullptr;
	int groupLevel = 0;
	int importLevel;
	std::string userCode;
	UINode* root = nullptr;
	ImGuiWindow* rootWin = nullptr;
	std::vector<ImGuiWindow*> popupWins;
	std::vector<UINode*> parents;
	bool inPopup = false;
	bool inTable = false;
	std::string ind;
	std::vector<std::string> errors;

	//convenience
	void ind_up();
	void ind_down();
};

struct Widget;
using children_t = std::vector<std::unique_ptr<Widget>>;

struct UINode
{
	struct Prop {
		const char* name;
		property_base* property;
		bool kbdInput = false; //this property accepts keyboard input by default
	};
	enum SnapOptions {
		SnapSides = 0x1,
		SnapInterior = 0x2,
		SnapGrandparentClip = 0x4,
	};

	virtual ~UINode() {}
	virtual void Draw(UIContext& ctx) = 0;
	virtual void TreeUI(UIContext& ctx) = 0;
	virtual auto Properties()->std::vector<Prop> = 0;
	virtual auto Events()->std::vector<Prop> = 0;
	virtual bool PropertyUI(int, UIContext& ctx) = 0;
	virtual bool EventUI(int, UIContext& ctx) = 0;
	virtual void Export(std::ostream&, UIContext& ctx) = 0;
	virtual void Import(cpp::stmt_iterator& sit, UIContext& ctx) = 0;
	virtual int SnapBehavior() { return SnapSides; }

	void DrawSnap(UIContext& ctx);
	void RenameFieldVars(const std::string& oldn, const std::string& newn);
	bool FindChild(const UINode*);

	ImVec2 cached_pos;
	ImVec2 cached_size;
	children_t children;
};

struct Widget : UINode
{
	direct_val<bool> sameLine = false;
	direct_val<bool> beginGroup = false;
	direct_val<bool> endGroup = false;
	direct_val<bool> nextColumn = false;
	bindable<bool> visible = true;
	bindable<bool> disabled = false;
	direct_val<int> indent = 0;
	direct_val<int> spacing = 0;
	bindable<std::string> tooltip = "";
	direct_val<int> cursor = ImGuiMouseCursor_Arrow;
	direct_val<std::string> style_font = "";
	parent_property style;
	event<> onItemClicked;
	event<> onItemDoubleClicked;
	event<> onItemHovered;
	event<> onItemFocused;
	event<> onItemActivated;
	event<> onItemDeactivated;
	event<> onItemDeactivatedAfterEdit;

	std::string userCode;

	static std::unique_ptr<Widget> Create(const std::string& s, UIContext& ctx);

	void Draw(UIContext& ctx);
	void Export(std::ostream& os, UIContext& ctx);
	void Import(cpp::stmt_iterator& sit, UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	auto Events() -> std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void TreeUI(UIContext& ctx);
	bool EventUI(int, UIContext& ctx);
	//virtual void DrawSnap(UIContext& ctx);
	virtual void DoDraw(UIContext& ctx) = 0;
	virtual void DrawExtra(UIContext& ctx) {}
	virtual void DoExport(std::ostream& os, UIContext& ctx) = 0;
	virtual void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx) = 0;
	virtual void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	virtual const char* GetIcon() const { return ""; }
};

struct Separator : Widget
{
	Separator(UIContext&);
	void DoDraw(UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
	bool PropertyUI(int, UIContext& ctx);
	void DoExport(std::ostream&, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_WINDOW_MINIMIZE; }// "-- "; }
};

struct Text : Widget
{
	bindable<std::string> text = "text";
	direct_val<bool> grayed = false; //Widget::disabled is already bindable
	bindable<color32> style_color;
	direct_val<bool> alignToFrame = false;
	direct_val<bool> wrap = false;
	
	Text(UIContext& ctx);
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
	bindable<color32> style_color;
	flags_helper flags = ImGuiSelectableFlags_DontClosePopups;
	direct_val<ImRad::Alignment> horizAlignment = ImRad::AlignLeft;
	direct_val<ImRad::Alignment> vertAlignment = ImRad::AlignTop;
	direct_val<bool> alignToFrame = false;
	field_ref<bool> fieldName;
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	event<> onChange;

	Selectable(UIContext& ctx);
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
	bindable<std::string> label = "\xef\x80\x82 Search";
	direct_val<ImGuiDir> arrowDir = ImGuiDir_None;
	direct_val<bool> small = false;
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	direct_val<ImRad::ModalResult> modalResult = ImRad::None;
	direct_val<std::string> shortcut = "";
	bindable<color32> style_text;
	bindable<color32> style_button;
	bindable<color32> style_hovered;
	direct_val<float> style_rounding = 0;
	event<> onChange;

	Button(UIContext& ctx);
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
	bindable<color32> style_color;
	event<> onChange;

	CheckBox(UIContext& ctx);
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
	bindable<color32> style_color;
	field_ref<int> fieldName;

	RadioButton(UIContext& ctx);
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
	direct_val<float> step = 1;
	direct_val<std::string> format = "%.3f";
	bindable<float> size_x = 200;
	bindable<float> size_y = 100;
	flags_helper flags = 0;
	direct_val<bool> keyboardFocus = false;
	event<> onChange;

	Input(UIContext& ctx);
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
	bindable<float> size_x = 200;
	event<> onChange;

	Combo(UIContext& ctx);
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
	bindable<float> size_x = 200;
	event<> onChange;

	Slider(UIContext& ctx);
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
	bindable<float> size_x = 200;
	bindable<float> size_y = 0;
	
	ProgressBar(UIContext& ctx);
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
	bindable<float> size_x = 200;
	event<> onChange;

	ColorEdit(UIContext& ctx);
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
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	field_ref<ImRad::Texture> fieldName;
	ImRad::Texture tex;

	Image(UIContext& ctx);
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
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	event<ImRad::CustomWidgetArgs> onDraw;
	
	CustomWidget(UIContext& ctx);
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
	};
	flags_helper flags = ImGuiTableFlags_Borders;
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	std::vector<ColumnData> columnData;
	direct_val<bool> header = true;
	field_ref<size_t> rowCount;
	bindable<bool> rowFilter;
	direct_val<bool> style_padding = true;
	bindable<color32> style_headerBg;
	bindable<color32> style_rowBg;
	bindable<color32> style_rowBgAlt;

	Table(UIContext&);
	int SnapBehavior() { return SnapSides | SnapInterior; }
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_TABLE_CELLS_LARGE; }
};

struct Child : Widget
{
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	direct_val<bool> border = false;
	bindable<int> columnCount = 1;
	direct_val<bool> columnBorder = true;
	std::vector<bindable<float>> columnsWidths{ 0.f };
	field_ref<size_t> data_size;
	direct_val<bool> style_padding = true;
	bindable<color32> style_bg;
	parent_property style{ &style_bg, &style_padding };

	Child(UIContext& ctx);
	int SnapBehavior() { return SnapSides | SnapInterior; }
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	const char* GetIcon() const { return ICON_FA_SQUARE_FULL; }
};

struct CollapsingHeader : Widget
{
	bindable<std::string> label = "label";
	bindable<bool> open = true;
	
	CollapsingHeader(UIContext& ctx);
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
	field_ref<size_t> tabCount;
	field_ref<int> tabIndex;

	TabBar(UIContext& ctx);
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

	TabItem(UIContext& ctx);
	int SnapBehavior() { return SnapInterior | SnapGrandparentClip; }
	void DoDraw(UIContext& ctx);
	void DrawExtra(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
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
	int SnapBehavior() { return 0; }
	void DoDraw(UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 p1, UIContext& ctx);
};

struct MenuIt : Widget
{
	direct_val<std::string> label = "Item";
	direct_val<std::string> shortcut = "";
	direct_val<bool> separator = false;
	field_ref<bool> checked;
	event<> onChange;

	MenuIt(UIContext& ctx);
	int SnapBehavior() { return 0; }
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

struct TopWindow : UINode
{
	enum Kind { Window, Popup, ModalPopup };

	flags_helper flags = ImGuiWindowFlags_NoCollapse;
	Kind kind = Window;
	bindable<std::string> title = "title";
	float size_x = 640;
	float size_y = 480;
	std::string style_font = "";
	std::optional<ImVec2> style_pading;
	std::optional<ImVec2> style_spacing;
	parent_property style;

	TopWindow(UIContext& ctx);
	void Draw(UIContext& ctx);
	void TreeUI(UIContext& ctx);
	bool EventUI(int, UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	auto Events() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void Export(std::ostream& os, UIContext& ctx);
	void Import(cpp::stmt_iterator& sit, UIContext& ctx);
	int SnapBehavior() { return SnapInterior; }
};

