#include "binding.h"
#include "cppgen.h"
#include <imgui.h>

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
	if (empty()) {
		return 0;
	}
	else if (has_value()) {
		return value() * ctx.unitFactor;
	}
	else if (const auto* var = ctx.codeGen->GetVar(str)) {
		float val;
		std::istringstream is(var->init);
		if (is >> val)
			return val;
	}
	else {
		//experimental - currently parses:
		//num*dp
		//cond ? num1*dp : num2*dp
		//extracts min value as that may be the preferrable one when some widgets show/hide 
		//e.g. -1 vs -20 or 20 vs 50
		std::istringstream is(str);
		int state = 1; //0 - skip, 1 - number, 2 - *, 3 - dp
		float val = 0, ret = 0;
		for (cpp::token_iterator it(is), ite; it != ite; ++it) {
			if (*it == "?") {
				state = 1;
			}
			else if (*it == ":") {
				if (state >= 2)
					ret = std::min(ret ? ret : 1e9f, val);
				state = 1;
			}
			else if (state == 1) {
				std::istringstream iss(*it);
				if (iss >> val)
					state = 2;
				else
					state = 0;
			}
			else if (state == 2) {
				if (*it == "*")
					state = 3;
				else
					state = 0;
			}
			else if (state == 3) {
				if (*it != "dp")
					state = 0;
			}
		}
		if (state >= 2)
			ret = std::min(ret ? ret : 1e9f, val);
		return ret * ctx.unitFactor;
	}
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

std::string bindable<font_name>::eval(const UIContext&) const
{
	if (!has_value())
		return "";
	return str.substr(22, str.size() - 22 - 2);
}
