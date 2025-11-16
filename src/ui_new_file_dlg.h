// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#pragma once
#include "imrad.h"

class NewFileDlg
{
public:
    enum Category { ProjectTemplate, FileTemplate, GithubTemplate };

    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    Category category;
    int itemId;
    std::string itemUrl;
    /// @end interface

private:
    struct FileItem
    {
        std::string label, platform, description;
        int id;
    };

    /// @begin impl
    void ResetLayout();
    void Init();

    void Category_Change();
    void Item_Change();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    std::vector<FileItem> items;
    ImRad::VBox vb1;
    ImRad::HBox hb1;
    ImRad::HBox hb2;
    /// @end impl
};

extern NewFileDlg newFileDlg;
