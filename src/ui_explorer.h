#pragma once
#include "imrad.h"
#include "cppgen.h"

extern int explorerFilter;
extern std::string explorerPath;
extern ImGuiTableColumnSortSpecs explorerSorting;

void ExplorerUI(const CppGen& codeGen, std::function<void (const std::string& fpath)> openFileFunc);
