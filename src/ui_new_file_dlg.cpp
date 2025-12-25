// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#include "ui_new_file_dlg.h"
#include "IconsFontAwesome6.h"
#include "node_window.h"
#include "cursor.h"
#include <imgui_internal.h>
#include <httplib.h>

NewFileDlg newFileDlg;


void NewFileDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImRad::GetUserData().dimBgRatio = 1.f;
    IM_ASSERT(ID && "Call Draw at least once to get ID assigned");
    ImGui::OpenPopup(ID);
    Init();
}

void NewFileDlg::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    ImRad::GetUserData().dimBgRatio = 0.f;
}

void NewFileDlg::Init()
{
    // TODO: Add your code here
    category = FileTemplate;
    itemId = -1;
    itemUrl = "";
    Category_Change();
}

void NewFileDlg::Draw()
{
    /// @dpi-info 141.357,1.25
    /// @style imrad
    /// @unit dp
    /// @begin TopWindow
    const float dp = ImRad::GetUserData().dpiScale;
    ID = ImGui::GetID("###NewFileDlg");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15*dp, 15*dp });
    ImGui::SetNextWindowSize({ 720*dp, 520*dp }, ImGuiCond_FirstUseEver); //{ 720*dp, 520*dp }
    ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("New File###NewFileDlg", &tmpOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
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

        /// @begin Text
        vb1.BeginLayout();
        hb1.BeginLayout();
        ImRad::Spacing(1);
        ImGui::PushFont(nullptr, uiFontSize*1.4f);
        ImGui::TextUnformatted("New File");
        vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
        hb1.AddSize(0 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::ItemSize);
        ImGui::PopFont();
        /// @end Text

        /// @begin Spacer
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImRad::Dummy({ hb1.GetSize(), 20*dp });
        vb1.UpdateSize(0, 20*dp);
        hb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::Stretch(1.0f));
        /// @end Spacer

        /// @begin Selectable
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
        ImGui::PushFont(nullptr, uiFontSize*1.4f);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
        if (ImRad::Selectable(" \xef\x80\x8d", false, ImGuiSelectableFlags_NoAutoClosePopups, { 0, 0 }))
        {
            ClosePopup(ImRad::Cancel);
        }
        ImGui::PopStyleVar();
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        hb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::ItemSize);
        ImGui::PopFont();
        ImGui::PopItemFlag();
        /// @end Selectable

        // TODO: Add Draw calls of dependent popup windows here

        /// @begin Table
        hb2.BeginLayout();
        ImRad::Spacing(3);
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 7*dp, 0 });
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xffdbd3ce);
        if (ImGui::BeginTable("table1", 1, ImGuiTableFlags_PadOuterX | ImGuiTableFlags_ScrollY, { hb2.GetSize(), vb1.GetSize() }))
        {
            ImGui::TableSetupColumn("A", 0, 0);
            ImGui::TableSetupScrollFreeze(0, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Selectable
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0.5f });
            if (ImRad::Selectable("Project Templates", category==0, ImGuiSelectableFlags_NoAutoClosePopups, { -1, 40*dp }))
            {
                Category_Change();
            }
            ImGui::PopStyleVar();
            /// @end Selectable

            /// @begin Selectable
            ImRad::TableNextColumn(1);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0.5f });
            if (ImRad::Selectable("New Window", category==1, ImGuiSelectableFlags_NoAutoClosePopups, { -1, 40*dp }))
            {
                Category_Change();
            }
            ImGui::PopStyleVar();
            /// @end Selectable

            /// @begin Selectable
            ImRad::TableNextColumn(1);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0.5f });
            if (ImRad::Selectable("ImGui Examples", category==2, ImGuiSelectableFlags_NoAutoClosePopups, { -1, 40*dp }))
            {
                Category_Change();
            }
            ImGui::PopStyleVar();
            /// @end Selectable

            /// @separator
            ImGui::EndTable();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        vb1.AddSize(4 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::Stretch(1.0f));
        hb2.AddSize(0 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::Stretch(0.3f));
        ImGui::PopItemFlag();
        /// @end Table

        /// @begin Table
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xfff0f0f0);
        if (ImGui::BeginTable("table2", 1, ImGuiTableFlags_ScrollY, { hb2.GetSize(), vb1.GetSize() }))
        {
            ImGui::TableSetupColumn("A", 0, 0);
            ImGui::TableSetupScrollFreeze(0, 0);

            for (int i = 0; i < items.size(); ++i)
            {
                auto& _item = items[i];
                ImGui::PushID(i);
                ImGui::TableNextRow(0, 0);
                ImGui::TableSetColumnIndex(0);
                /// @separator

                /// @begin Selectable
                ImGui::BeginDisabled(_item.id<0);
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
                if (ImRad::Selectable("", false, ImGuiSelectableFlags_NoAutoClosePopups, { -1, 57*dp }))
                {
                    Item_Change();
                    ClosePopup(ImRad::Ok);
                }
                ImGui::PopStyleVar();

                auto tmpCursor2 = ImRad::GetCursorData();
                auto tmpItem2 = ImRad::GetLastItemData();
                auto tmpRect2 = ImRad::Rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                ImVec2 tmpPadding2 = ImGui::GetStyle().FramePadding;
                ImGui::SetCursorScreenPos({tmpRect2.Min.x + tmpPadding2.x, tmpRect2.Min.y + tmpPadding2.y });
                ImGui::BeginGroup();
                ImGui::PushClipRect({ tmpRect2.Min.x + tmpPadding2.x, tmpRect2.Min.y + tmpPadding2.y }, { tmpRect2.Max.x - tmpPadding2.x, tmpRect2.Max.y - tmpPadding2.y }, true);
                /// @separator

                /// @begin Text
                ImGui::SetCursorScreenPos({ tmpRect2.Max.x-80*dp, tmpRect2.Min.y+7*dp }); //overlayPos
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::TextUnformatted(ImRad::Format("{}", _item.platform).c_str());
                ImGui::PopStyleColor();
                /// @end Text

                /// @begin Text
                ImGui::SetCursorScreenPos({ tmpRect2.Min.x+7*dp, tmpRect2.Min.y+32*dp }); //overlayPos
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(category==GithubTemplate?ImGuiCol_TextDisabled:ImGuiCol_Text));
                ImGui::TextUnformatted(ImRad::Format("{}", _item.description).c_str());
                ImGui::PopStyleColor();
                /// @end Text

                /// @begin Text
                ImGui::SetCursorScreenPos({ tmpRect2.Min.x+7*dp, tmpRect2.Min.y+7*dp }); //overlayPos
                ImGui::PushFont(nullptr, ::uiFontSize*1.1f);
                ImGui::PushStyleColor(ImGuiCol_Text, 0xff000080);
                ImGui::TextUnformatted(ImRad::Format("{}", _item.label).c_str());
                ImGui::PopStyleColor();
                ImGui::PopFont();
                /// @end Text

                /// @separator
                ImGui::PopClipRect();
                ImGui::EndGroup();
                ImRad::SetCursorData(tmpCursor2);
                ImRad::SetLastItemData(tmpItem2);
                ImGui::EndDisabled();
                /// @end Selectable

                /// @separator
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleColor();
        vb1.UpdateSize(0, ImRad::VBox::Stretch(1.0f));
        hb2.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::Stretch(1.0f));
        ImGui::PopItemFlag();
        /// @end Table

        /// @separator
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    /// @end TopWindow
}

void NewFileDlg::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    vb1.Reset();
    hb1.Reset();
    hb2.Reset();
}

void NewFileDlg::Category_Change()
{
    if (ImGui::GetCurrentTable())
        category = (Category)ImGui::TableGetRowIndex();

    if (!category)
    {
        items = {
            { "GLFW template (Windows/Linux/macOS)", "GLFW", "main.cpp", 0 },
            { "Android template", "Android", "main.cpp, CMakeLists.txt, MainActivity.java, AndroidManifest.xml", 1 }
        };
    }
    else if (category == 1)
    {
        items = {
            { ICON_FA_TV "  Main Window", "GLFW", "ImGui window integrated into OS window", TopWindow::MainWindow },
            { ICON_FA_WINDOW_MAXIMIZE "  Window", "(All)", "General ImGui window", TopWindow::Window },
            { ICON_FA_CLONE "  Popup", "(All)", "Popup window without a titlebar", TopWindow::Popup },
            { ICON_FA_WINDOW_RESTORE "  ModalPopup", "(All)", "Modal popup window", TopWindow::ModalPopup },
            { ICON_FA_MOBILE_SCREEN "  Activity", "Android", "Maximized, title-less window where only one is active at a time", TopWindow::Activity }
        };
    }
    else if (category == 2)
    {
        SetWaitCursor();
        items.clear();
        try {
            httplib::Headers hs;
            hs.insert({ "Accept", "application/vnd.github+json" });
            hs.insert({ "X-GitHub-Api-Version", "2022-11-28" });
            httplib::Client cli("https://api.github.com");
            auto res = cli.Get("/repos/ocornut/imgui/contents/examples", hs);
            if (res->status != httplib::StatusCode::OK_200)
                return;
            //dirty parsing
            std::istringstream is(res->body);
            FileItem item;
            int level = 0;
            for (cpp::token_iterator tok(is); tok != cpp::token_iterator(); ++tok)
            {
                if (*tok == "[" || *tok == "{") {
                    if (++level == 2) {
                        item.label = item.description = item.platform = "";
                        item.id = (int)items.size();
                    }
                }
                else if (*tok == "]" || *tok == "}") {
                    if (level-- == 2 && item.id >= 0)
                        items.push_back(item);
                }
                if (level != 2)
                    continue;

                if (*tok == "\"name\"") {
                    if (*++tok != ":")
                        continue;
                    ++tok;
                    if (tok->size() < 2 || tok->compare(0, 9, "\"example_"))
                        item.id = -1;
                    else
                        item.label = tok->substr(1, tok->size() - 2);
                }
                else if (*tok == "\"html_url\"") {
                    if (*++tok != ":")
                        continue;
                    ++tok;
                    if (tok->size() < 2 || (*tok)[0] != '\"')
                        item.id = -1;
                    else
                        item.description = tok->substr(1, tok->size() - 2);
                }
                else if (*tok == "\"type\"") {
                    if (*++tok != ":")
                        continue;
                    if (*++tok != "\"dir\"")
                        item.id = -1;
                }
            }
        }
        catch (std::exception&) {
            //OpenSSL likely not installed
            return;
        }
    }
    else
    {
        items = {
            { "Not found", "", "", -1 }
        };
    }
}

void NewFileDlg::Item_Change()
{
    if (ImGui::GetCurrentTable() && ImGui::TableGetRowIndex() < items.size()) {
        itemId = items[ImGui::TableGetRowIndex()].id;
        itemUrl = category == GithubTemplate ?
            items[ImGui::TableGetRowIndex()].description : "";
    }
}
