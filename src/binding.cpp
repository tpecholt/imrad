#include "binding.h"
#include "cppgen.h"
#include <imgui.h>

std::string CodeShortcut(std::string_view sh)
{
    if (sh.empty())
        return "";
    size_t i = -1;
    std::string code;
    while (true)
    {
        size_t j = sh.find_first_of("+-", i + 1);
        if (j == std::string::npos)
            j = sh.size();
        std::string key(sh.substr(i + 1, j - i - 1));
        if (key.empty() && j < sh.size()) //like in ctrl-+
            key = sh[j];
        stx::erase_if(key, [](char c) { return std::isspace(c); });
        if (key.empty())
            break;
        std::string lk = key;
        stx::for_each(lk, [](char& c) { c = std::tolower(c); });
        //todo
        if (lk == "ctrl")
            code += "ImGuiMod_Ctrl";
        else if (lk == "alt")
            code += "ImGuiMod_Alt";
        else if (lk == "shift")
            code += "ImGuiMod_Shift";
        else if (key == "+")
            code += "ImGuiKey_Equal";
        else if (key == "-")
            code += "ImGuiKey_Minus";
        else if (key == ".")
            code += "ImGuiKey_Period";
        else if (key == ",")
            code += "ImGuiKey_Comma";
        else if (key == "/")
            code += "ImGuiKey_Slash";
        else
            code += std::string("ImGuiKey_")
            + (char)std::toupper(key[0])
            + key.substr(1);
        code += " | ";
        i = j;
        if (j == sh.size())
            break;
    }
    if (code.size())
        code.resize(code.size() - 3);
    return code;
}

std::string ParseShortcut(std::string_view line)
{
    std::string sh;
    size_t i = 0;
    while (true)
    {
        size_t j1 = line.find("ImGuiKey_", i);
        size_t j2 = line.find("ImGuiMod_", i);
        if (j1 == std::string::npos && j2 == std::string::npos)
            break;
        if (j1 < j2)
            i = j1 + 9;
        else
            i = j2 + 9;
        for (; i < line.size(); ++i)
        {
            if (!isalnum(line[i]))
                break;
            sh += line[i];
        }
        sh += "+";
    }
    if (sh.size())
        sh.pop_back();
    return sh;
}

float direct_val<dimension>::eval_px(const UIContext& ctx) const
{
    return val * ctx.zoomFactor;
}

float direct_val<pzdimension>::eval_px(const UIContext& ctx) const 
{
    return val * ctx.zoomFactor;
}

ImVec2 direct_val<pzdimension2>::eval_px(const UIContext& ctx) const
{
    return { val[0] * ctx.zoomFactor, val[1] * ctx.zoomFactor };
}

float bindable<dimension>::eval_px(int axis, const UIContext& ctx) const
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
            return val;
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

color32 bindable<color32>::eval(int col, const UIContext& ctx) const 
{
    if (empty())
        return {};
    std::istringstream is(str);
    color32 val = ImGui::ColorConvertFloat4ToU32(ctx.style.Colors[col]);
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
