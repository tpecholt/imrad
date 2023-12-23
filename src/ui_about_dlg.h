// Generated with ImRAD 0.1
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
    void ClosePopup(ImRad::ModalResult mr);
    void Draw();

    void OpenURL();
    /// @end interface

private:
    /// @begin impl
    void Init();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    ImRad::Animator animator;
    std::function<void(ImRad::ModalResult)> callback;

    /// @end impl
};

extern AboutDlg aboutDlg;
