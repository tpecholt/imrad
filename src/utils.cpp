#include "utils.h"
#include "node_standard.h"
#include "binding_input.h"
#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#endif

void ShellExec(const std::string& path)
{
#ifdef WIN32
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    system(("xdg-open " + path).c_str());
#endif
}

int InputTextCharExprFilter(ImGuiInputTextCallbackData* data)
{
    //always map numpad decimal point to . (all expressions are in C)
    //int localeDP = *localeconv()->decimal_point;
    if (data->EventChar == ',' && ImGui::IsKeyDown(ImGuiKey_KeypadDecimal))
        data->EventChar = '.';
    return 0;
}

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
