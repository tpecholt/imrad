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
	if constexpr (std::is_same_v<T, void>)
		return "";
	else if (std::is_same_v<T, std::string>)
		return "std::string";
	else if (std::is_same_v<T, color32>)
		return "color4";
	else if (std::is_same_v<T, dimension>)
		return "float";
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
	else if (std::is_same_v<T, ImVec2> || std::is_same_v<T, dimension2>)
		return "ImVec2";
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
	auto nextData = ImGui::GetCurrentContext()->NextItemData; //copy
	int styleClr = val->style_color();
	ImVec4 buttonClr = styleClr >= 0 ? ctx.style.Colors[styleClr] :
		val->has_value() ? ImGui::ColorConvertU32ToFloat4(val->value()) :
		ImGui::GetStyle().Colors[ImGuiCol_WindowBg]; //clear background
	static std::string lastColor; //could be bound
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
	if (ImGui::ColorButton(label, buttonClr, ImGuiColorEditFlags_NoTooltip))
	{
		lastColor = val->c_str();
		ImGui::OpenPopup(label);
	}
	ImGui::PopStyleColor();
	
	ImGui::SameLine();
	ImVec2 sz{ 0, 0 };
	if (nextData.Flags & ImGuiNextItemDataFlags_HasWidth)
	{
		sz.x = nextData.Width;
		if (sz.x < 0)
			sz.x += ImGui::GetContentRegionAvail().x;
	}
	std::string clrName = styleClr >= 0 ? ImGui::GetStyleColorName(styleClr) : val->c_str();
	ImGui::Selectable(clrName.c_str(), false, ImGuiSelectableFlags_Disabled, sz);

	static color32 COLORS[]{
		IM_COL32(255, 255, 255, 255),
		IM_COL32(246, 246, 194, 255),
		IM_COL32(202, 23, 55, 255),
		IM_COL32(246, 58, 56, 255),
		IM_COL32(162, 34, 31, 255),
		IM_COL32(7, 17, 115, 255),
		IM_COL32(159, 215, 252, 255),
		IM_COL32(15, 63, 25, 255),
		IM_COL32(96, 231, 66, 255),
		IM_COL32(205, 255, 202, 255),
		IM_COL32(246, 253, 85, 255),
		IM_COL32(248, 117, 0, 255),
		IM_COL32(244, 180, 118, 255),
		IM_COL32(104, 102, 90, 255),
		IM_COL32(165, 166, 160, 255),
		IM_COL32(54, 8, 81, 255),
		IM_COL32(148, 35, 213, 255),
		IM_COL32(248, 181, 248, 255),
		IM_COL32(204, 22, 182, 255),
		IM_COL32(219, 190, 2, 255),
		IM_COL32(10, 226, 211, 255),
		IM_COL32(2, 251, 171, 255),
		IM_COL32(143, 105, 69, 255),
		IM_COL32(59, 33, 8, 255),
		IM_COL32(0, 0, 0, 255),
	};
	bool changed = false;
	static std::string lastOpen;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8, 8 });
	ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos() + ImVec2{ ImGui::GetContentRegionAvail().x, 0 }, ImGuiCond_Always, { 1, 0 });
	if (ImGui::BeginPopup(label, ImGuiWindowFlags_AlwaysAutoResize))
	{
		lastOpen = label;
		*val->access() = lastColor;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 5, 5 });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
		if (ImGui::Button("Automatic", { -30, ImGui::GetFrameHeight() }))
		{
			changed = true;
			lastColor = "";
			*val = color32();
		}
		if (ImGui::IsItemHovered())
		{
			*val = color32();
		}
		ImGui::SameLine(0, 0);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		int clri = val->style_color();
		if (ImGui::BeginCombo("##cmb", "", ImGuiComboFlags_NoPreview))
		{
			for (int i = 0; i < ImGuiCol_COUNT; ++i)
			{
				auto cc = ctx.style.Colors[i];
				auto ccc = ImGui::ColorConvertFloat4ToU32(cc);
				ImGui::ColorButton("##color", cc, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder);
				ImGui::SameLine();
				if (ImGui::Selectable(ImGui::GetStyleColorName(i), clri == i))
				{
					changed = true;
					val->set_style_color(i);
					lastColor = *val->access();
					break;
				}
				ImRect r{ ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
				r.Min.y -= ImGui::GetStyle().ItemSpacing.y;
				if (r.Contains(ImGui::GetMousePos())) //more stable than IsItemHovered
				{
					val->set_style_color(i);
				}
			}
			ImGui::EndCombo();
		}
		for (int i = 0; i < stx::ssize(COLORS); ++i)
		{
			ImGui::PushID(i);
			ImGui::PushStyleColor(ImGuiCol_Button, COLORS[i]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COLORS[i]);
			if (ImGui::Button("##clr", { 30, 30 }))
			{
				changed = true;
				*val = COLORS[i];
				lastColor = *val->access();
			}
			if (ImGui::IsItemHovered())
			{
				*val = COLORS[i];
			}
			ImGui::PopStyleColor(2);
			ImGui::PopID();
			if ((i + 1) % 5)
				ImGui::SameLine();
		}
		if (changed)
			ImGui::CloseCurrentPopup();
		ImGui::PopStyleVar(2);
		ImGui::EndPopup();
	}
	else if (lastOpen == label)
	{
		lastOpen = "";
		//return last value if the current color was hovered only
		*val->access() = lastColor;
	}
	ImGui::PopStyleVar();

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
inline bool InputFieldRef(const char* label, field_ref<T>* val, const std::string& type, bool allowEmpty, UIContext& ctx)
{
	bool changed = false;
	if (ImGui::BeginCombo(label, val->c_str()))
	{
		if (allowEmpty && ImGui::Selectable("None"))
		{
			*val->access() = "";
			changed = true;
		}
		
		if (!val->empty() && ImGui::Selectable("Rename..."))
		{
			auto vars = val->used_variables();
			assert(vars.size() == 1);
			newFieldPopup.codeGen = ctx.codeGen;
			newFieldPopup.varOldName = vars[0];
			newFieldPopup.mode = NewFieldPopup::RenameField;
			newFieldPopup.OpenPopup([val, root=ctx.root] {
				root->RenameFieldVars(newFieldPopup.varOldName, newFieldPopup.varName);
				});
		}
		
		if (ImGui::Selectable("New Variable..."))
		{
			newFieldPopup.varType = type;
			newFieldPopup.codeGen = ctx.codeGen;
			newFieldPopup.mode = NewFieldPopup::NewField;
			newFieldPopup.OpenPopup([val] {
				*val->access() = newFieldPopup.varName;
				});
		}
		
		ImGui::Separator();
		const auto& vars = ctx.codeGen->GetVarExprs(type);
		for (const auto& v : vars)
		{
			if (ImGui::Selectable(v.first.c_str(), v.first == val->c_str()))
			{
				*val->access() = v.first;
				changed = true;
			}
		}
	
		ImGui::EndCombo();
	}
	return changed;
}

template <class T>
inline bool InputFieldRef(const char* label, field_ref<T>* val, bool allowEmpty, UIContext& ctx)
{
	std::string type = typeid_name<T>();
	return InputFieldRef(label, val, type, allowEmpty, ctx);
}

template <>
inline bool InputFieldRef(const char* label, field_ref<void>* val, bool allowEmpty, UIContext& ctx) = delete;


template <class Arg>
inline bool InputEvent(const char* label, event<Arg>* val, UIContext& ctx)
{
	bool changed = false;
	std::string type = "void(" + typeid_name<Arg>() + ")";
	if (ImGui::BeginCombo(label, val->c_str()))
	{
		if (ImGui::Selectable("None"))
		{
			*val->access() = "";
			changed = true;
		}
		if (ImGui::Selectable("New Method..."))
		{
			newFieldPopup.varType = type;
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
			if (v.type == type && ImGui::Selectable(v.name.c_str(), v.name == val->c_str()))
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
