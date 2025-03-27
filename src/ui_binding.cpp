// Generated with ImRAD 0.8
// github.com/xyz

#include "ui_binding.h"
#include "ui_new_field.h"
#include "utils.h"

BindingDlg bindingDlg;


void BindingDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 1.f;
    ImGui::OpenPopup(ID);
    Init();
}

void BindingDlg::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    auto *ioUserData = (ImRad::IOUserData *)ImGui::GetIO().UserData;
    ioUserData->dimBgRatio = 0.f;
}

void BindingDlg::Draw()
{
    /// @style Dark
    /// @unit px
    /// @begin TopWindow
    auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;
    ID = ImGui::GetID("###BindingDlg");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10, 10 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 7, 7 });
    ImGui::SetNextWindowSize({ 600, 480 }, ImGuiCond_FirstUseEver); //{ 600, 480 }
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Binding###BindingDlg", &tmpOpen, ImGuiWindowFlags_NoCollapse))
    {
        if (ioUserData->activeActivity != "")
            ImRad::RenderDimmedBackground(ioUserData->WorkRect(), ioUserData->dimBgRatio);
        if (modalResult != ImRad::None)
        {
            ImGui::CloseCurrentPopup();
            if (modalResult != ImRad::Cancel)
                callback(modalResult);
        }
        /// @separator

        newFieldPopup.Draw();

        /// @begin Text
        ImGui::PushStyleColor(ImGuiCol_Text, 0xff4d4dff);
        ImGui::TextUnformatted(ImRad::Format(" {}=", name).c_str());
        ImGui::PopStyleColor();
        /// @end Text

        /// @begin Selectable
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(true);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 1.f, 0 });
        ImRad::Selectable(ImRad::Format("{}", type).c_str(), false, ImGuiSelectableFlags_DontClosePopups, { 0, 0 });
        ImGui::PopStyleVar();
        ImGui::EndDisabled();
        /// @end Selectable

        /// @begin Input
        ImGui::PushFont(font);
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        if (focusExpr)
        {
            focusExpr = false;
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##expr", &expr, ImGuiInputTextFlags_CallbackCharFilter, IMRAD_INPUTTEXT_EVENT(BindingDlg, OnTextInputFilter));
        if (ImGui::IsItemActive())
            ioUserData->imeType = ImRad::ImeText;
        ImGui::PopFont();
        /// @end Input

        /// @begin Text
        hb3.BeginLayout();
        ImRad::Spacing(1);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Available fields:");
        hb3.AddSize(0, ImRad::HBox::ItemSize);
        /// @end Text

        /// @begin Spacer
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImRad::Dummy({ hb3.GetSize(), 0 });
        hb3.AddSize(1, ImRad::HBox::Stretch(1));
        /// @end Spacer

        /// @begin CheckBox
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Checkbox("show all", &showAll))
            Refresh();
        hb3.AddSize(1, ImRad::HBox::ItemSize);
        /// @end CheckBox

        /// @begin Table
        if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_ScrollY, { 0, -48 }))
        {
            ImGui::TableSetupColumn("Name", 0, 0);
            ImGui::TableSetupColumn("Type", 0, 0);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (int i = 0; i < vars.size(); ++i)
            {
                ImGui::PushID(i);
                ImGui::TableNextRow(0, 0);
                ImGui::TableSetColumnIndex(0);
                /// @separator

                /// @begin Selectable
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
                if (ImRad::Selectable(ImRad::Format("{}", vars[i].first).c_str(), false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAllColumns, { 0, 0 }))
                    OnVarClicked();
                ImGui::PopStyleVar();
                /// @end Selectable

                /// @begin Selectable
                ImRad::TableNextColumn(1);
                ImGui::BeginDisabled(true);
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
                ImRad::Selectable(ImRad::Format("{}", vars[i].second).c_str(), false, ImGuiSelectableFlags_DontClosePopups, { 0, 0 });
                ImGui::PopStyleVar();
                ImGui::EndDisabled();
                /// @end Selectable


                /// @separator
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        /// @end Table

        /// @begin Button
        hb5.BeginLayout();
        ImRad::Spacing(1);
        if (ImGui::Button(" New Field... ", { 110, 30 }))
        {
            OnNewField();
        }
        hb5.AddSize(0, 110);
        /// @end Button

        /// @begin Spacer
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImRad::Dummy({ hb5.GetSize(), 0 });
        hb5.AddSize(1, ImRad::HBox::Stretch(1));
        /// @end Spacer

            bool exprValid = true;
            /*if (type == "bool" || type == "float" || type == "int")
                exprValid = stx::count_if(expr, [](char c) { return !std::isspace(c); });*/
            //combo.items requires carefully embedded nulls so disable editing here
            if (type == "std::vector<std::string>" && 
                (expr.empty() || expr[0] != '{' || expr.find('{', 1) != std::string::npos || expr.back() != '}')) 
                exprValid = false;
        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(!exprValid);
        if (ImGui::Button("OK", { 90, 30 }))
        {
            ClosePopup(ImRad::Ok);
        }
        hb5.AddSize(1, 90);
        ImGui::EndDisabled();
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Cancel", { 90, 30 }) ||
            ImGui::Shortcut(ImGuiKey_Escape))
        {
            ClosePopup(ImRad::Cancel);
        }
        hb5.AddSize(1, 90);
        /// @end Button

        /// @separator
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    /// @end TopWindow
}

void BindingDlg::Refresh()
{
    vars = codeGen->GetVarExprs(showAll ? "" : type);
}

void BindingDlg::OnNewField()
{
    newFieldPopup.mode = NewFieldPopup::NewField;
    newFieldPopup.varType = ""; //allow all types f.e. vector<string> for taking its .size()
    newFieldPopup.scope = "";
    newFieldPopup.codeGen = codeGen;
    newFieldPopup.OpenPopup([this] {
        Refresh();
        });
}

void BindingDlg::OnVarClicked()
{
    int idx = ImGui::TableGetRowIndex() - 1; //header row
    if (type == "std::string")
        expr += "{" + vars[idx].first + "}";
    else if (type == "std::vector<std::string>")
        expr = "{" + vars[idx].first + "}";
    else
        expr = vars[idx].first;
    focusExpr = true;
}

int BindingDlg::OnTextInputFilter(ImGuiInputTextCallbackData& data)
{
    return InputTextCharExprFilter(&data);
}

void BindingDlg::Init()
{
    showAll = type == "std::string";
    Refresh();
}

void BindingDlg::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    hb3.Reset();
    hb5.Reset();
}

 
