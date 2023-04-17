#pragma once
#include <string>
#include <vector>
#include <functional>
#include <imgui.h>
#include <imgui_internal.h> //for NextIemData
#include <misc/cpp/imgui_stdlib.h> //for Input(std::string)

#ifdef IMRAD_WITH_GLFW_TEXTURE
#include <stb_image.h> //for LoadTextureFromFile
#include <GLFW/glfw3.h> //for LoadTextureFromFile
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#endif

namespace ImRad {

enum ModalResult {
	None = 0,
	Ok = 0x1,
	Cancel = 0x2,
	Yes = 0x4,
	No = 0x8,
	All = 0x10,
};

enum Alignment {
	AlignLeft = 0x1,
	AlignRight = 0x2,
	AlignHCenter = 0x4,
	AlignTop = 0x8,
	AlignBottom = 0x10,
	AlignVCenter = 0x20,
};

inline bool Combo(const char* label, int* curr, const std::vector<std::string>& items, int maxh = -1)
{
	//todo: BeginCombo/Selectable
	std::vector<const char*> citems(items.size());
	for (size_t i = 0; i < items.size(); ++i)
		citems[i] = items[i].c_str();
	return ImGui::Combo(label, curr, citems.data(), (int)citems.size(), maxh);
}

//this is a poor man Format and should be implemented using c++20 format or fmt library
inline std::string Format(std::string_view fmt)
{
	return std::string(fmt);
}

template <class A1, class... A>
std::string Format(std::string_view fmt, A1&& arg, A... args)
{
	//todo
	std::string s;
	for (size_t i = 0; i < fmt.size(); ++i)
	{
		if (fmt[i] == '{') {
			if (i + 1 == fmt.size())
				break;
			if (fmt[i + 1] == '{')
				s += '{';
			else {
				auto j = fmt.find('}', i + 1);
				if (j == std::string::npos)
					break;
				if constexpr (std::is_same_v<std::decay_t<A1>, std::string>)
					s += arg;
				else
					s += std::to_string(arg);
				return s + Format(fmt.substr(j + 1), args...);
			}
		}
		else
			s += fmt[i];
	}
	return s;
}

struct Texture
{
	ImTextureID id = 0;
	int w, h;
	explicit operator bool() const { return id != 0; }
};

#ifdef IMRAD_WITH_GLFW_TEXTURE
// Simple helper function to load an image into a OpenGL texture with common settings
// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
inline Texture LoadTextureFromFile(const std::string& filename)
{
	// Load from file
	Texture tex;
	unsigned char* image_data = stbi_load(filename.c_str(), &tex.w, &tex.h, NULL, 4);
	if (image_data == NULL)
		return {};

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	tex.id = (ImTextureID)(intptr_t)image_texture;
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.w, tex.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	return tex;
}
#else
Texture LoadTextureFromFile(const std::string& filename);
#endif

}