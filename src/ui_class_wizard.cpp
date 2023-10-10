// Generated with imgui-designer 0.1
// github.com/xyz
#include "ui_class_wizard.h"
#include "ui_new_field.h"
#include "ui_message_box.h"
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome6.h>

ClassWizard classWizard;

//todo: need to call our own copy otherwise ImGui thinks messageBox is not ClassWizard's child
//and hides CW when OpenPopup
//static MessageBox msgBox; 

void ClassWizard::OpenPopup()
{
	requestClose = false;
	ImGui::OpenPopup(ID);
	Refresh();
}

void ClassWizard::ClosePopup()
{
	requestClose = true;
}

void ClassWizard::Refresh()
{
	className = codeGen->GetName();
	varName = codeGen->GetVName();
	
	std::string stype = className;
	if (stypeIdx < stypes.size())
		stype = stypes[stypeIdx];
	stypes = codeGen->GetStructTypes();
	stypes.insert(stypes.begin(), className);
	stypes.push_back("New struct...");
	stypeIdx = stx::find(stypes, stype) - stypes.begin();
	if (stypeIdx == stypes.size())
		stypeIdx = 0;
	
	fields = codeGen->GetVars(stypeIdx ? stypes[stypeIdx] : "");
	stx::erase_if(fields, [](CppGen::Var& var) {
		//CppGen exports user code as-is so no modification allowed here
		if (var.flags & CppGen::Var::UserCode)
			return true;
		//skip member functions 
		if (var.type.back() == ')')
			return true;
		return false;
		});

	used.clear();
	FindUsed(root, used);
}

void ClassWizard::FindUsed(UINode* node, std::vector<std::string>& used)
{
	for (int i = 0; i < 2; ++i)
	{
		const auto& props = i ? node->Events() : node->Properties();
		for (const auto& p : props) {
			if (!p.property)
				continue;
			auto vars = p.property->used_variables();
			for (const auto& var : vars) {
				assert(var.find_first_of("[.") == std::string::npos);
				used.push_back(var);
			}
		}
	}
	for (const auto& child : node->children)
		FindUsed(child.get(), used);
}

void ClassWizard::Draw()
{
	const float BWIDTH = 150;
	bool doRenameField = false;
	/// @begin TopWindow
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12, 12 });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 12, 5 });
	ImGui::SetNextWindowSize({ 800, 600 });
	ID = ImGui::GetID("Class Wizard");
	bool tmpOpen = true;
	if (ImGui::BeginPopupModal("Class Wizard", &tmpOpen, ImGuiWindowFlags_None))
	{
		if (requestClose) ImGui::CloseCurrentPopup();
		/// @separator

		newFieldPopup.Draw();
		messageBox.Draw(); //doesn't work from here

		/// @begin Text
		ImGui::Text("Class Name");
		/// @end Text

		/*/// @begin Text
		ImGui::SameLine(0, 6 * ImGui::GetStyle().IndentSpacing);
		ImGui::Text("Variable Name");
		/// @end Text
		*/

		/// @begin ComboBox
		ImGui::SetNextItemWidth(200);
		//ImGui::InputText("##className", &className, ImGuiInputTextFlags_None);
		if (ImGui::BeginCombo("##className", stypes[stypeIdx].c_str()))
		{
			for (size_t i = 0; i + 1 < stypes.size(); ++i)
			{
				if (ImGui::Selectable(stypes[i].c_str(), i == stypeIdx)) {
					stypeIdx = i;
					fields = codeGen->GetVars(stypeIdx ? stypes[stypeIdx] : "");
				}
			}
			if (ImGui::Selectable(stypes.back().c_str(), false)) { //New struct
				newFieldPopup.codeGen = codeGen;
				newFieldPopup.mode = NewFieldPopup::NewStruct;
				newFieldPopup.OpenPopup([this] {
					*modified = true;
					Refresh();
					});
			}
			ImGui::EndCombo();
		}
		/// @end ComboBox

		/*/// @begin Input
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200);
		ImGui::BeginDisabled(stypeIdx != 0);
		std::string dummy;
		ImGui::InputText("##varName", stypeIdx ? &dummy : &varName, ImGuiInputTextFlags_CharsNoBlank);
		ImGui::EndDisabled();
		/// @end Input
		*/
		/// @begin Button
		ImGui::SameLine();
		if (ImGui::Button(" Rename "))
		{
			newFieldPopup.codeGen = codeGen;
			newFieldPopup.mode = stypeIdx ? NewFieldPopup::RenameField : NewFieldPopup::RenameWindow;
			newFieldPopup.varOldName = stypes[stypeIdx];
			newFieldPopup.OpenPopup([this] {
				*modified = true;
				Refresh();
				});
		}
		/// @end Button

		/// @begin Child
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::BeginChild("child2551", { 0, -45 }, true);
		float autoSizeW = (ImGui::GetContentRegionAvail().x - 150) / 1;
		ImGui::Columns(2, "", false);
		ImGui::SetColumnWidth(0, autoSizeW);
		ImGui::SetColumnWidth(1, BWIDTH + ImGui::GetStyle().ItemSpacing.x);

		/// @begin Text
		ImGui::Text("Fields:");
		/// @end Text

		/// @begin Table
		if (ImGui::BeginTable("table", 4, ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchProp /*| ImGuiTableFlags_RowBg*/, { 0, -1 }))
		{
			ImGui::TableSetupColumn("Section", ImGuiTableColumnFlags_WidthFixed, 20);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 1);
			ImGui::TableSetupColumn("Init", ImGuiTableColumnFlags_WidthStretch, 1);
			ImGui::TableHeadersRow();
		
			for (int i = 0; i < fields.size(); ++i)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::PushID(i);
				/// @separator

				const auto& var = fields[i];
				
				//ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, 255, 255));
				//ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(192, 192, 192, 255));
				
				bool unused = !stx::count(used, var.name);
				if (unused)
					ImGui::PushStyleColor(ImGuiCol_Text, 0xff400040);
				
				const char* icon =
					(var.flags & CppGen::Var::Impl) ? ICON_FA_LOCK :
					(var.flags & CppGen::Var::Interface) ? ICON_FA_CUBE :
					"";
				const char* tooltip =
					(var.flags & CppGen::Var::Impl) ? "private" :
					(var.flags & CppGen::Var::Interface) ? "public" :
					"uknown";
				/// @begin Selectable
				if (ImGui::Selectable(icon, selRow == i, ImGuiSelectableFlags_SpanAllColumns))
					selRow = i;
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
					doRenameField = true;
				//if (ImGui::IsItemHovered() && ImGui::GetMousePos().x < 20)
				//	ImGui::SetTooltip(tooltip);
				/// @end Selectable

				/// @begin Selectable
				ImGui::TableNextColumn();
				ImGui::Selectable(var.name.c_str(), selRow == i);
				/// @end Selectable

				/// @begin Selectable
				ImGui::TableNextColumn();
				ImGui::Selectable(var.type.c_str(), selRow == i);
				/// @end Selectable

				/// @begin Selectable
				ImGui::TableNextColumn();
				ImGui::Selectable(var.init.c_str(), selRow == i);
				/// @end Selectable

				if (unused)
					ImGui::PopStyleColor();
				
				/// @separator
				ImGui::PopID();
			}

			ImGui::EndTable();
		}
		/// @end Table

		/// @begin Button
		ImGui::NextColumn();
		if (ImGui::Button("Add Field...", { BWIDTH, 0 }))
		{
			newFieldPopup.codeGen = codeGen;
			newFieldPopup.mode = NewFieldPopup::NewField;
			newFieldPopup.scope = stypeIdx ? stypes[stypeIdx] : "";
			newFieldPopup.varType = "";
			newFieldPopup.OpenPopup([this] {
				*modified = true;
				Refresh();
				});
		}
		/// @end Button

		/// @begin Button
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::BeginDisabled(selRow < 0 || selRow >= (int)fields.size());
		if (ImGui::Button("Rename Field...", { BWIDTH, 0 }) ||
			(!ImRad::IsItemDisabled() && doRenameField))
		{
			newFieldPopup.codeGen = codeGen;
			newFieldPopup.mode = NewFieldPopup::RenameField;
			newFieldPopup.scope = stypeIdx ? stypes[stypeIdx] : "";
			newFieldPopup.varOldName = fields[selRow].name;
			newFieldPopup.OpenPopup([this] {
				*modified = true;
				root->RenameFieldVars(newFieldPopup.varOldName, newFieldPopup.varName);
				Refresh();
				});
		}
		ImGui::EndDisabled();
		/// @end Button

		/// @begin Button
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::BeginDisabled(selRow < 0 || selRow >= (int)fields.size());
		if (ImGui::Button("Remove Field", { BWIDTH, 0 }))
		{
			std::string name = fields[selRow].name;
			if (!stypeIdx && stx::count(used, name)) 
			{
				messageBox.title = "Remove variable";
				messageBox.message = "Remove used variable '" + name + "' ?";
				messageBox.buttons = ImRad::Yes | ImRad::No;
				messageBox.OpenPopup([this,name](ImRad::ModalResult mr) {
					if (mr == ImRad::Yes) {
						*modified = true;
						codeGen->RemoveVar(name);
						Refresh();
					}
					});
			}
			else 
			{
				*modified = true;
				codeGen->RemoveVar(name, stypeIdx ? stypes[stypeIdx] : "");
				Refresh();
			}
		}
		ImGui::EndDisabled();
		/// @end Button

		/// @begin Button
		ImGui::Spacing();
		ImGui::BeginDisabled(stypeIdx);
		if (ImGui::Button("Remove Unused", { BWIDTH, 0 }))
		{
			for (const auto& fi : fields)
			{
				if (!stx::count(used, fi.name)) {
					*modified = true;
					codeGen->RemoveVar(fi.name);
				}
			}
			Refresh();
		}
		ImGui::EndDisabled();
		/// @end Button

		ImGui::EndChild();
		/// @end Child

		/// @begin Dummy
		ImGui::Spacing();
		ImGui::Dummy({ ImGui::GetContentRegionAvail().x - 110, 0 });
		/// @end Dummy

		/// @begin Button
		ImGui::SameLine();
		if (ImGui::Button("OK", { 100, 35 }) ||
			ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ClosePopup();
		}
		/// @end Button

		/// @separator
		ImGui::End();
		/// @end TopWindow
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
}