#include "ui_message_box.h"
#include "IconsFontAwesome6.h"

MessageBox messageBox;

void MessageBox::OpenPopup(std::function<void(ImRad::ModalResult)> f)
{
	callback = std::move(f);
	ImGui::OpenPopup(ID);
}

void MessageBox::Draw()
{
	ID = ImGui::GetID("###MessageBox");
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	bool open = true;
	if (ImGui::BeginPopupModal((title + "###MessageBox").c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		wasOpen = ID;
		if (error != "")
		{
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(192, 0, 0, 255));
			ImGui::Text(ICON_FA_TRIANGLE_EXCLAMATION " ");
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::TextWrapped("%s", message.c_str());
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(164, 164, 164, 255));
			ImGui::InputTextMultiline("##err", &error, { 500, 300 }, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor();
		}
		else
		{
			ImGui::BeginChild("ch", { 300, 50 });
			ImGui::TextWrapped("%s", message.c_str());
			ImGui::EndChild();
		}
		ImGui::Spacing();
		ImGui::Spacing();

		int n = (bool)(buttons & ImRad::Ok) + (bool)(buttons & ImRad::Cancel) + (bool)(buttons & ImRad::Yes) + (bool)(buttons & ImRad::No);
		float w = (n * 80) + (n - 1) * ImGui::GetStyle().ItemSpacing.x;
		float x = (ImGui::GetContentRegionAvail().x + ImGui::GetStyle().FramePadding.x - w) / 2.f;
		x += ImGui::GetStyle().WindowPadding.x;
		float y = ImGui::GetCursorPosY();
		if (buttons & ImRad::Ok)
		{
			if (ImGui::IsWindowAppearing())
				ImGui::SetKeyboardFocusHere();
			ImGui::SetCursorPos({ x, y });
			if (ImGui::Button("OK", { 80, 30 }) ||
				(ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)) ||
				(buttons == ImRad::Ok && ImGui::IsKeyPressed(ImGuiKey_Escape)))
			{
				ImGui::CloseCurrentPopup();
				callback(ImRad::Ok);
			}
			x += 80 + ImGui::GetStyle().ItemSpacing.x;
		}
		if (buttons & ImRad::Yes)
		{
			ImGui::SetCursorPos({ x, y });
			if (ImGui::Button("Yes", { 80, 30 })) {
				ImGui::CloseCurrentPopup();
				callback(ImRad::Yes);
			}
			x += 80 + ImGui::GetStyle().ItemSpacing.x;
		}
		if (buttons & ImRad::No)
		{
			ImGui::SetCursorPos({ x, y });
			if (ImGui::Button("No", { 80, 30 })) {
				ImGui::CloseCurrentPopup();
				callback(ImRad::No);
			}
			x += 80 + ImGui::GetStyle().ItemSpacing.x;
		}
		if (buttons & ImRad::Cancel)
		{
			ImGui::SetCursorPos({ x, y });
			if (ImGui::Button("Cancel", { 80, 30 }) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				ImGui::CloseCurrentPopup();
				//callback(ImRad::Cancel);
			}
			x += 80 + ImGui::GetStyle().ItemSpacing.x;
		}

		ImGui::EndPopup();
	}
	else if (wasOpen == ID)
	{
		//hack - clear fields for next invocation
		wasOpen = 0;
		error = "";
		title = "title";
		message = "";
		buttons = ImRad::Ok;
	}
}