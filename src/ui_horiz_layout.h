// Generated with ImRAD 0.8
// visit github.com/tpecholt/imrad

#pragma once
#include "imrad.h"
#include "node.h"

class HorizLayout
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    int spacing = 1;
    int padding = 0;
    int alignment;
    std::vector<UINode *> selected;
    UINode * root;
    UIContext * ctx;
    /// @end interface

	static void ExpandSelection(std::vector<UINode*>& selected, UINode* root);

private:
    /// @begin impl
    void Init();

    void OnAlignment();
    void Work();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    /// @end impl
};

extern HorizLayout horizLayout;
