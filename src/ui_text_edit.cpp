// Generated with ImRAD 0.10-WIP
// visit https://github.com/tpecholt/imrad

#include "ui_text_edit.h"
#include "ui_message_box.h"
#include "utils.h"
#include "imgui_internal.h"

TextEdit textEdit;


void TextEdit::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImRad::GetUserData().dimBgRatio = 1.f;
    IM_ASSERT(ID && "Call Draw at least once to get ID assigned");
    ImGui::OpenPopup(ID);
    Init();
}

void TextEdit::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    ImRad::GetUserData().dimBgRatio = 0.f;
}

void TextEdit::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    vb1.Reset();
    hb11.Reset();
}

void TextEdit::DrawPopups()
{
    // TODO: Draw dependent popups here
    messageBox.Draw();
}

void TextEdit::Init()
{
    focusInput = true;
}

void TextEdit::Draw()
{
    /// @dpi-info 141.357,1.25
    /// @style imrad
    /// @unit dp
    /// @begin TopWindow
    const float dp = ImRad::GetUserData().dpiScale;
    ID = ImGui::GetID("###TextEdit");
    ImGui::SetNextWindowSize({ 420*dp, 380*dp }, ImGuiCond_FirstUseEver); //{ 420*dp, 380*dp }
    ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Text Editor###TextEdit", &tmpOpen, 0))
    {
        if (ImRad::GetUserData().activeActivity != "")
            ImRad::RenderDimmedBackground(ImRad::GetUserData().WorkRect(), ImRad::GetUserData().dimBgRatio);
        if (modalResult != ImRad::None)
        {
            ImGui::CloseCurrentPopup();
            if (modalResult != ImRad::Cancel)
                callback(modalResult);
        }
        DrawPopups();
        /// @separator

        /// @begin TabBar
        vb1.BeginLayout();
        if (ImGui::BeginTabBar("tabBar1", 0))
        {
            vb1.AddSize(0 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
            /// @separator

            /// @begin TabItem
            if (ImGui::BeginTabItem("Multiline", nullptr, activeTab == 0 && !ImRad::CurrentTabBarHasFocusQueued() ? ImGuiTabItemFlags_SetSelected : 0))
            {
                if (!ImGui::IsWindowAppearing())
                    activeTab = 0;
                /// @separator

                auto ps = PrepareString(text, false);
                /// @begin Input
                ImGui::PushFont(font, (0));
                ImGui::InputTextMultiline("##text", &text, { -1, vb1.GetSize() }, 0);
                if (ImGui::IsItemActive())
                    ImRad::GetUserData().imeType = ImRad::ImeText;
                vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::Stretch(1));
                ImGui::PopFont();
                if (focusInput)
                {
                    //forceFocus
                    if (ImGui::IsItemFocused())
                        focusInput = false;
                    ImGui::SetKeyboardFocusHere(-1);
                }
                /// @end Input

                DrawMultilineTextArgs(ps);

                /// @separator
                ImGui::EndTabItem();
            }
            /// @end TabItem

            /// @begin TabItem
            if (ImGui::BeginTabItem("Translation", nullptr, activeTab == 1 && !ImRad::CurrentTabBarHasFocusQueued() ? ImGuiTabItemFlags_SetSelected : 0))
            {
                if (!ImGui::IsWindowAppearing())
                    activeTab = 1;
                /// @separator

                /// @begin CollapsingHeader
                bool tmpOpen2 = ImGui::CollapsingHeader("Primary", ImGuiTreeNodeFlags_DefaultOpen);
                vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
                if (tmpOpen2)
                {
                    /// @separator

                    auto ps = PrepareString(singular, false);
                    /// @begin Input
                    ImGui::PushFont(font, (0));
                    ImGui::InputTextMultiline("##singular", &singular, { -1, vb1.GetSize() }, 0);
                    if (ImGui::IsItemActive())
                        ImRad::GetUserData().imeType = ImRad::ImeText;
                    vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::Stretch(1));
                    ImGui::PopFont();
                    if (focusInput)
                    {
                        //forceFocus
                        if (ImGui::IsItemFocused())
                            focusInput = false;
                        ImGui::SetKeyboardFocusHere(-1);
                    }
                    /// @end Input

                    DrawMultilineTextArgs(ps);

                    /// @begin Text
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Context");
                    vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::TextSize);
                    /// @end Text

                    /// @begin Input
                    ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
                    ImGui::PushFont(font, (0));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputText("##context", &context, 0);
                    if (ImGui::IsItemActive())
                        ImRad::GetUserData().imeType = ImRad::ImeText;
                    vb1.UpdateSize(0, ImRad::VBox::ItemSize);
                    ImGui::PopFont();
                    /// @end Input

                    /// @separator
                }
                /// @end CollapsingHeader

                /// @begin CollapsingHeader
                ImRad::Spacing(1);
                ImGui::BeginDisabled(!showPlural);
                bool tmpOpen3 = ImGui::CollapsingHeader("Plural form", 0);
                vb1.AddSize(2 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
                if (tmpOpen3)
                {
                    /// @separator

                    /// @begin Selectable
                    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
                    ImGui::AlignTextToFramePadding();
                    ImRad::Selectable("variable", false, ImGuiSelectableFlags_NoAutoClosePopups | ImGuiSelectableFlags_Disabled, { 50*dp, 0 });
                    ImGui::PopStyleVar();
                    vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
                    /// @end Selectable

                    /// @begin Input
                    ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputText("##pluralVariable", &pluralVariable, 0);
                    if (ImGui::IsItemActive())
                        ImRad::GetUserData().imeType = ImRad::ImeText;
                    vb1.UpdateSize(0, ImRad::VBox::ItemSize);
                    /// @end Input

                    auto ps = PrepareString(plural, false);
                    /// @begin Input
                    ImGui::PushFont(font, (0));
                    ImGui::InputTextMultiline("##plural", &plural, { -1, vb1.GetSize() }, 0);
                    if (ImGui::IsItemActive())
                        ImRad::GetUserData().imeType = ImRad::ImeText;
                    vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::Stretch(1));
                    ImGui::PopFont();
                    /// @end Input

                    DrawMultilineTextArgs(ps);

                    /// @separator
                }
                ImGui::EndDisabled();
                /// @end CollapsingHeader

                /// @begin Spacer
                ImRad::Dummy({ 15*dp, vb1.GetSize() });
                vb1.AddSize(1 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::Stretch(0.01f));
                /// @end Spacer

                /// @separator
                ImGui::EndTabItem();
            }
            /// @end TabItem

            /// @separator
            ImGui::EndTabBar();
        }
        /// @end TabBar

        /// @begin Spacer
        hb11.BeginLayout();
        ImRad::Spacing(1);
        ImRad::Dummy({ hb11.GetSize(), 0 });
        vb1.AddSize(2 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::ItemSize);
        hb11.AddSize(0 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::Stretch(1));
        /// @end Spacer

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(OkDisabled());
        if (ImGui::Button("OK", { 80*dp, 25*dp }))
        {
            OkButton_Change();
        }
        vb1.UpdateSize(0, 25*dp);
        hb11.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 80*dp);
        ImGui::EndDisabled();
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Cancel", { 80*dp, 25*dp }) ||
            ImGui::Shortcut(ImGuiKey_Escape))
        {
            ClosePopup(ImRad::Cancel);
        }
        vb1.UpdateSize(0, 25*dp);
        hb11.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 80*dp);
        /// @end Button

        /// @separator
        ImGui::EndPopup();
    }
    /// @end TopWindow
}

void TextEdit::DrawMultilineTextArgs(PreparedString& ps)
{
    if (ps.error || ImGui::GetCurrentWindow()->DC.ChildWindows.empty())
        return;
    UIContext ctx;
    ctx.colors[UIContext::Color::DrawArgs] = 0xff6060ff;
    ps.window = ImGui::GetCurrentWindow()->DC.ChildWindows.back();
    DrawTextArgs(ps, ctx, ImGui::GetStyle().FramePadding, ImGui::GetItemRectSize());
}

bool TextEdit::OkDisabled()
{
    if (!activeTab)
        return false;
    else
        return Trim(singular).empty() || Trim(plural).empty() != Trim(pluralVariable).empty();
}

void TextEdit::OkButton_Change()
{
    if (!activeTab)
    {
        if (Trim(text).empty())
        {
            messageBox.title = "Question";
            messageBox.icon = MessageBox::Info;
            messageBox.message = "Save empty text?";
            messageBox.buttons = ImRad::Yes | ImRad::Cancel;
            messageBox.OpenPopup([this](ImRad::ModalResult mr) {
                if (mr == ImRad::Yes)
                    ClosePopup(ImRad::Ok);
                });
            return;
        }
    }
    else
    {
        if (plural != "" &&
            cpp::format_str_arg(singular).second != cpp::format_str_arg(plural).second)
        {
            messageBox.title = "Error";
            messageBox.icon = MessageBox::Warning;
            messageBox.message = "Formatting variables used in singular and plural forms have to match";
            messageBox.buttons = ImRad::Ok;
            messageBox.OpenPopup();
            return;
        }
        context = Trim(context);
        pluralVariable = Trim(pluralVariable);
    }
    ClosePopup(ImRad::Ok);
}
