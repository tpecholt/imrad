// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#pragma once
#include "imrad.h"

class SettingsDlg
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    std::vector<std::string> fontNames;
    std::string uiFontName;
    std::string uiFontSize;
    std::string pgFontName;
    std::string pgbFontName;
    std::string pgFontSize;
    /// @end interface

private:
    /// @begin impl
    void ResetLayout();
    void Init();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    ImRad::VBox vb1;
    ImRad::HBox hb3;
    /// @end impl
};

extern SettingsDlg settingsDlg;
