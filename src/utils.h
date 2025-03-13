#pragma once

#include <cmath>
#include <imgui.h>
#include <iostream>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

inline const std::string VER_STR = "ImRAD 0.8";
inline const std::string GITHUB_URL = "https://github.com/tpecholt/imrad";


inline ImVec2 operator+ (const ImVec2& a, const ImVec2& b)
{
    return ImVec2(a.x + b.x, a.y + b.y);
}

inline ImVec2 operator- (const ImVec2& a, const ImVec2& b)
{
    return ImVec2(a.x - b.x, a.y - b.y);
}

inline ImVec2 operator* (const ImVec2& a, float f)
{
    return { a.x * f, a.y * f };
}

inline ImVec2 operator* (float f, const ImVec2& a)
{
    return a * f;
}

inline ImVec2 operator/ (const ImVec2& a, float f)
{
    return { a.x / f, a.y / f };
}

inline ImVec2& operator+= (ImVec2& a, const ImVec2& b)
{
    a = a + b;
    return a;
}

inline ImVec2& operator*= (ImVec2& a, float f)
{
    a = a * f;
    return a;
}

inline ImVec2& operator/= (ImVec2& a, float f)
{
    a = a / f;
    return a;
}

inline float Norm(const ImVec2& a)
{
    return std::sqrt(a.x * a.x + a.y*a.y);
}

//-----------------------------------------------------------------------

inline std::string operator+ (const std::string& s, std::string_view t)
{
    std::string ss = s;
    ss += t;
    return ss;
}

inline std::string operator+ (std::string_view t, const std::string& s)
{
    std::string ss = s;
    ss.insert(0, t);
    return ss;
}

inline std::ostream& operator<< (std::ostream& os, std::string_view t)
{
    os.write(t.data(), t.size());
    return os;
}

//----------------------------------------------------------------------

template <class T>
inline void HashCombineData(ImU32& hash, T data)
{
    extern IMGUI_API ImGuiID ImHashData(const void* data, size_t data_size, ImGuiID seed);
    hash = ImHashData(&data, sizeof(data), hash);
}

void ShellExec(const std::string& path);

int InputTextCharExprFilter(ImGuiInputTextCallbackData* data);

std::string CodeShortcut(std::string_view sh);
std::string ParseShortcut(std::string_view line);

bool IsAscii(std::string_view str);
