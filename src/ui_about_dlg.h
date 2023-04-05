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
    /// @interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup();
    void Draw();


private:
    /// @impl
    void OpenURL();

    ImGuiID ID;
    bool requestClose = false;
    std::function<void(ImRad::ModalResult)> callback;

};

extern AboutDlg aboutDlg;
