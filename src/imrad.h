#pragma once
#include <string>
#include <vector>
#include <functional> //for ModalPopup callback
#include <fstream> //Save/LoadStyle
#include <iomanip> //std::quoted
#include <sstream> 
#include <map>
#include <imgui.h>
#include <imgui_internal.h> //CurrentItemFlags, GetCurrentWindow, PushOverrideID
#include <misc/cpp/imgui_stdlib.h> //for Input(std::string)

#ifdef IMRAD_WITH_FMT
#include <fmt/format.h>
#endif

#ifdef IMRAD_WITH_GLFW
#include <GLFW/glfw3.h> //enables kind=MainWindow
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifdef IMRAD_WITH_STB
#include <stb_image.h> //for LoadTextureFromFile
#endif
#endif

namespace ImRad {

using Int2 = int[2];
using Int3 = int[3];
using Int4 = int[4];
using Float2 = float[2];
using Float3 = float[3];
using Float4 = float[4];
using Color3 = ImVec4;
using Color4 = ImVec4;

enum ModalResult {
	None = 0,
	Ok = 0x1,
	Cancel = 0x2,
	Yes = 0x4,
	No = 0x8,
	Abort = 0x10,
	Retry = 0x20,
	Ignore = 0x40,
	All = 0x80,
};

enum Alignment {
	AlignLeft = 0x1,
	AlignRight = 0x2,
	AlignHCenter = 0x4,
	AlignTop = 0x8,
	AlignBottom = 0x10,
	AlignVCenter = 0x20,
};

enum ImeType {
    ImeText = 1, 
	ImeNumber = 2, 
	ImeDecimal = 3, 
	ImePhone = 4,
	ImeEmail = 5,

	ImeActionDone = 0x100,
	ImeActionGo = 0x200,
	ImeActionNext = 0x300,
	ImeActionPrevious = 0x400,
	ImeActionSearch = 0x500,
	ImeActionSend = 0x600,
};

struct Texture
{
	ImTextureID id = 0;
	int w, h;
	explicit operator bool() const { return id != 0; }
};

struct CustomWidgetArgs
{
	ImVec2 size;

	CustomWidgetArgs(const ImVec2& sz) : size(sz) {}
	CustomWidgetArgs(float x, float y) : size(x, y) {}
};

struct IOUserData
{
	//to UI
	float dpiScale = 1;
	ImVec2 displayOffsetMin;
	ImVec2 displayOffsetMax;
	float dimBgRatio = 1;
	bool kbdShown = false;
	std::string activeActivity;
	//from UI 
	int imeType = ImeText;

	ImRect WorkRect() const 
	{
		return {
			displayOffsetMin.x,
			displayOffsetMin.y,
			ImGui::GetMainViewport()->Size.x - displayOffsetMax.x,
			ImGui::GetMainViewport()->Size.y - displayOffsetMax.y };
	}
};

struct Animator 
{
	//todo: configure
	static inline const float DurOpenPopup = 0.4f; 
	static inline const float DurClosePopup = 0.3f;

	void StartAlways(float *v, float s, float e, float dur) 
	{
		Var nvar{ 0, v, s, e, dur, false };
		for (auto& var : vars)
			if (var.var == v) {
				var = nvar;
				return;
			}
		vars.push_back(nvar);
	}
	void StartOnce(float *v, float s, float e, float dur) 
	{
		Var nvar{ 0, v, s, e, dur, true };
		for (auto& var : vars)
			if (var.var == v) {
				var = nvar;
				return;
			}
		vars.push_back(nvar);
	}
	bool IsDone() const 
	{
		for (const auto& var : vars)
			if (var.oneShot || std::abs(*var.var - var.end) / std::abs(var.end - var.start) > 0.01)
				return false;
		return true;
	}
	//to be called from withing Begin
	void Tick() 
	{
		wsize = ImGui::GetCurrentWindow()->Size; //cache actual windows size
		size_t j = 0;
		for (size_t i = 0; i < vars.size(); ++i) {
			auto& var = vars[i];
			var.time += ImGui::GetIO().DeltaTime;
			float distance = var.end - var.start;
			float x = var.time / var.duration;
			if (x > 1)
				x = 1.f;
			float y = 1 - (1 - x) * (1 - x); //easeOutQuad
			*var.var = var.start + y * distance;
			if (!var.oneShot || std::abs(x - 1.0) >= 0.01) { //keep current var
				if (j < i)
					vars[j] = var;
				++j;
			}
		}
		vars.resize(j);
	}
	ImVec2 GetWindowSize() const 
	{
		return wsize;
	}

	struct Var 
	{
		float time;
		float* var = nullptr;
		float start, end;
		float duration;
		bool oneShot;
	};
	std::vector<Var> vars;
	ImVec2 wsize{ 0, 0 };
};

//------------------------------------------------------------------------

#ifdef ANDROID
extern int GetAssetData(const char* filename, void** outData);
#endif

inline bool Combo(const char* label, int* curr, const std::vector<std::string>& items, int maxh = -1)
{
	//todo: BeginCombo/Selectable
	std::vector<const char*> citems(items.size());
	for (size_t i = 0; i < items.size(); ++i)
		citems[i] = items[i].c_str();
	return ImGui::Combo(label, curr, citems.data(), (int)citems.size(), maxh);
}

inline bool Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
	//ImGui Selectable doesn't support negative dimensions like other controls
	ImVec2 sz = ImGui::CalcItemSize(size, 0, 0);
	return ImGui::Selectable(label, selected, flags, sz);
}

inline bool Splitter(bool split_horiz, float thickness, float* position, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
	using namespace ImGui;
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImGuiID id = window->GetID("##Splitter");
	ImRect bb;
	bb.Min = window->DC.CursorPos;
	bb.Min[!split_horiz] += *position;
	
	bb.Max = bb.Min;
	ImVec2 sz(thickness, splitter_long_axis_size);
	if (!split_horiz)
		std::swap(sz[0], sz[1]);
	sz = CalcItemSize(sz, 0.0f, 0.0f);
	bb.Max[0] += sz[0];
	bb.Max[1] += sz[1];
	
	float tmp = ImGui::GetContentRegionAvail().x - *position - thickness;
	return SplitterBehavior(bb, id, split_horiz ? ImGuiAxis_X : ImGuiAxis_Y, position, &tmp, min_size1, min_size2, 0.0f);
}

inline bool IsItemDoubleClicked()
{
	return ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered();
}

inline bool IsItemContextMenuClicked()
{
	return ImGui::IsMouseReleased(ImGuiPopupFlags_MouseButtonDefault_) && 
		ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
}

inline bool IsItemImeAction()
{
	return ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_AppForward);
}

inline bool IsItemDisabled()
{
	return ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled;
}

//allows to define popups in the window and open it from widgets calling internally
//Push/PopID like TabControl
inline void OpenWindowPopup(const char* str_id, ImGuiPopupFlags flags = 0)
{
	//RootWindow skips child window parents 
	ImGui::PushOverrideID(ImGui::GetCurrentWindow()->RootWindow->ID);
	//todo: for drop down menu use button's BL corner
	ImGui::OpenPopup(str_id, flags);
	ImGui::PopID();
}

inline void Spacing(int n)
{
	while (n--)
		ImGui::Spacing();
}

inline bool TableNextColumn(int n)
{
	bool b;
	while (n--)
		b = ImGui::TableNextColumn();
	return b;
}

inline void NextColumn(int n)
{
	while (n--)
		ImGui::NextColumn();
}

inline void PushInvisibleScrollbar()
{
	ImVec4 clr = ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarBg);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, { clr.x, clr.y, clr.z, 0 });
	clr = ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrab);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, { clr.x, clr.y, clr.z, 0 });
}

inline void PopInvisibleScrollbar()
{
	ImGui::PopStyleColor(2);
}

//optionally draws scrollbars so they can be kept hidden when no scrolling occurs
//returns:
//0 - nothing happening or scrolling continues
//1 - scrolling started
//2 - scrolling ended
inline int ScrollWhenDragging(bool drawScrollbars)
{
	static int dragState = 0;

	if (!ImGui::IsWindowFocused())
		return 0;

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		int ret = !dragState ? 1 : 0;
		dragState = 1;
		ImGuiWindow *window = ImGui::GetCurrentWindow();
		ImGui::GetCurrentContext()->NavDisableMouseHover = true;
		ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
		if (delta.x)
			ImGui::SetScrollX(window, window->Scroll.x - delta.x);
		if (delta.y)
			ImGui::SetScrollY(window, window->Scroll.y - delta.y);
		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);

		//scrollbars were made invisible, draw them again
		if (drawScrollbars)
		{
			bool tmp = window->SkipItems;
			window->SkipItems = false;
			ImGui::PushClipRect(window->Rect().Min, window->Rect().Max, false);
			ImVec4 clr = ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrab);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, { clr.x, clr.y, clr.z, 1 });
			if (window->ScrollbarX)
				ImGui::Scrollbar(ImGuiAxis_X);
			if (window->ScrollbarY)
				ImGui::Scrollbar(ImGuiAxis_Y);
			ImGui::PopStyleColor();
			ImGui::PopClipRect();
			window->SkipItems = tmp;
		}
		return ret;
	}
	else if (dragState == 1)
	{
		dragState = 0;
		ImGui::GetCurrentContext()->NavDisableMouseHover = false;
		ImGui::GetIO().MousePos = { -FLT_MAX, -FLT_MAX }; //ignore mouse release event, buttons won't get pushed
		return 2;
	}

	return 0;
}

//this currently
//* allows to move popups on the screen side further out of the screen just to give it responsive feeling
//* detects modal/popup close event
//returns
// 0 - close popup either by sliding or clicking outside
// 1 - nothing happening
// 2 - todo: maximize up/down popup
inline int MoveWhenDragging(ImGuiDir dir, ImVec2& pos, float& dimBgRatio)
{
	static int dragState = 0;
	static ImVec2 mousePos[3];
	static ImVec2 startPos, lastPos;
	static float lastDim;

	if (ImGui::IsWindowFocused())
	{
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			if (!dragState)
			{
				startPos = pos;
				mousePos[1] = mousePos[2] = ImGui::GetMousePos();
			}
			dragState = 1;
			mousePos[0] = mousePos[1];
			mousePos[1] = mousePos[2];
			mousePos[2] = ImGui::GetMousePos();
			ImGuiWindow *window = ImGui::GetCurrentWindow();
			ImGui::GetCurrentContext()->NavDisableMouseHover = true;

			ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
			//don't reset DragDelta - we need to apply full delta if pos
			//was externally modified with Animator
			//ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
			if (dir == ImGuiDir_Left) {
				pos.x = startPos.x + delta.x;
				dimBgRatio = (window->Size.x + pos.x) / window->Size.x;
			}
			else if (dir == ImGuiDir_Right) {
				pos.x = startPos.x - delta.x;
				dimBgRatio = (window->Size.x + pos.x) / window->Size.x;
			}
			else if (dir == ImGuiDir_Up) {
				pos.y = startPos.y + delta.y;
				dimBgRatio = (window->Size.y + pos.y) / window->Size.y;
			}
			else if (dir == ImGuiDir_Down) {
				pos.y = startPos.y - delta.y;
				dimBgRatio = (window->Size.y + pos.y) / window->Size.y;
			}
			if (pos.x > 0) {
				pos.x = 0;
				dimBgRatio = 1;
			}
			if (pos.y > 0) {
				pos.y = 0;
				dimBgRatio = 1;
			}
			lastPos = pos;
			lastDim = dimBgRatio;
		}
		else if (dragState == 1)
		{
			//apply lastPos because position could be rewritten by Animator but the real value
			//needs to be taken for next closing animation
			pos = lastPos;
			dimBgRatio = lastDim;
			dragState = 0;
			ImGui::GetCurrentContext()->NavDisableMouseHover = false;
			ImGui::GetIO().MousePos = { -FLT_MAX, -FLT_MAX }; //ignore mouse release event, buttons won't get pushed

			float spx = (mousePos[2].x - mousePos[0].x) / 2;
			float spy = (mousePos[2].y - mousePos[0].y) / 2;
			if (dir == ImGuiDir_Left && spx < -5)
				return 0;
			if (dir == ImGuiDir_Right && spx > 5)
				return 0;
			if (dir == ImGuiDir_Up && spy < -5)
				return 0;
			if (dir == ImGuiDir_Down && spy > 5)
				return 0;
		}
	}

	if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
	{
		ImGui::GetIO().MouseClicked[0] = false; //eat event
		return 0;
	}

	return 1;
}

//intended for android
//original version doesn't respect ioUserData.displayMinMaxOffset
inline void RenderDimmedBackground(const ImRect& rect, float alpha_mul)
{
	ImGui::GetStyle().Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0); //disable ImGui dimming
	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->PushClipRectFullScreen();
	//imgui dim default = 0.2f, 0.2f, 0.2f, 0.35f * alpha_mul
	ImU32 color = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 0.35f * alpha_mul });
	const ImRect& wr = ImGui::GetCurrentWindow()->Rect();
    dl->AddRectFilled(rect.Min, { rect.Max.x, wr.Min.y }, color);
	dl->AddRectFilled({ rect.Min.x, wr.Min.y }, { wr.Min.x, wr.Max.y }, color);
	dl->AddRectFilled({ wr.Max.x, wr.Min.y }, { rect.Max.x, wr.Max.y }, color);
	dl->AddRectFilled({ rect.Min.x, wr.Max.y }, rect.Max, color);
	dl->PopClipRect();
}

inline void RenderFilledWindowCorners(ImDrawFlags fl)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImGuiWindow* win = ImGui::GetCurrentWindow();
	ImVec2 pos = ImGui::GetWindowPos();
	ImVec2 size = ImGui::GetWindowSize();
	float r = (win->Flags & ImGuiWindowFlags_Popup) && !(win->Flags & ImGuiWindowFlags_Modal) ?
		ImGui::GetStyle().PopupRounding : ImGui::GetStyle().WindowRounding;
	ImVec2 pad = ImGui::GetStyle().WindowPadding;
	//GetWindowBgColorIdx is not accessible
	ImU32 col = ImGui::GetColorU32((win->Flags & ImGuiWindowFlags_Popup) ? ImGuiCol_PopupBg : ImGuiCol_WindowBg);

	ImGui::PushClipRect(pos, { pos.x + size.x, pos.y + size.y }, false);

	if (fl & ImDrawFlags_RoundCornersBottomLeft)
		dl->AddRectFilled({ pos.x, pos.y + size.y - r }, { pos.x + r, pos.y + size.y },	col);
	if (fl & ImDrawFlags_RoundCornersBottomRight)
		dl->AddRectFilled({ pos.x + size.x - r, pos.y + size.y - r }, { pos.x + size.x, pos.y + size.y }, col);
	if (fl & ImDrawFlags_RoundCornersTopLeft)
		dl->AddRectFilled(pos, { pos.x + r, pos.y + r }, col);
	if (fl & ImDrawFlags_RoundCornersTopRight)
		dl->AddRectFilled({ pos.x + size.x - r, pos.y }, { pos.x + size.x, pos.y + r }, col);

	ImGui::PopClipRect();
}

inline std::string Format(std::string_view fmt)
{
	return std::string(fmt);
}

template <class A1, class... A>
std::string Format(std::string_view fmt, A1&& arg, A&&... args)
{
#ifdef IMRAD_WITH_FMT
	return fmt::format(fmt, std::forward<A1>(arg), std::forward<A>(args)...);
#else
	//todo
	std::string s;
	for (size_t i = 0; i < fmt.size(); ++i)
	{
		if (fmt[i] == '{') {
			if (i + 1 == fmt.size())
				break;
			if (fmt[i + 1] == '{') {
				s += '{';
				++i;
			}
			else {
				auto j = fmt.find('}', i + 1);
				if (j == std::string::npos)
					break;
				if constexpr (std::is_same_v<std::decay_t<A1>, std::string>)
					s += arg;
				else if constexpr (std::is_same_v<std::decay_t<A1>, const char*> || 
									std::is_same_v<std::decay_t<A1>, char*>)
					s += arg;
				else if constexpr (std::is_same_v<std::decay_t<A1>, char>)
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
#endif
}

#if (defined (IMRAD_WITH_GLFW) || defined(ANDROID)) && defined(IMRAD_WITH_STB)
// Simple helper function to load an image into a OpenGL texture with common settings
// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
inline Texture LoadTextureFromFile(std::string_view filename)
{
	// Load from file
	Texture tex;
	unsigned char* image_data = nullptr;
#ifdef ANDROID
	unsigned char* buffer;
	int len = GetAssetData(filename.c_str(), &buffer);
	image_data = stbi_load_from_memory(buffer, len, &tex.w, &tex.h, NULL, 4);
#else
	std::string tmp(filename);
    image_data = stbi_load(tmp.c_str(), &tex.w, &tex.h, NULL, 4);
#endif
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
Texture LoadTextureFromFile(std::string_view filename);
#endif

//For debugging pruposes
inline void SaveStyle(std::string_view fname_, const ImGuiStyle* src = nullptr, const std::map<std::string, std::string>& extra = {})
{
	const ImGuiStyle* style = src ? src : &ImGui::GetStyle();
	std::string fname(fname_.begin(), fname_.end());
	std::ofstream fout(fname);
	if (!fout)
		throw std::runtime_error("can't write '" + fname + "'");
	
	fout << "[colors]\n";
	for (int i = 0; i < ImGuiCol_COUNT; ++i) {
		fout << ImGui::GetStyleColorName(i) << " = ";
		const auto& clr = style->Colors[i];
		fout << int(clr.x * 255) << " " << int(clr.y * 255) << " " 
			<< int(clr.z * 255) << " " << int(clr.w * 255) << "\n";
	}

	fout << "\n[variables]\n";
#define WRITE_FLT(a) fout << #a " = " << style->a << "\n"
#define WRITE_VEC(a) fout << #a " = " << style->a.x << " " << style->a.y << "\n"
	
	WRITE_FLT(Alpha);
	WRITE_FLT(DisabledAlpha);
	WRITE_VEC(WindowPadding);
	WRITE_FLT(WindowRounding);
	WRITE_FLT(WindowBorderSize);
	WRITE_VEC(WindowMinSize);
	WRITE_VEC(WindowTitleAlign);
	WRITE_FLT(ChildRounding);
	WRITE_FLT(ChildBorderSize);
	WRITE_FLT(PopupRounding);
	WRITE_FLT(PopupBorderSize);
	WRITE_VEC(FramePadding);
	WRITE_FLT(FrameRounding);
	WRITE_FLT(FrameBorderSize);
	WRITE_VEC(ItemSpacing);
	WRITE_VEC(ItemInnerSpacing);
	WRITE_VEC(CellPadding);
	WRITE_FLT(IndentSpacing);
	WRITE_FLT(ScrollbarSize);
	WRITE_FLT(ScrollbarRounding);
	WRITE_FLT(TabRounding);
	WRITE_FLT(TabBorderSize);
#undef WRITE_FLT
#undef WRITE_VEC

	fout << "\n[fonts]\n";
	fout << "Default = \"Roboto-Medium.ttf\" size 20\n";
	
	std::string lastSection;
	for (const auto& kv : extra)
	{
		size_t i = kv.first.find_last_of('.');
		if (i == std::string::npos)
			continue;
		if (kv.first.substr(0, i) != lastSection) {
			lastSection = kv.first.substr(0, i);
			fout << "\n[" << lastSection << "]\n";
		}
		fout << kv.first.substr(i + 1) << " = " << kv.second << "\n";
	}
}

//This function can be used in your code to load style and fonts from the INI file
//It is also used by ImRAD when switching themes
inline void LoadStyle(std::string_view fname, float fontScaling = 1, ImGuiStyle* dst = nullptr, std::map<std::string, ImFont*>* fontMap = nullptr, std::map<std::string, std::string>* extra = nullptr)
{
	ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
	*style = ImGuiStyle();
	auto& io = ImGui::GetIO();

	std::string parentPath(fname);
	size_t ix = parentPath.find_last_of("/\\");
	if (ix != std::string::npos)
		parentPath.resize(ix + 1);

	std::ifstream fin(std::string(fname.begin(), fname.end()));
	if (!fin)
		throw std::runtime_error("Can't read " + std::string(fname));
	std::string line;
	std::string cat;
	int lastClr = -1;
	std::string lastFont;
	while (std::getline(fin, line)) 
	{
		if (line.empty() || line[0] == ';' || line[0] == '#')
			continue;
		else if (line[0] == '[' && line.back() == ']')
			cat = line.substr(1, line.size() - 2);
		else
		{
			size_t i1 = line.find_first_not_of("=\t ", 0);
			size_t i2 = line.find_first_of("=\t ", i1);
			if (i1 == std::string::npos || i2 == std::string::npos)
				continue;
			std::string key = line.substr(i1, i2 - i1);
			i1 = line.find_first_not_of("=\t ", i2);
			std::istringstream is(line.substr(i1));
			
			if (cat == "colors")
			{
				for (int i = lastClr + 1; i != lastClr; i = (i + 1) % ImGuiCol_COUNT)
					if (key == ImGui::GetStyleColorName(i)) {
						lastClr = i;
						int r, g, b, a;
						is >> r >> g >> b >> a;
						style->Colors[i].x = r / 255.f;
						style->Colors[i].y = g / 255.f;
						style->Colors[i].z = b / 255.f;
						style->Colors[i].w = a / 255.f;
						break;
					}
			}
			else if (cat == "variables")
			{
#define READ_FLT(a) if (key == #a) is >> style->a;
#define READ_VEC(a) if (key == #a) is >> style->a.x >> style->a.y;
				
				READ_FLT(Alpha);
				READ_FLT(DisabledAlpha);
				READ_VEC(WindowPadding);
				READ_FLT(WindowRounding);
				READ_FLT(WindowBorderSize);
				READ_VEC(WindowMinSize);
				READ_VEC(WindowTitleAlign);
				READ_FLT(ChildRounding);
				READ_FLT(ChildBorderSize);
				READ_FLT(PopupRounding);
				READ_FLT(PopupBorderSize);
				READ_VEC(FramePadding);
				READ_FLT(FrameRounding);
				READ_FLT(FrameBorderSize);
				READ_VEC(ItemSpacing);
				READ_VEC(ItemInnerSpacing);
				READ_VEC(CellPadding);
				READ_FLT(IndentSpacing);
				READ_FLT(ScrollbarSize);
				READ_FLT(ScrollbarRounding);
				READ_FLT(TabRounding);
				READ_FLT(TabBorderSize);
#undef READ_FLT
#undef READ_VEC
			}
			else if (cat == "fonts")
			{
				std::string path;
				float size = 20;
				ImVec2 goffset;
				bool hasRange = false;
				static std::vector<std::unique_ptr<ImWchar[]>> rngs;

				is >> std::quoted(path);
				bool isAbsolute = path.size() >= 2 && (path[0] == '/' || path[1] == ':');
				if (!isAbsolute)
					path = parentPath + path;
				std::string tmp;
				while (is >> tmp)
				{
					if (tmp == "size")
						is >> size;
					else if (tmp == "range") {
						hasRange = true;
						//needs to outlive this function
						rngs.push_back(std::unique_ptr<ImWchar[]>(new ImWchar[3]));
						is >> rngs.back()[0] >> rngs.back()[1];
						rngs.back()[2] = 0;
					}
					else if (tmp == "goffset") {
						is >> goffset.x >> goffset.y;
					}
				}

				ImFontConfig cfg;
				strncpy(cfg.Name, key.c_str(), sizeof(cfg.Name));
				cfg.Name[sizeof(cfg.Name) - 1] = '\0';
				cfg.MergeMode = key == lastFont;
				cfg.GlyphRanges = hasRange ? rngs.back().get() : nullptr;
				cfg.GlyphOffset = { goffset.x * fontScaling, goffset.y * fontScaling };
#ifdef ANDROID
				void* font_data;
				int font_data_size = GetAssetData(path.c_str(), &font_data);
				ImFont* fnt = io.Fonts->AddFontFromMemoryTTF(font_data, font_data_size, size * fontScaling);
#else
				ImFont* fnt = io.Fonts->AddFontFromFileTTF(path.c_str(), size * fontScaling, &cfg);
#endif
				if (!fnt)
					throw std::runtime_error("Can't load " + path);
				if (!cfg.MergeMode && fontMap)
					(*fontMap)[lastFont == "" ? "" : key] = fnt;
			
				lastFont = key;
			}
			else if (extra)
			{
				(*extra)[cat + "." + key] = is.str();
			}
		}
	}
	if (fontMap && !(*fontMap).count(""))
		(*fontMap)[""] = io.Fonts->AddFontDefault();
}

//This function will be called from the generated code when alternate font is used
inline ImFont* GetFontByName(std::string_view name)
{
	if (name == "")
		return ImGui::GetDefaultFont();

	const auto& io = ImGui::GetIO();
	for (const auto& cfg : io.Fonts->ConfigData) {
		if (cfg.MergeMode)
			continue;
		if (name == cfg.Name)
			return cfg.DstFont;
	}
	return nullptr;
}

}