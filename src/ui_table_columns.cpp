// Generated with imgui-designer 0.1
// github.com/xyz

#include "ui_table_columns.h"
#include "binding_input.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome6.h>

TableColumns tableColumns;

void TableColumns::OpenPopup()
{
	selRow = -1;
	ImGui::OpenPopup(ID);
}

void TableColumns::Draw()
{
	/// @begin TopWindow
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize({ 640, 420 });
	ID = ImGui::GetID("Table Columns");
	bool open = true;
	if (ImGui::BeginPopupModal("Table Columns", &open, ImGuiWindowFlags_NoCollapse))
	{
		/// @begin Table
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Indent(2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 2, 1 });
		if (ImGui::BeginTable("table2103590221472", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadInnerX, { -150, -10 }))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_None, 0);
			ImGui::TableSetupColumn("policy", ImGuiTableColumnFlags_None, 0);
			ImGui::TableSetupColumn("width", ImGuiTableColumnFlags_None, 0);
			ImGui::TableHeadersRow();

			for (auto& cd : columnData)
			{
				int i = int(&cd - columnData.data());
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::PushID(i);
				/// @separator

				float cy = ImGui::GetCursorPos().y;
				if (i == selRow)
					selY = cy;
				
				/// @begin Input
				ImGui::SetNextItemWidth(-1);
				ImGui::InputText("##lab", &cd.label);
				if (ImGui::IsItemFocused() || selRow < 0) {
					selRow = i;
				}
				/// @end Input
				
				/// @begin Combo
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-1);
				int tmp =
					(cd.flags & ImGuiTableColumnFlags_WidthFixed) ? 1 :
					(cd.flags & ImGuiTableColumnFlags_WidthStretch) ? 2 :
					0;
				if (ImGui::Combo("##f", &tmp, "Default\0Fixed\0Stretch"))
				{
					cd.flags &= ~(ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_WidthStretch);
					if (tmp == 2)
						cd.flags |= ImGuiTableColumnFlags_WidthStretch;
					else if (tmp == 1)
						cd.flags |= ImGuiTableColumnFlags_WidthFixed;
				}
				if (ImGui::IsItemClicked()) {
					selRow = i;
				}
				/// @end Combo
				
				/// @begin Input
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-1);
				ImGui::InputFloat("##w", &cd.width);
				if (ImGui::IsItemFocused()) {
					selRow = i;
				}
				/// @end Input
				
				ImGui::PopID();
			}

			ImGui::EndTable();
		}
		ImGui::PopStyleVar();
		/// @end Table

		ImGui::BeginDisabled(columnData.empty());
		
		/// @begin Button
		ImGui::SameLine(0, 3 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::BeginGroup();
		if (ImGui::Button("Insert Before", { 120, 0 }))
		{
			if (selRow < 0)
				columnData.insert(columnData.begin(), {});
			else
				columnData.insert(columnData.begin() + selRow, Table::ColumnData());
		}
		/// @end Button

		/// @begin Button
		ImGui::Spacing();
		ImGui::Spacing();
		if (ImGui::Button("Insert After", { 120, 0 }))
		{
			if (selRow < 0)
				columnData.push_back({});
			else
				columnData.insert(columnData.begin() + selRow + 1, Table::ColumnData());
			++selRow;
			
		}
		/// @end Button

		/// @begin Button
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::BeginDisabled(selRow < 0);
		if (ImGui::Button("Remove", { 120, 0 }))
		{
			columnData.erase(columnData.begin() + selRow);
			if (selRow == columnData.size())
				--selRow;
		}
		ImGui::EndDisabled();
		/// @end Button

		ImGui::EndDisabled();
		
		/// @begin Button
		//ImGui::Spacing();
		//ImGui::Spacing();
		ImGui::Dummy({ 0, 30 });
		if (ImGui::Button("OK", { 120, 30 }))
		{
			*target = columnData;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		/// @end Button

		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndGroup();
		
		ImGui::SetCursorPos({ ImGui::GetStyle().ItemSpacing.x, selY });
		ImGui::AlignTextToFramePadding();
		ImGui::Text(ICON_FA_CARET_RIGHT);

		ImGui::EndPopup();
	}
	/// @end TopWindow
}
