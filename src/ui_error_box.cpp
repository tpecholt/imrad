// Generated with ImRAD 0.9
// visit https://github.com/tpecholt/imrad

#include "ui_error_box.h"
#include <IconsFontAwesome6.h>

ErrorBox errorBox;


std::string WordWrap(const std::string& str, float multilineWidth)
{
    float textSize = 0;
    std::string tmpStr = "";
    std::string finalStr = "";
    int curChr = 0;
    while (curChr < str.size()) {

        if (str[curChr] == '\n') {
            finalStr += tmpStr + "\n";
            tmpStr = "";
        }
        else
            tmpStr += str[curChr];
        textSize = ImGui::CalcTextSize(tmpStr.c_str()).x;

        if (textSize > multilineWidth) {
            int lastSpace = (int)tmpStr.size() - 1;
            while (tmpStr[lastSpace] != ' ' && lastSpace > 0)
                lastSpace--;
            if (lastSpace == 0)
                lastSpace = (int)tmpStr.size() - 2;
            finalStr += tmpStr.substr(0, lastSpace + 1) + "\n";
            if (lastSpace + 1 > tmpStr.size())
                tmpStr = "";
            else
                tmpStr = tmpStr.substr(lastSpace + 1);
        }
        curChr++;
    }
    if (tmpStr.size() > 0)
        finalStr += tmpStr;

    return finalStr;
};

int TreeNode(const std::string& str, size_t& i, int level, bool skip)
{
    struct ScopeGuard {
        ScopeGuard(std::function<void()> f) : func(f) {}
        ~ScopeGuard() { func(); }
        std::function<void()> func;
    };
    for (int n = 0; i < str.size(); ++n)
    {
        ImGui::PushID(n);
        ScopeGuard g([] { ImGui::PopID(); });

        size_t j = str.find("\n", i);
        int nlevel = 0;
        std::string label;
        if (j == std::string::npos) {
            label = str.substr(i);
            i = str.size();
        }
        else {
            label = str.substr(i, j - i);
            i = j + 1;
            for (nlevel = 0; i < str.size() && str[i] == '\t'; ++i)
                ++nlevel;
        }

        if (nlevel > level) {
            bool open = true;
            if (!skip) {
                ImGui::PushStyleColor(ImGuiCol_Text, !level ? 0xffa02020 : 0xff000000);
                open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::PopStyleColor();
            }
            nlevel = TreeNode(str, i, nlevel, skip || !open);
            if (nlevel < level) {
                if (!skip && open)
                    ImGui::TreePop();
                return nlevel;
            }
        }
        else {
            if (!skip) {
                ImVec2 pad = ImGui::GetStyle().FramePadding;
                if (!level) {
                    ImGui::Bullet();
                    ImGui::SameLine(0, pad.x * 3);
                }
                else {
                    ImGui::Text(" ");
                    ImGui::SameLine(0, pad.x * 3);
                }
                float w = ImGui::GetContentRegionAvail().x - pad.x;
                label = WordWrap(label, w);
                ImGui::Text("%s", label.c_str());
            }
            if (nlevel < level) {
                if (!skip)
                    ImGui::TreePop();
                return nlevel;
            }
        }
    }
    return 0;
}

//-----------------------------------------------------------

void ErrorBox::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 1.f;
    ImGui::OpenPopup(ID);
    Init();
}

void ErrorBox::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 0.f;
}

void ErrorBox::Init()
{
    // TODO: Add your code here
}

void ErrorBox::Draw()
{
    /// @style Dark
    /// @unit px
    /// @begin TopWindow
    auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;
    ID = ImGui::GetID("###ErrorBox");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10, 10 });
    ImGui::SetNextWindowSize({ 640, 480 }, ImGuiCond_FirstUseEver); //{ 640, 480 }
    ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal(ImRad::Format("{}###ErrorBox", title).c_str(), &tmpOpen, ImGuiWindowFlags_NoCollapse))
    {
        if (ioUserData->activeActivity != "")
            ImRad::RenderDimmedBackground(ioUserData->WorkRect(), ioUserData->dimBgRatio);
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
        ImRad::Spacing(1);
        ImGui::PushStyleColor(ImGuiCol_Text, 0xff0099ff);
        ImGui::TextUnformatted(ImRad::Format(" {} ", ICON_FA_CIRCLE_EXCLAMATION).c_str());
        vb1.AddSize(1, ImRad::VBox::ItemSize);
        ImGui::PopStyleColor();
        /// @end Text

        /// @begin Text
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(ImRad::Format("{}", message).c_str());
        ImGui::PopTextWrapPos();
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        /// @end Text

        /// @begin Child
        ImRad::Spacing(3);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 4 });
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xffc0c0c0);
        ImGui::BeginChild("child1", { -1, vb1.GetSize() }, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoSavedSettings);
        {
            /// @separator

            /// @begin CustomWidget
            CustomWidget_Draw({ -1, -1 });
            /// @end CustomWidget

            /// @separator
            ImGui::EndChild();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        vb1.AddSize(4, ImRad::VBox::Stretch(1));
        /// @end Child

        /// @begin Spacer
        hb3.BeginLayout();
        ImRad::Spacing(1);
        ImRad::Dummy({ hb3.GetSize(), 0 });
        vb1.AddSize(2, ImRad::VBox::ItemSize);
        hb3.AddSize(0, ImRad::HBox::Stretch(1));
        /// @end Spacer

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("OK", { 100, 30 }))
        {
            ClosePopup(ImRad::Ok);
        }
        vb1.UpdateSize(0, 30);
        hb3.AddSize(1, 100);
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere(-1);
        /// @end Button

        /// @begin Spacer
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImRad::Dummy({ hb3.GetSize(), 0 });
        vb1.UpdateSize(0, ImRad::VBox::ItemSize);
        hb3.AddSize(1, ImRad::HBox::Stretch(1));
        /// @end Spacer

        /// @separator
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    /// @end TopWindow
}

void ErrorBox::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    vb1.Reset();
    hb3.Reset();
}

void ErrorBox::CustomWidget_Draw(const ImRad::CustomWidgetArgs& args)
{
    size_t i = 0;
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    TreeNode(error, i, 0, false);
    ImGui::PopItemFlag();
}
