// Generated with ImRAD 0.9
// github.com/xyz

#pragma once
#include <string>
#include <functional>
#include <imgui.h>
#include "imrad.h"
#include "cppgen.h"

class BindingDlg
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    void OnNewField();
    void OnVarClicked();
    int OnTextInputFilter(ImGuiInputTextCallbackData& args);
    void Refresh();

    std::string name;
    std::string expr;
    std::string type;
    CppGen* codeGen;
    std::string curArray;
    bool forceReference;
    ImFont* font = nullptr;
    /// @end interface

private:
    /// @begin impl
    void ResetLayout();
    void Init();

    void OkButton_Change();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    bool showAll;
    std::vector<std::pair<std::string,std::string>> vars;
    bool focusExpr = false;
    ImRad::HBox hb1;
    ImRad::HBox hb3;
    ImRad::HBox hb5;
    /// @end impl
};

extern BindingDlg bindingDlg;
