#include "binding.h"
#include "cppgen.h"
#include <imgui.h>

template <class T>
template <class U>
U field_ref<T>::eval(const UIContext& ctx) const 
{
	if (empty())
		return {};
	const auto* var = ctx.codeGen->GetVar(str);
	if (!var)
		return {};
	U val;
	std::istringstream is(var->init);
	if (is >> val)
		return val;
	return {};
}

float direct_val<dimension>::eval_px(const UIContext& ctx) const
{
	return val * ctx.unitFactor;
}

float direct_val<pzdimension>::eval_px(const UIContext& ctx) const 
{
	return val * ctx.unitFactor;
}

ImVec2 direct_val<pzdimension2>::eval_px(const UIContext& ctx) const
{
	return { val[0] * ctx.unitFactor, val[1] * ctx.unitFactor };
}

float bindable<dimension>::eval_px(const UIContext& ctx) const
{
	if (empty())
		return {};
	else if (has_value())
		return value() * ctx.unitFactor;
	else {
		const auto* var = ctx.codeGen->GetVar(str);
		if (var) {
			float val;
			std::istringstream is(var->init);
			if (is >> val)
				return val;
		}
	}
	return {};
}

color32 bindable<color32>::eval(const UIContext& ctx) const 
{
	if (empty())
		return {};
	std::istringstream is(str);
	color32 val = ImGui::ColorConvertFloat4ToU32(ctx.style.Colors[ImGuiCol_Text]);
	if (!(is >> val)) {
		int idx = style_color();
		if (idx >= 0)
			val = ImGui::ColorConvertFloat4ToU32(ctx.style.Colors[idx]);
	}
	return val;
}
