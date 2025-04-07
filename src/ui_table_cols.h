// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#pragma once
#include "imrad.h"
#include "node_container.h"

class TableCols
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    std::vector<Table::ColumnData> columns;
    UIContext * ctx;
    float sash = 180;
    std::string error;
    /// @end interface

private:
    /// @begin impl
    void ResetLayout();
    void Init();

    void CheckErrors();
    void AddButton_Change();
    void RemoveButton_Change();
    void UpButton_Change();
    void DownButton_Change();
    void Properties_Draw(const ImRad::CustomWidgetArgs& args);
    void Selectable_Change();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    int sel;
    ImRad::VBox vb1;
    ImRad::HBox hb4;
    /// @end impl
};

extern TableCols tableCols;
