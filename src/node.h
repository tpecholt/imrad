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

	//snap result
	UINode* snapParent = nullptr;
	size_t snapIndex;
	bool snapSameLine[2];
	bool snapNextColumn[2];
	bool snapBeginGroup[2];
	
	//recursive info
	UINode* hovered = nullptr;
	int level = 0;
	int groupLevel = 0;
	int importLevel;
	std::string userCode;
	UINode* root = nullptr;
	UINode* parent = nullptr;
	bool modalPopup = false;
	bool table = false;
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

	void RenameFieldVars(const std::string& oldn, const std::string& newn);
	
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
	direct_val<std::string> tooltip = "";
	direct_val<int> cursor = ImGuiMouseCursor_Arrow;
	event<> onItemClicked;
	event<> onItemDoubleClicked;
	event<> onItemHovered;
	event<> onItemFocused;
	event<> onItemActivated;
	event<> onItemDeactivated;
	event<> onItemDeactivatedAfterEdit;

	std::string userCode;

	static std::unique_ptr<Widget> Create(const std::string& s, UIContext& ctx);

	Widget(UIContext& ctx);
	void Draw(UIContext& ctx);
	void Export(std::ostream& os, UIContext& ctx);
	void Import(cpp::stmt_iterator& sit, UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	auto Events() -> std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void TreeUI(UIContext& ctx);
	bool EventUI(int, UIContext& ctx);
	virtual bool IsContainer() { return false; }
	virtual void DrawSnap(UIContext& ctx);
	virtual void DoDraw(UIContext& ctx) = 0;
	virtual void DoExport(std::ostream& os, UIContext& ctx) = 0;
	virtual void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx) = 0;
	virtual void CalcSizeEx(ImVec2 x1);
};

struct Separator : Widget
{
	void DoDraw(UIContext& ctx);
	bool PropertyUI(int, UIContext& ctx);
	void DoExport(std::ostream&, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
};

struct Text : Widget
{
	bindable<std::string> text = "text";
	direct_val<bool> grayed = false; //Widget::disabled is already bindable
	bindable<color32> color;
	direct_val<std::string> alignment = "Left"; 
	direct_val<bool> alignToFrame = false;
	bindable<float> size_x = 0;

	Text(UIContext& ctx);
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 x1);
};

struct Selectable : Widget
{
	bindable<std::string> label = "label";
	bindable<color32> color;
	flags_helper flags = ImGuiSelectableFlags_DontClosePopups;
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
};

struct Button : Widget
{
	bindable<std::string> label = "\xef\x80\x82 Search";
	direct_val<ImGuiDir> arrowDir = ImGuiDir_None;
	direct_val<bool> small = false;
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	bindable<color32> color;
	direct_val<ImRad::ModalResult> modalResult = ImRad::None;
	event<> onChange;

	Button(UIContext& ctx);
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events() ->std::vector<Prop>;
	bool EventUI(int, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
};

struct CheckBox : Widget
{
	bindable<std::string> label = "label";
	direct_val<bool> init_value = false;
	field_ref<bool> field_name;
	event<> onChange;

	CheckBox(UIContext& ctx);
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
};

struct RadioButton : Widget
{
	bindable<std::string> label = "label";
	direct_val<int> valueID = 0;
	field_ref<int> field_name;

	RadioButton(UIContext& ctx);
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
};

struct Input : Widget
{
	field_ref<> field_name;
	direct_val<std::string> type = "std::string";
	direct_val<std::string> hint = "";
	bindable<float> size_x = 200;
	bindable<float> size_y = 100;
	flags_helper flags = 0;
	direct_val<bool> keyboard_focus = false;
	event<> onChange;

	Input(UIContext& ctx);
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	auto Events()->std::vector<Prop>;
	bool EventUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
};

struct Combo : Widget
{
	field_ref<int> field_name;
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
};

struct Image : Widget
{
	bindable<std::string> file_name = "";
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	field_ref<ImRad::Texture> field_name;
	ImRad::Texture tex;

	Image(UIContext& ctx);
	void DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void RefreshTexture(UIContext& ctx);
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
	direct_val<bool> stylePadding = true;
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	std::vector<ColumnData> columnData;
	direct_val<bool> header = true;
	field_ref<size_t> row_count;

	Table(UIContext&);
	bool IsContainer() { return true; }
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
};

struct Child : Widget
{
	bindable<float> size_x = 0;
	bindable<float> size_y = 0;
	direct_val<bool> border = false;
	direct_val<bool> stylePadding = true;
	bindable<int> column_count = 1;
	direct_val<bool> column_border = true;
	std::vector<bindable<float>> columnsWidths{ 0.f };
	field_ref<size_t> data_size;
	bindable<color32> styleBg;

	Child(UIContext& ctx);
	bool IsContainer() { return true; }
	void DoDraw(UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
};

struct CollapsingHeader : Widget
{
	bindable<std::string> label = "label";
	bindable<bool> open = true;
	
	CollapsingHeader(UIContext& ctx);
	bool IsContainer() { return true; }
	void DoDraw(UIContext& ctx);
	auto Properties()->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void DoExport(std::ostream& os, UIContext& ctx);
	void DoImport(const cpp::stmt_iterator& sit, UIContext& ctx);
	void CalcSizeEx(ImVec2 x1);
};

struct TopWindow : UINode
{
	flags_helper flags = ImGuiWindowFlags_NoCollapse;
	bool modalPopup = false;
	bindable<std::string> title = "title";
	float size_x = 640;
	float size_y = 480;
	std::optional<ImVec2> stylePading;
	std::optional<ImVec2> styleSpacing;

	TopWindow(UIContext& ctx);
	void Draw(UIContext& ctx);
	void TreeUI(UIContext& ctx);
	bool EventUI(int, UIContext& ctx);
	auto Properties() ->std::vector<Prop>;
	auto Events() ->std::vector<Prop>;
	bool PropertyUI(int i, UIContext& ctx);
	void Export(std::ostream& os, UIContext& ctx);
	void Import(cpp::stmt_iterator& sit, UIContext& ctx);
};

