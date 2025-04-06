#pragma once
#include <string>
#include <functional>
#include <imgui.h>

extern int explorerFilter;
extern std::string explorerPath;
extern ImGuiTableColumnSortSpecs explorerSorting;

void ExplorerUI(std::function<void (const std::string& fpath)> openFileFunc);