// Generated with imgui-designer 0.1
// github.com/xyz

#pragma once
#include "node_container.h"
#include <string>
#include <imgui.h>
#include <vector>

class TableColumns
{
public:
    void OpenPopup();
    void Draw();

public:
    /// @interface
    std::vector<Table::ColumnData> columnData;
    std::vector<Table::ColumnData>* target;
    ImFont* defaultFont = nullptr;

private:
    ImGuiID ID;
    int selRow = 0;
    float selY = 0;
};

extern TableColumns tableColumns;