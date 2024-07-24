// Generated with ImRAD 0.8
// github.com/xyz

#include "ui_binding.h"
#include "ui_new_field.h"

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
        ImGui::InputText("##expr", &expr, ImGuiInputTextFlags_None);
        if (ImGui::IsItemActive())
            ioUserData->imeType = ImRad::ImeText;
        ImGui::PopFont();
        /// @end Input

        /// @begin Table
        if (ImGui::BeginTable("table1", 3, ImGuiTableFlags_None, { 0, 0 }))
        {
            ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableSetupColumn("B", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("C", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Text
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Available fields:");
            /// @end Text

            /// @begin Child
            ImRad::TableNextColumn(1);
            ImGui::BeginChild("child1", { 0, 20 }, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoSavedSettings);
            {
                /// @separator

                /// @separator
                ImGui::EndChild();
            }
            /// @end Child

            /// @begin CheckBox
            ImRad::TableNextColumn(1);
            if (ImGui::Checkbox("show all", &showAll))
                Refresh();
            /// @end CheckBox


            /// @separator
            ImGui::EndTable();
        }
        /// @end Table

        /// @begin Table
        if (ImGui::BeginTable("table3", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_ScrollY, { 0, -48 }))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_None, 0);
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
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
                ImRad::Selectable(ImRad::Format("{}", vars[i].second).c_str(), false, ImGuiSelectableFlags_DontClosePopups, { 0, 0 });
                ImGui::PopStyleVar();
                /// @end Selectable


                /// @separator
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        /// @end Table

        /// @begin Table
        ImRad::Spacing(1);
        if (ImGui::BeginTable("table4", 3, ImGuiTableFlags_None, { 0, -1 }))
        {
            ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("B", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableSetupColumn("C", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Button
            if (ImGui::Button(" New Field... ", { 110, 30 }))
            {
                OnNewField();
            }
            /// @end Button

			bool exprValid = true;
			if (type == "bool" || type == "float" || type == "int")
				exprValid = stx::count_if(expr, [](char c) { return !std::isspace(c); });
            /// @begin Button
            ImRad::TableNextColumn(1);
            ImGui::BeginDisabled(!exprValid);
            if (ImGui::Button("OK", { 90, 30 }))
            {
                ClosePopup(ImRad::Ok);
            }
            ImGui::EndDisabled();
            /// @end Button

            /// @begin Button
            ImRad::TableNextColumn(1);
            if (ImGui::Button("Cancel", { 90, 30 }) ||
                ImGui::Shortcut(ImGuiKey_Escape))
            {
                ClosePopup(ImRad::Cancel);
            }
            /// @end Button


            /// @separator
            ImGui::EndTable();
        }
        /// @end Table

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
	if (type == "std::string" || type == "std::vector<std::string>")
		expr += "{" + vars[idx].first + "}";
	else
		expr = vars[idx].first;
	focusExpr = true;
}

void BindingDlg::Init()
{
	showAll = type == "std::string";
	Refresh();
}
