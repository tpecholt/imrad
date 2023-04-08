// Generated with ImRAD 0.1
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
    /// @interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup();
    void Draw();

    std::string name;
    std::string expr;
    std::string type;
    CppGen* codeGen;
    bool showAll;

private:
    /// @impl
    std::vector<std::pair<std::string , std::string >> vars;
    void OnNewField();
    void OnVarClicked();
    void Refresh();
    ImGuiID ID;

    bool requestOpen = false;
    bool requestClose = false;
    std::function<void(ImRad::ModalResult)> callback;

};

extern BindingDlg bindingDlg;
