// Generated with ImRAD 0.1
// github.com/xyz

#include "ui_binding.h"
#include "ui_new_field.h"

BindingDlg bindingDlg;


void BindingDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
	callback = clb;
	requestOpen = true;
	requestClose = false;
	ImGui::OpenPopup(ID);

	// Add your init code here
	showAll = type == "std::string";
	Refresh();
}

void BindingDlg::ClosePopup()
{
	requestClose = true;
}

void BindingDlg::Draw()
{
    /// @style Dark
    /// @begin TopWindow
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10, 10 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 7, 7 });
    ImGui::SetNextWindowSize({ 480, 480 }, ImGuiCond_FirstUseEver);
    if (requestOpen) {
        requestOpen = false;
        ImGui::OpenPopup("Binding");
    }
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { 0.5f, 0.5f });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Binding", &tmpOpen, ImGuiWindowFlags_NoCollapse))
    {
        if (requestClose)
            ImGui::CloseCurrentPopup();
        /// @separator

		newFieldPopup.Draw();

        /// @begin Text
        ImGui::PushStyleColor(ImGuiCol_Text, 0xff4d4dff);
        ImGui::TextUnformatted(ImRad::Format(" {}=", name).c_str());
        ImGui::PopStyleColor();
        /// @end Text

        /// @begin Selectable
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 1.f, 0 });
		ImGui::Selectable(ImRad::Format("{}", type).c_str(), false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_Disabled, { 0, 0 });
		ImGui::PopStyleVar();
        /// @end Selectable

        /// @begin Input
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##expr", &expr, ImGuiInputTextFlags_None);
        /// @end Input

        /// @begin Table
        ImGui::Spacing();
        if (ImGui::BeginTable("table2201201799584", 3, ImGuiTableFlags_None, { 0, 0 }))
        {
            ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableSetupColumn("B", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("C", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Text
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Available fields:");
            /// @end Text

            /// @begin Child
            ImGui::TableNextColumn();
            ImGui::BeginChild("child2201294508848", { 0, 20 });
            {
                /// @separator

                /// @separator
                ImGui::EndChild();
            }
            /// @end Child

            /// @begin CheckBox
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("show all", &showAll))
                Refresh();
            /// @end CheckBox


            /// @separator
            ImGui::EndTable();
        }
        /// @end Table

        /// @begin Table
        if (ImGui::BeginTable("table2201201801200", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV, { 0, -44 }))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_None, 0);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < vars.size(); ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID((int)i);
                /// @separator

                /// @begin Selectable
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
                if (ImGui::Selectable(ImRad::Format("{}", vars[i].first).c_str(), false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAllColumns, { 0, 0 }))
					OnVarClicked();
				ImGui::PopStyleVar();
                /// @end Selectable

                /// @begin Selectable
                ImGui::TableNextColumn();
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0, 0 });
                ImGui::Selectable(ImRad::Format("{}", vars[i].second).c_str(), false, ImGuiSelectableFlags_DontClosePopups, { 0, 0 });
                ImGui::PopStyleVar();
                /// @end Selectable


                /// @separator
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        /// @end Table

        /// @begin Table
        ImGui::Spacing();
        if (ImGui::BeginTable("table2201201806048", 3, ImGuiTableFlags_None, { 0, -1 }))
        {
            ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("B", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableSetupColumn("C", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Button
            if (ImGui::Button(" New Field... ", { 110, 0 }))
            {
                OnNewField();
            }
            /// @end Button

            /// @begin Button
            ImGui::TableNextColumn();
            if (ImGui::Button("OK", { 90, 0 }))
            {
                ClosePopup();
                callback(ImRad::Ok);
            }
            /// @end Button

            /// @begin Button
            ImGui::TableNextColumn();
            if (ImGui::Button("Cancel", { 90, 0 }) ||
                (!ImRad::IsItemDisabled() && ImGui::IsKeyPressed(ImGuiKey_Escape, false)))
            {
                ClosePopup();
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
}
