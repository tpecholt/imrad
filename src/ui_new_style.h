// Generated with ImRAD 0.10-WIP
// visit https://github.com/tpecholt/imrad

#pragma once
#include "imrad.h"

class NewStyleDlg
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    std::string name;
    std::string source;
    std::vector<std::string> sources;
    std::string title;
    /// @end interface

private:
    /// @begin impl
    void ResetLayout();
    void Init();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    ImRad::HBox hb5;
    /// @end impl
};

extern NewStyleDlg newStyleDlg;
