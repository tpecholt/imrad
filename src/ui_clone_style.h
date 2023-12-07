// Generated with ImRAD 0.7
// visit https://github.com/tpecholt/imrad

#pragma once
#include "imrad.h"

class CloneStyle
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr);
    void Draw();

    std::string styleName;
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

extern CloneStyle cloneStyle;
