// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#include "ui_select_resource.h"
#include "ui_message_box.h"
#include "utils.h"
#include "stx.h"
#include <IconsFontAwesome6.h>

SelectResource selectResource;


void SelectResource::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImRad::GetUserData().dimBgRatio = 1.f;
    ImGui::OpenPopup(ID);
    Init();
}

void SelectResource::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    ImRad::GetUserData().dimBgRatio = 0.f;
}

void SelectResource::Init()
{
    // TODO: Add your code here
    ReadZip();
}

void SelectResource::Draw()
{
    /// @dpi-info 141.357,1.25
    /// @style imrad
    /// @unit px
    /// @begin TopWindow
    ID = ImGui::GetID("###SelectResource");
    ImGui::SetNextWindowSize({ 400, 420 }, ImGuiCond_FirstUseEver); //{ 400, 420 }
    ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Select Resource###SelectResource", &tmpOpen, ImGuiWindowFlags_NoCollapse))
    {
        if (ImRad::GetUserData().activeActivity != "")
            ImRad::RenderDimmedBackground(ImRad::GetUserData().WorkRect(), ImRad::GetUserData().dimBgRatio);
        if (modalResult != ImRad::None)
        {
            ImGui::CloseCurrentPopup();
            if (modalResult != ImRad::Cancel)
                callback(modalResult);
        }
        if (ImGui::Shortcut(ImGuiKey_Escape))
            ClosePopup();
        /// @separator

        // TODO: Add Draw calls of dependent popup windows here

        /// @begin Child
        vb1.BeginLayout();
        ImGui::BeginChild("child1", { -1, vb1.GetSize() }, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoSavedSettings);
        {
            /// @separator

            /// @begin CustomWidget
            CustomWidget_Draw({ -1, -1 });
            /// @end CustomWidget

            /// @separator
            ImGui::EndChild();
        }
        vb1.AddSize(0, ImRad::VBox::Stretch(-1.0f));
        /// @end Child

        
        /// @separator
        ImGui::EndPopup();
    }
    /// @end TopWindow
}

void SelectResource::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    vb1.Reset();
}

void SelectResource::CustomWidget_Draw(const ImRad::CustomWidgetArgs& args)
{
    std::string prefix = "";
    std::vector<bool> path;
    int n = 1;
    for (const std::string& item : items)
    {
        std::string pre = "", fname = item;
        size_t i = item.rfind('/');
        if (i != std::string::npos) {
            pre = item.substr(0, i);
            fname = item.substr(i + 1);
        }
        //pop folders
        if (pre.compare(0, prefix.size(), prefix)) {
            size_t j = prefix.size();
            i = prefix.rfind('/', j);
            if (i == std::string::npos)
                i = 0;
            while (prefix.compare(0, prefix.size(), pre))
            {
                if (!stx::count(path, false))
                    ImGui::TreePop();
                path.pop_back();
                j = i;
                prefix.resize(j);
                if (!i)
                    break;
                i = prefix.rfind('/', j);
                if (i == std::string::npos)
                    i = 0;
            }
        }
        //push folders
        if (pre.size() > prefix.size()) 
        {
            i = prefix.size() ? prefix.size() + 1 : 0;
            size_t j = pre.find('/', i);
            if (j == std::string::npos)
                j = pre.size();
            while (true) 
            {
                bool open = false;
                if (!stx::count(path, false)) 
                {
                    ImGui::SetNextItemOpen(false, ImGuiCond_Appearing);
                    ImGui::PushStyleColor(ImGuiCol_Text, 0xff006060);
                    open = ImGui::TreeNodeEx((ICON_FA_FOLDER "##" + std::to_string(n)).c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::TextUnformatted(pre.substr(i, j - i).c_str());
                }
                path.push_back(open);
                if (j == pre.size())
                    break;
                i = j + 1;
                j = pre.find('/', i);
                if (j == std::string::npos)
                    j = pre.size();
            }
            prefix = pre;
        }
        //item
        if (!stx::count(path, false)) 
        {
            bool open = ImGui::TreeNodeEx((ICON_FA_IMAGE "##" + item).c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf);
            if (ImGui::IsItemActivated()) {
                selectedName = fname;
                modalResult = ImRad::Ok;
                ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] = false; //eat event
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(fname.c_str());
            ImGui::TreePop();
        }
        ++n;
    }
    //pop folders
    size_t i = prefix.rfind('/', prefix.size());
    if (i == std::string::npos)
        i = 0;
    while (path.size()) {
        if (!stx::count(path, false))
            ImGui::TreePop();
        path.pop_back();
        if (!i)
            break;
        i = prefix.rfind('/', i - 1);
    }
}

void SelectResource::ReadZip()
{
    items.clear();
    unzFile zipfh = unzOpen(zipFile.c_str());
    if (!zipfh)
        return;

    unz_global_info global_info;
    if (unzGetGlobalInfo(zipfh, &global_info) != UNZ_OK)
    {
        unzClose(zipfh);
        return;
    }

    for (size_t i = 0; i < global_info.number_entry; ++i, unzGoToNextFile(zipfh))
    {
        unz_file_info file_info;
        char filename[256];
        if (unzGetCurrentFileInfo(zipfh, &file_info, filename, 256, NULL, 0, NULL, 0) != UNZ_OK)
            continue;
        if (filename[strlen(filename) - 1] == '/')
            continue;
        items.push_back(filename);
    }

    stx::sort(items, [](const auto& a, const auto& b) {
        size_t n1 = stx::count(a, '/');
        size_t n2 = stx::count(b, '/');
        if (n1 != n2) // show folders first
            return n1 > n2;
        return a < b;
        });
    unzClose(zipfh);
}
