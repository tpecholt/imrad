// Generated with ImRAD 0.9
// github.com/xyz

#pragma once
#include <string>
#include <functional>
#include <imgui.h>
#include "imrad.h"

class AboutDlg
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    void OpenURL();

    /// @end interface

private:
    /// @begin impl
    void Init();

    void HoverURL();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    ImRad::Texture value1;
    /// @end impl
};

extern AboutDlg aboutDlg;
