// Generated with ImRAD 0.8
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
    void Refresh();

    std::string name;
    std::string expr;
    std::string type;
    CppGen * codeGen;
    ImFont * font = nullptr;
    bool showAll;
    std::vector<std::pair<std::string , std::string >> vars;
    bool focusExpr = false;
    /// @end interface

private:
    /// @begin impl
    void Init();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    /// @end impl
};

extern BindingDlg bindingDlg;
