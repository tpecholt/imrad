#pragma once

// Generated with imgui-designer 0.1
// github.com/xyz

#include <string>
#include <functional>
#include <imgui.h>
#include "cppgen.h"

class ClassWizard
{
public:
	void OpenPopup();
	void ClosePopup();
	void Draw();

public:
	CppGen* codeGen;
	UINode* root;
	bool* modified;
	
private:
	void Refresh();
	void FindUsed(UINode* node, std::vector<std::string>& used);

	std::string varName;
	std::string className;
	std::vector<std::string> stypes;
	std::vector<CppGen::Var> fields;
	std::vector<std::string> used;
	size_t stypeIdx;
	int selRow;
	
private:
	ImGuiID ID;
	bool requestClose;
};

extern ClassWizard classWizard;



