// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#pragma once
#include "imrad.h"

class SelectResource
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    std::string zipFile;
    std::string selectedName;
    /// @end interface

private:
    struct TreeItem
    {
        std::string label;
        std::vector<TreeItem> children;
    };

    /// @begin impl
    void ResetLayout();
    void Init();

    void CustomWidget_Draw(const ImRad::CustomWidgetArgs& args);

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    TreeItem root;
    std::vector<std::string> items;
    ImRad::VBox vb1;
    /// @end impl

    void ReadZip();
};

extern SelectResource selectResource;
