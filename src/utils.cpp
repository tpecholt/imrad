#include "utils.h"
#include "node_standard.h"
#include "binding_input.h"
#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#endif
#undef min
#undef max

bool ShellExec(const std::string& path)
{
#ifdef WIN32
    return (uintptr_t)ShellExecuteW(nullptr, L"open", u8path(path).c_str(), nullptr, nullptr, SW_SHOWNORMAL) > 32;
#else
    return system(("xdg-open " + path).c_str()) == 0;
#endif
}

int DefaultCharFilter(ImGuiInputTextCallbackData* data)
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

bool IsAscii(std::string_view str)
{
    return !stx::count_if(str, [](char c) { return c < 0; });
}

std::string Trim(std::string_view str)
{
    size_t i1, i2;
    for (i1 = 0; i1 < str.size(); ++i1)
        if (str[i1] >= 128 || !std::isspace(str[i1]))
            break;
    for (i2 = str.size() - 1; i2 < str.size(); --i2)
        if (str[i2] >= 128 || !std::isspace(str[i2]))
            break;
    return std::string(str.substr(i1, i2 - i1 + 1));
}

std::string Replace(std::string_view s, std::string_view sold, std::string_view snew)
{
    std::string out;
    size_t i = 0;
    while (i < s.size()) {
        size_t j = s.find(sold, i);
        if (j == std::string::npos)
            break;
        out += s.substr(i, j - i);
        out += snew;
        i = j + sold.size();
    }
    out += s.substr(i);
    return out;
}

fs::path u8path(std::string_view s)
{
#if __cplusplus >= 202202L
    return fs::path((const char8_t*)s.data(), s.size());
#else
    return fs::u8path(s);
#endif
}

std::string u8string(const fs::path& p)
{
#if __cplusplus >= 202202L
    return std::string((const char*)p.u8string().data());
#else
    return p.u8string();
#endif
}

std::string generic_u8string(const fs::path& p)
{
#if __cplusplus >= 202202L
    return std::string((const char*)p.generic_u8string().data());
#else
    return p.generic_u8string();
#endif
}

bool path_cmp(const std::string& a, const std::string& b)
{
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i)
    {
        int ca = a[i];
        int cb = b[i];
        if (ca >= 'a' && ca <= 'z')
            ca += 'A' - 'a';
        if (cb >= 'a' && cb <= 'z')
            cb += 'A' - 'a';
        if (ca != cb)
            return ca < cb;
    }
    return b.size() > a.size();
}