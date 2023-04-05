// Generated with ImRAD 0.1
// github.com/xyz

#include "ui_binding.h"
#include "ui_new_field.h"

BindingDlg bindingDlg;


void BindingDlg::Draw()
{
    /// @begin TopWindow
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10, 10 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 7, 7 });
    ImGui::SetNextWindowSize({ 360, 480 }, ImGuiCond_Appearing);
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ID = ImGui::GetID("Binding");
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal("Binding", &tmpOpen, ImGuiWindowFlags_NoCollapse))
    {
        if (requestClose)
            ImGui::CloseCurrentPopup();
        /// @separator

		newFieldPopup.Draw();

        /// @begin Text
        ImRad::AlignedText(ImRad::Align_Left, "Expression:");
        /// @end Text

        /// @begin Input
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##expr", &expr, ImGuiInputTextFlags_None); 
        /// @end Input

	    /// @begin Text
		ImGui::Spacing();
        ImRad::AlignedText(ImRad::Align_Left, "Available fields:");
        /// @end Text

		/// @begin Text
		if (type != "std::string")
		{
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
			ImGui::SetNextItemWidth(-1);
			ImRad::AlignedText(ImRad::Align_Right, type.c_str());
			ImGui::PopStyleColor();
		}
		/// @end Text

		/// @begin Table
        if (ImGui::BeginTable("table1781119594448", 1, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_RowBg, { 0, -44 }))
        {
            ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_None, 0);

            for (size_t i = 0; i < vars.size(); ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID((int)i);
                /// @separator

				/// @begin Selectable
				ImGui::Selectable(ImRad::Format("{}", vars[i]).c_str(), false, ImGuiSelectableFlags_DontClosePopups);
				if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
					OnVarClicked();
				/// @end Selectable


                /// @separator
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        /// @end Table

        /// @begin Table
        ImGui::Spacing();
        if (ImGui::BeginTable("table1781119967744", 3, ImGuiTableFlags_None, { 0, -1 }))
        {
            ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("B", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableSetupColumn("C", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            /// @separator

            /// @begin Button
            if (ImGui::Button("New Field...", { 0, 0 }))
            {
                OnNewField();
            }
            /// @end Button

            /// @begin Button
            ImGui::TableNextColumn();
            if (ImGui::Button("OK", { 70, 0 }))
            {
                ClosePopup();
                callback(ImRad::Ok);
            }
            /// @end Button

            /// @begin Button
            ImGui::TableNextColumn();
            if (ImGui::Button("Cancel", { 70, 0 }) ||
                ImGui::IsKeyPressed(ImGuiKey_Escape))
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

void BindingDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
	callback = clb;
	requestClose = false;
	ImGui::OpenPopup(ID);

	// Add your init code here
	Refresh();
}

void BindingDlg::Refresh()
{
	vars = codeGen->GetVarExprs(type == "std::string" ? "" : type);
}

void BindingDlg::ClosePopup()
{
    requestClose = true;
}

void BindingDlg::OnNewField()
{
	newFieldPopup.mode = NewFieldPopup::NewField;
	newFieldPopup.varType = type == "std::string" ? "" : type;
	newFieldPopup.scope = "";
	newFieldPopup.codeGen = codeGen;
	newFieldPopup.OpenPopup([this] {
		Refresh();
		});
}

void BindingDlg::OnVarClicked()
{
	if (type == "std::string" || type == "std::vector<std::string>")
		expr += "{" + vars[ImGui::TableGetRowIndex()] + "}";
	else
		expr = vars[ImGui::TableGetRowIndex()];
}
