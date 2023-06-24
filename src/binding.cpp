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
