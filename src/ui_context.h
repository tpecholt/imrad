#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <sstream>
#include <iomanip>
#include <imgui.h>
#include "cpp_parser.h"
#include "imrad.h"
#include "IconsFontAwesome6.h"

struct UINode;
class CppGen;

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
	ImVec2 lastSize;
	int groupLevel = 0;
	int importLevel;
	std::string userCode;
	UINode* root = nullptr;
	ImGuiWindow* rootWin = nullptr;
	std::vector<ImGuiWindow*> activePopups;
	std::vector<UINode*> parents;
	std::vector<std::string> contextMenus;
	int kind = 0; //TopWindow::Kind
	float unitFactor = 1; //for dimension value scaling
	ImVec2 selStart, selEnd;
	std::string ind;
	int varCounter;
	std::vector<std::string> errors;

	//convenience
	void ind_up();
	void ind_down();
};
