#pragma once
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_internal.h>

namespace ImRad {

enum ModalResult {
	None,
	Ok,
	Cancel,
	Yes,
	No,
	All,
	ModalResult_Count
};

enum Align {
	Align_Left,
	Align_Right,
	Align_Center,
};

inline void AlignedText(Align alignment, const char* label)
{
	//todo: align each line 
	const auto& nextItemData = ImGui::GetCurrentContext()->NextItemData;
	float x1 = ImGui::GetCursorPosX();
	if (nextItemData.Flags & ImGuiNextItemDataFlags_HasWidth)
	{
		float w = nextItemData.Width;
		if (w < 0)
			w += ImGui::GetContentRegionAvail().x;
		
		float tw = ImGui::CalcTextSize(label, nullptr, false, w).x;
		float dx = 0;
		if (alignment == Align_Center)
			dx = (w - tw) / 2.f;
		else if (alignment == Align_Right)
			dx = w - tw;
		
		ImGui::SetCursorPosX(x1 + dx);
		ImGui::PushTextWrapPos(x1 + w);
		ImGui::TextWrapped(label);
		ImGui::PopTextWrapPos();
		ImGui::SameLine(x1 + w);
		ImGui::NewLine();
	}
	else
	{
		ImGui::TextUnformatted(label);
	}
}

inline bool Combo(const char* label, int* curr, const std::vector<std::string>& items, int maxh = -1)
{
	std::vector<const char*> citems(items.size());
	for (size_t i = 0; i < items.size(); ++i)
		citems[i] = items[i].c_str();
	return ImGui::Combo(label, curr, citems.data(), (int)citems.size(), maxh);
}

//this is a poor man Format and should be implemented using c++20 format or fmt library
template <class A1, class... A>
std::string Format(std::string_view fmt, A1&& arg, A... args)
{
	//todo
	std::string s;
	for (size_t i = 0; i < fmt.size(); ++i)
	{
		if (fmt[i] == '{') {
			if (i + 1 == fmt.size())
				break;
			if (fmt[i + 1] == '{')
				s += '{';
			else {
				auto j = fmt.find('}', i + 1);
				if (j == std::string::npos)
					break;
				if constexpr (std::is_same_v<std::decay_t<A1>, std::string>)
					s += arg;
				else
					s += std::to_string(arg);
				return s + Format(fmt.substr(j + 1), args...);
			}
		}
		else
			s += fmt[i];
	}
	return s;
}

inline std::string Format(std::string_view fmt)
{
	return std::string(fmt);
}

}