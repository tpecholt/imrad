// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#pragma once
#include "imrad.h"

class ErrorBox
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    std::string title = "title";
    std::string message = "message";
    std::string error;
    /// @end interface

private:
    /// @begin impl
    void ResetLayout();
    void Init();

    void CustomWidget_Draw(const ImRad::CustomWidgetArgs& args);

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    ImRad::VBox vb1;
    ImRad::HBox hb3;
    /// @end impl
};

extern ErrorBox errorBox;
