// Generated with ImRAD 0.10-WIP
// visit https://github.com/tpecholt/imrad

#pragma once
#include <imrad.h>
#include "node_standard.h"

class TextEdit
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    std::string singular;
    std::string pluralVariable;
    std::string context;
    std::string text;
    std::string plural;
    int activeTab = 0;
    bool showPlural = true;
    ImFont* font = nullptr;
    /// @end interface

private:
    /// @begin impl
    void DrawPopups();
    void ResetLayout();
    void Init();

    void OkButton_Change();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    bool focusInput = false;
    ImRad::VBox vb1;
    ImRad::HBox hb11;
    /// @end impl

    bool OkDisabled();
    void DrawMultilineTextArgs(PreparedString& ps);
};

extern TextEdit textEdit;
