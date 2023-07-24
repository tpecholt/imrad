// Generated with ImRAD 0.5
// visit github.com/tpecholt/imrad

#pragma once
#include "imrad.h"
#include "node.h"

class HorizLayout
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup();
    void Draw();

    int spacing = 1;
    int padding = 0;
    int alignment = 0;
    std::vector<UINode *> selected;
    UINode * root;
    UIContext * ctx;
	/// @end interface

	static void ExpandSelection(std::vector<UINode*>& selected, UINode* root);

private:
    /// @begin impl
    void OnAlignment();
    void Work();

    bool requestOpen = false;
    bool requestClose = false;
    std::function<void(ImRad::ModalResult)> callback;

    /// @end impl
};

extern HorizLayout horizLayout;
