#pragma once
#include <vector>
#include <array>
#include <string>
#include <imgui.h>

struct UINode;
class CppGen;
struct property_base;
struct ImGuiWindow;

struct UIContext
{
	//set from outside
	enum Mode { 
		NormalSelection, RectSelection, Snap, PickPoint, 
		ItemDragging, ItemSizingX, ItemSizingY, ItemSizingXY 
	};
	Mode mode = NormalSelection;
	std::vector<UINode*> selected;
	CppGen* codeGen = nullptr;
	ImVec2 designAreaMin, designAreaMax; //ImRect is internal?
	std::string workingDir;
	enum Color { Hovered, Selected, Snap1, Snap2, Snap3, Snap4, Snap5, COUNT };
	std::array<ImU32, Color::COUNT> colors;
	std::vector<std::string> fontNames;
	ImFont* defaultFont = nullptr;
	std::string unit; //for dimension export
	bool createVars = true; //create variables etc. during contructor/Clone calls
	ImGuiStyle style;
	const property_base* setProp = nullptr;
	std::string setPropValue;
	std::string forVarName;
	ImTextureID dashTexId = 0;

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
	UINode* hovered = nullptr;
	UINode* dragged = nullptr;
	ImVec2 lastSize;
	int groupLevel = 0;
	int importLevel;
	std::string userCode;
	UINode* root = nullptr;
	ImGuiWindow* rootWin = nullptr;
	bool beingResized = false;
	std::vector<ImGuiWindow*> activePopups;
	std::vector<UINode*> parents;
	int parentId;
	std::vector<std::string> contextMenus;
	int kind = 0; //TopWindow::Kind
	float unitFactor = 1; //for dimension value scaling
	ImVec2 selStart, selEnd;
	std::string ind;
	int varCounter;
	std::vector<std::string> errors;
	ImVec2 stretchSize;
	std::array<std::string, 2> stretchSizeExpr;

	//convenience
	void ind_up();
	void ind_down();
};
