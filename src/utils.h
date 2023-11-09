#pragma once

#include <cmath>
#include <imgui.h>
#include <iostream>
// c17 standard now
#include <filesystem>
namespace fs = std::filesystem;

inline const std::string VER_STR = "ImRAD 0.7";
inline const std::string GITHUB_URL = "https://github.com/tpecholt/imrad";


inline ImVec2 operator+ (const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x + b.x, a.y + b.y);
}

inline ImVec2 operator- (const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x - b.x, a.y - b.y);
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

inline float Norm(const ImVec2& a)
{
	return std::sqrt(a.x * a.x + a.y*a.y);
}

inline std::string operator+ (const std::string& s, std::string_view t)
{
	std::string ss = s;
	ss += t;
	return ss;
}

inline std::string operator+ (std::string_view t, const std::string& s)
{
	std::string ss = s;
	ss.insert(0, t, t.size());
	return ss;
}

inline std::ostream& operator<< (std::ostream& os, std::string_view t)
{
	os.write(t.data(), t.size());
	return os;
}

inline float ScaleFactor(std::string_view from, std::string_view to)
{
	//on non-adroid platform dp unit should equal to pixels
	float scale = 1;
	if (from == "fs")
		scale *= ImGui::GetFontSize();
	if (to == "fs")
		scale /= ImGui::GetFontSize();
	return scale;
}

void ShellExec(const std::string& path);