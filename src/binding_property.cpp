#include "binding_property.h"
#include "cppgen.h"
#include <imgui.h>

float direct_val<dimension_t>::eval_px(const UIContext& ctx) const
{
    return val * ctx.zoomFactor;
}

float direct_val<pzdimension_t>::eval_px(const UIContext& ctx) const 
{
    return val * ctx.zoomFactor;
}

ImVec2 direct_val<pzdimension2_t>::eval_px(const UIContext& ctx) const
{
    return { val[0] * ctx.zoomFactor, val[1] * ctx.zoomFactor };
}

float bindable<dimension_t>::eval_px(int axis, const UIContext& ctx) const
{
    if (empty()) {
        return 0;
    }
    else if (stretched()) {
        return ctx.stretchSize[axis];
    }
    else if (has_value()) {
        return value() * ctx.zoomFactor;
    }
    else if (const auto* var = ctx.codeGen->GetVar(str)) {
        float val;
        std::istringstream is(var->init);
        if (is >> val)
            return val * ctx.zoomFactor;
        else
            return 0;
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
        return ret * ctx.zoomFactor;
    }
}

ImU32 bindable<color_t>::eval(int defClr, const UIContext& ctx) const 
{
    if (empty()) //default color
        return ImGui::ColorConvertFloat4ToU32(ctx.style.Colors[defClr]);
    
    int idx = style_color();
    if (idx >= 0)
        return ImGui::ColorConvertFloat4ToU32(ctx.style.Colors[idx]);
    
    ImU32 clr;
    std::istringstream is(str);
    if (is.get() == '0' && is.get() == 'x' && is >> std::hex >> clr)
        return clr;
    
    return ImGui::ColorConvertFloat4ToU32(ctx.style.Colors[defClr]);
}

std::string bindable<font_name_t>::eval(const UIContext&) const
{
    if (!has_value())
        return "";
    return str.substr(22, str.size() - 22 - 2);
}
