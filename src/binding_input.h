#pragma once
#include <array>
#include <optional>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include "stx.h"
#include "utils.h"
#include "binding.h"
#include "cppgen.h"
#include "ui_new_field.h"
#include "ui_binding.h"

template <class T>
std::string typeid_name()
{
	if constexpr (std::is_same_v<T, std::string>)
		return "std::string";
	else if (std::is_same_v<T, color32>)
		return "color32";
	else if (std::is_same_v<T, int>)
		return "int";
	else if (std::is_same_v<T, float>)
		return "float";
	else if (std::is_same_v<T, double>)
		return "double";
	else if (std::is_same_v<T, size_t>)
		return "size_t";
	else if (std::is_same_v<T, bool>)
		return "bool";
	else if (std::is_same_v<T, std::vector<std::string>>)
		return "std::vector<std::string>";
	else
	{
		std::string str = typeid(T).name();	
		if (str.size() >= 14 && !str.compare(str.size() - 14, 14, " __cdecl(void)"))
			str = str.substr(0, str.size() - 14) + "()";
		auto i = str.find(' ');
		if (i != std::string::npos)
			str.erase(0, i + 1);
		auto it = stx::find_if(str, [](char c) { return isalpha(c);});
		if (it != str.end())
			str.erase(0, it - str.begin());
		return str;
	}
}

template <class T>
inline bool BindingButton(const char* label, bindable<T>* val, const std::string& type, UIContext& ctx)
{
	bool has_var = !val->used_variables().empty();
	ImGui::PushStyleColor(ImGuiCol_ChildBg, has_var ? 0xff0080ff : 0xffffffff);
	float sp = (ImGui::GetFrameHeight() - 11) / 2;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sp);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + sp);
	ImGui::BeginChild(("child" + std::string(label)).c_str(), { 11, 11 }, true);
	bool pushed = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered();
	ImGui::EndChild();
	ImGui::PopStyleColor();
	if (pushed)
	{
		bindingDlg.codeGen = ctx.codeGen;
		bindingDlg.name = label;
		bindingDlg.expr = val->c_str();
		bindingDlg.type = type;
		bindingDlg.OpenPopup([val](ImRad::ModalResult) {
			*val->access() = bindingDlg.expr;
			});
		return true;
	}
	
	return false;
}

template <class T>
inline bool BindingButton(const char* label, bindable<T>* val, UIContext& ctx)
{
	return BindingButton(label, val, typeid_name<T>(), ctx);
}

inline bool InputBindable(const char* label, bindable<bool>* val, bool defval, UIContext& ctx)
{
	bool changed = false;
	if (ImGui::BeginCombo((label + std::string("combo")).c_str(), val->c_str()))
	{
		if (ImGui::Selectable("true", val->has_value() && val->value()))
		{
			*val = true; 
			changed = true;
		}
		if (ImGui::Selectable("false", val->has_value() && !val->value()))
		{
			*val = false; 
			changed = true;
		}
		ImGui::EndCombo();
	}

	return changed;
}

inline bool InputBindable(const char* label, bindable<color32>* val, UIContext& ctx)
{
	static const std::array<std::pair<const char*, color32>, 9> COLORS{ {
		{ "red", IM_COL32(255, 77, 77, 255) },
		{ "green", IM_COL32(40, 167, 69, 255) },
		{ "blue", IM_COL32(24, 144, 255, 255) },
		{ "aquamarine", IM_COL32(24, 162, 184, 255) },
		{ "magenta", IM_COL32(204, 121, 219, 255) },
		{ "yellow", IM_COL32(247, 189, 11, 255) },
		{ "orange", IM_COL32(250, 128, 68, 255) },
		{ "dark", IM_COL32(52, 58, 64, 255) },
		{ "light", IM_COL32(108, 117, 125, 255) },
	} };

	bool changed = false;
	std::optional<color32> clr;
	const char* str = val->c_str();
	if (val->has_value())
	{
		clr = val->value();
		auto it = stx::find_if(COLORS, [&](const auto& c) { return c.second == *clr; });
		if (it != COLORS.end())
			str = it->first;
	}
	if (ImGui::BeginCombo(label, str))
	{
		if (ImGui::Selectable(" ", val->empty()))
		{
			*val->access() = "";
			changed = true;
		}
		for (const auto& c : COLORS)
		{
			auto cc = ImGui::ColorConvertU32ToFloat4(c.second);
			ImGui::ColorButton("##color", cc, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder);
			ImGui::SameLine();
			if (ImGui::Selectable(c.first, clr && clr == c.second))
			{
				*val = c.second;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}

	return changed;
}

inline bool InputBindable(const char* label, bindable<std::string>* val, UIContext& ctx)
{
	bool changed = ImGui::InputText(label, val->access());
	
	return changed;
}

template <class T>
inline bool InputBindable(const char* label, bindable<T>* val, T defVal, UIContext& ctx)
{
	bool changed = ImGui::InputText(label, val->access());

	//disallow empty state except string values
	if (ImGui::IsItemDeactivatedAfterEdit() &&
		val->access()->empty())
	{
		*val = defVal;
	}

	return changed;
}

template <class T>
inline bool InputFieldName(const char* label, field_ref<T>* val, bool allowEmpty, UIContext& ctx)
{
	std::string type = typeid_name<T>();
	return InputFieldName(label, val->access(), type, allowEmpty, ctx);
}

template <>
inline bool InputFieldName(const char* label, field_ref<void>* val, bool allowEmpty, UIContext& ctx) = delete;

inline bool InputFieldName(const char* label, std::string* val, const std::string& type, bool allowEmpty, UIContext& ctx)
{
	bool changed = false;
	if (ImGui::BeginCombo(label, val->c_str()))
	{
		if (allowEmpty && ImGui::Selectable("None"))
		{
			*val = "";
			changed = true;
		}
		if (ImGui::Selectable("New Variable..."))
		{
			newFieldPopup.varType = type;
			newFieldPopup.codeGen = ctx.codeGen;
			newFieldPopup.mode = NewFieldPopup::NewField;
			newFieldPopup.OpenPopup([val] {
				*val = newFieldPopup.varName;
				});
		}
		
		ImGui::Separator();
		const auto& vars = ctx.codeGen->GetVarExprs(type);
		for (const auto& v : vars)
		{
			if (ImGui::Selectable(v.first.c_str(), v.first == val->c_str()))
			{
				*val = v.first;
				changed = true;
			}
		}
	
		ImGui::EndCombo();
	}
	return changed;
}

inline bool InputEvent(const char* label, event<>* val, UIContext& ctx)
{
	bool changed = false;
	if (ImGui::BeginCombo(label, val->c_str()))
	{
		if (ImGui::Selectable("None"))
		{
			*val->access() = "";
			changed = true;
		}
		if (ImGui::Selectable("New Method..."))
		{
			newFieldPopup.varType = "void()";
			newFieldPopup.codeGen = ctx.codeGen;
			newFieldPopup.mode = NewFieldPopup::NewEvent;
			newFieldPopup.OpenPopup([val] {
				*val->access() = newFieldPopup.varName;
				});
		}

		ImGui::Separator();
		const auto& vars = ctx.codeGen->GetVars();
		for (const auto& v : vars)
		{
			if (v.type == "void()" && ImGui::Selectable(v.name.c_str(), v.name == val->c_str()))
			{
				*val->access() = v.name;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}

inline bool CheckBoxFlags(flags_helper* flags)
{
	bool changed = false;
	for (const auto& id : flags->get_ids())
	{
		if (id.first == "")
			ImGui::Separator();
		else if (ImGui::CheckboxFlags(id.first.c_str(), flags->access(), id.second))
			changed = true;
	}
	return changed;
}
