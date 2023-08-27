#pragma once

#include <cmath>
#include <imgui.h>
#include <iostream>
#ifdef WIN32
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

inline const std::string VER_STR = "ImRAD 0.6";
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
	float scale = 1;
	if (from == "fs")
		scale *= ImGui::GetFontSize();
	if (to == "fs")
		scale /= ImGui::GetFontSize();
	return scale;
}

void ShellExec(const std::string& path);

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
		auto it = stx::find_if(str, [](char c) { return isalpha(c); });
		if (it != str.end())
			str.erase(0, it - str.begin());
		return str;
	}
}
