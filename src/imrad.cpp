#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#undef min
#undef max
#undef MessageBox
#endif
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome6.h>
//#include <misc/fonts/IconsMaterialDesign.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>
#include <fstream>
#include <string>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <nfd.h>

#include "node.h"
#include "cppgen.h"
#include "utils.h"
#include "ui_new_field.h"
#include "ui_message_box.h"
#include "ui_class_wizard.h"
#include "ui_table_columns.h"
#include "ui_about_dlg.h"
#include "ui_binding.h"
#include "ui_combo_dlg.h"

//must come last
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static void glfw_error_callback(int error, const char* description)
{
	std::cerr << "Glfw Error: " << description;
}

const float TB_SIZE = 40;
const float TAB_SIZE = 25; //todo examine
const std::string UNTITLED = "Untitled";

struct File
{
	std::string fname;
	CppGen codeGen;
	std::unique_ptr<UINode> rootNode;
	bool modified = false;
	fs::file_time_type time[2];
};

bool firstTime;
UIContext ctx;
std::unique_ptr<Widget> newNode;
std::vector<File> fileTabs;
int activeTab = -1;
ImGuiID dock_id_top, dock_id_right, dock_id_right2;
bool pgFocused;
int styleIdx = 0;
GLFWwindow* window = nullptr;

struct TB_Button 
{
	std::string label;
	std::string name;
};
std::vector<std::pair<std::string, std::vector<TB_Button>>> tbButtons{
	{ "Standard", {
		{ ICON_FA_ARROW_POINTER, "" },
		{ ICON_FA_FONT, "Text" },
		{ ICON_FA_AUDIO_DESCRIPTION, "Selectable" },
		{ ICON_FA_CIRCLE_PLAY, "Button" }, //ICON_FA_SQUARE_PLUS
		{ "[ab]", "Input" }, //ICON_FA_CLOSED_CAPTIONING
		{ ICON_FA_SQUARE_CHECK, "CheckBox" },
		{ ICON_FA_CIRCLE_DOT, "RadioButton" },
		{ ICON_FA_SQUARE_CARET_DOWN, "Combo" },
		{ ICON_FA_SLIDERS, "Slider" },
		{ ICON_FA_IMAGE, "Image" },
	}},
	{ "Containers", {
		{ ICON_FA_SQUARE_FULL, "Child" },
		{ ICON_FA_TABLE_CELLS_LARGE, "Table" },
		{ ICON_FA_ARROW_DOWN_WIDE_SHORT, "CollapsingHeader" },
	}}
	/*{ ICON_MD_NORTH_WEST, "" },
	{ ICON_MD_CHECK_BOX_OUTLINE_BLANK, "child" },
	{ ICON_MD_TEXT_FORMAT, "text" },
	{ ICON_MD_VIEW_COMPACT_ALT, "button" },
	{ ICON_MD_INPUT, "input" },
	{ ICON_MD_CHECK_BOX, "checkbox" },
	{ ICON_MD_RADIO_BUTTON_CHECKED, "radio" },*/
};
std::string activeButton = "";

void ActivateTab(int i)
{
	if (i >= fileTabs.size()) {
		activeTab = -1;
		ctx.codeGen = nullptr;
		return;
	}
	activeTab = i;
	auto& tab = fileTabs[i];
	ctx.selected = { tab.rootNode.get() };
	ctx.codeGen = &tab.codeGen;
	ctx.fname = tab.fname;
}

void NewFile()
{
	fileTabs.push_back({});
	fileTabs.back().rootNode = std::make_unique<TopWindow>(ctx);
	ActivateTab((int)fileTabs.size() - 1);
}

void DoOpenFile(const std::string& path)
{
	File file;
	file.fname = path;
	file.time[0] = fs::last_write_time(file.fname);
	std::error_code err;
	file.time[1] = fs::last_write_time(file.codeGen.AltFName(file.fname), err);
	file.rootNode = file.codeGen.Import(file.fname, messageBox.error);
	if (!file.rootNode) {
		messageBox.title = "CodeGen";
		messageBox.message = "Unsuccessful import because of errors";
		messageBox.buttons = MessageBox::OK;
		messageBox.OpenPopup();
		return;
	}

	auto it = stx::find_if(fileTabs, [&](const File& f) { return f.fname == file.fname; });
	if (it == fileTabs.end()) {
		fileTabs.push_back({});
		it = fileTabs.begin() + fileTabs.size() - 1;
	}
	int idx = int(it - fileTabs.begin());
	fileTabs[idx] = std::move(file);
	ActivateTab(idx);

	if (messageBox.error != "") {
		messageBox.title = "CodeGen";
		messageBox.message = "Import finished with errors";
		messageBox.buttons = MessageBox::OK;
		messageBox.OpenPopup();
	}
}

void OpenFile()
{
	nfdchar_t *outPath = NULL;
	nfdresult_t result = NFD_OpenDialog("h", NULL, &outPath);
	if (result != NFD_OKAY)
		return;
	
	ctx.snapMode = false;
	ctx.selected.clear();
	auto it = stx::find_if(fileTabs, [&](const File& f) { return f.fname == outPath; });
	if (it != fileTabs.end()) 
	{
		activeTab = int(it - fileTabs.begin());
		if (it->modified) {
			messageBox.message = "Reload and lose unsaved changes?";
			messageBox.error = "";
			messageBox.buttons = MessageBox::Yes | MessageBox::No;
			messageBox.OpenPopup([=](ImRad::ModalResult mr) {
				if (mr == MessageBox::Yes)
					DoOpenFile(outPath);
				});
		}
		else {
			DoOpenFile(outPath);
		}
	}
	else {
		DoOpenFile(outPath);
	}
}

void ReloadFiles()
{
	for (auto& tab : fileTabs)
	{
		if (tab.fname == "")
			continue;
		auto time1 = fs::last_write_time(tab.fname);
		std::error_code err;
		auto time2 = fs::last_write_time(tab.codeGen.AltFName(tab.fname), err);
		if (time1 == tab.time[0] && time2 == tab.time[1])
			continue;
		tab.time[0] = time1;
		tab.time[1] = time2;
		auto fn = fs::path(tab.fname).filename().string();
		messageBox.message = "File content of '" + fn + "' has changed. Reload?";
		messageBox.error = "";
		messageBox.buttons = MessageBox::Yes | MessageBox::No;
		messageBox.OpenPopup([&](ImRad::ModalResult mr) {
			if (mr != MessageBox::Yes)
				return;
			tab.rootNode = tab.codeGen.Import(tab.fname, messageBox.error);
			tab.modified = false;
			if (&tab == &fileTabs[activeTab])
				ctx.snapMode = false;
				ctx.selected.clear();
			});
	}
}

void DoCloseFile()
{
	ctx.selected.clear();
	fileTabs.erase(fileTabs.begin() + activeTab);
	ActivateTab(activeTab);
}

void SaveFile(bool thenClose = false);

void CloseFile()
{
	if (activeTab < 0)
		return;
	if (fileTabs[activeTab].modified) {
		messageBox.title = "Confirmation";
		std::string fname = fs::path(fileTabs[activeTab].fname).filename().string();
		if (fname.empty())
			fname = UNTITLED;
		messageBox.message = "Save changes to " + fname + "?";
		messageBox.buttons = MessageBox::Yes | MessageBox::No | MessageBox::Cancel;
		messageBox.OpenPopup([=](ImRad::ModalResult mr) {
			if (mr == ImRad::Yes)
				SaveFile(true);
			else if (mr == ImRad::No)
				DoCloseFile();
			else
				glfwSetWindowShouldClose(window, false);
			});
	}
	else {
		DoCloseFile();
	}
}

void SaveFile(bool thenClose)
{
	auto& tab = fileTabs[activeTab];
	bool trunc = false;
	if (tab.fname == "") {
		nfdchar_t *outPath = NULL;
		nfdresult_t result = NFD_SaveDialog("h", NULL, &outPath);
		if (result != NFD_OKAY) {
			if (thenClose)
				DoCloseFile();
			return;
		}
		tab.fname = outPath;
		if (!fs::path(tab.fname).has_extension())
			tab.fname += ".h";
		ctx.fname = tab.fname;
		tab.codeGen.SetNamesFromId(
			fs::path(tab.fname).stem().string());
		trunc = true;
	}
	
	if (!tab.codeGen.Export(tab.fname, trunc, tab.rootNode.get(), messageBox.error))
	{
		messageBox.title = "CodeGen";
		messageBox.message = "Unsuccessful export due to errors";
		messageBox.buttons = MessageBox::OK;
		messageBox.OpenPopup([=](ImRad::ModalResult) {
			if (thenClose)
				DoCloseFile();
			});
		return;
	}
	
	tab.modified = false;
	tab.time[0] = fs::last_write_time(tab.fname);
	std::error_code err;
	tab.time[1] = fs::last_write_time(tab.codeGen.AltFName(tab.fname), err);
	if (messageBox.error != "")
	{
		messageBox.title = "CodeGen";
		messageBox.message = "Export finished with errors";
		messageBox.buttons = MessageBox::OK;
		messageBox.OpenPopup([=](ImRad::ModalResult) {
			if (thenClose)
				DoCloseFile();
			});
		return;
	}

	if (thenClose)
		DoCloseFile();
}

void ShowCode()
{
	if (activeTab < 0)
		return;
	std::string path = (fs::temp_directory_path() / "imrad-preview.cpp").string();
	std::ofstream fout(path);
	ctx.ind = "";
	auto* root = fileTabs[activeTab].rootNode.get();
	root->Export(fout, ctx);
	
	if (ctx.errors.size()) {
		fout << "\n// Export finished with errors\n";
		for (const std::string& e : ctx.errors)
			fout << "// " << e <<  "\n";
	}
	fout.close();
#ifdef WIN32
	ShellExecute(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
	system(("xdg-open " + path).c_str());
#endif
}

void NewWidget(const std::string& name)
{
	activeButton = name;
	if (name == "") {
		ctx.selected.clear();
		ctx.snapMode = false;
	}
	else {
		newNode = Widget::Create(name, ctx);
		ctx.selected.clear();
		ctx.snapMode = true;
	}
}

void StyleColors() 
{
	ImGui::StyleColorsLight();
	ImGuiStyle& style = ImGui::GetStyle();

	// light style from Pac�me Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
	style.Alpha = 1.0f;
	style.FrameRounding = 3.0f;
	style.WindowRounding = 3.f;
	style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	//style.Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	//style.Colors[ImGuiCol_Header] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	//style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	float alpha_ = 0.95f;
	if (false) //dark
	{
		for (int i = 0; i <= ImGuiCol_COUNT; i++)
		{
			ImVec4& col = style.Colors[i];
			float H, S, V;
			ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

			if (S < 0.1f)
			{
				V = 1.0f - V;
			}
			ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
			if (col.w < 1.00f)
			{
				col.w *= alpha_;
			}
		}
	}
	else
	{
		for (int i = 0; i <= ImGuiCol_COUNT; i++)
		{
			ImVec4& col = style.Colors[i];
			if (col.w < 1.00f)
			{
				col.x *= alpha_;
				col.y *= alpha_;
				col.z *= alpha_;
				col.w *= alpha_;
			}
		}
	}
}

void DockspaceUI()
{
	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = /*ImGuiWindowFlags_MenuBar |*/ ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos + ImVec2(0, TB_SIZE + 0));
	ImGui::SetNextWindowSize(viewport->Size - ImVec2(0, TB_SIZE + 0));
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	window_flags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", nullptr, window_flags);
	ImGui::PopStyleVar();
	ImGui::PopStyleVar(2);

	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode);
	if (firstTime)
	{
		ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
		ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 350.f / viewport->Size.x, nullptr, &dockspace_id);
		ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 300.f / (viewport->Size.x - 350), nullptr, &dockspace_id);
		ImGuiID dock_id_right1, dock_id_right2;
		ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Up, 230.f / (viewport->Size.y - TB_SIZE), &dock_id_right1, &dock_id_right2);
		float vh = viewport->Size.y - TB_SIZE;
		dock_id_top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, TAB_SIZE / vh, nullptr, &dockspace_id);
		
		ImGui::DockBuilderDockWindow("FileTabs", dock_id_top);
		ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left);
		ImGui::DockBuilderDockWindow("Widgets", dock_id_right1);
		ImGui::DockBuilderDockWindow("Properties", dock_id_right2);
		ImGui::DockBuilderDockWindow("Events", dock_id_right2);
		ImGui::DockBuilderFinish(dockspace_id);
	}

	ImGui::End();
}

void ToolbarUI()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + 0));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, TB_SIZE));
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags = 0
		| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoScrollbar
		;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::Begin("TOOLBAR", nullptr, window_flags);
	ImGui::PopStyleVar();

	const float BTN_SIZE = 30;
	const auto& io = ImGui::GetIO();
	if (ImGui::Button(ICON_FA_FILE) ||
		(ImGui::IsKeyPressed(ImGuiKey_N, false) && io.KeyCtrl)) 
		NewFile();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("New File (Ctrl+N)");
	
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_FOLDER_OPEN) ||
		(ImGui::IsKeyPressed(ImGuiKey_O, false) && io.KeyCtrl))
		OpenFile();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Open File (Ctrl+O)");
	
	ImGui::BeginDisabled(activeTab < 0);
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_FLOPPY_DISK) ||
		(ImGui::IsKeyPressed(ImGuiKey_S, false) && io.KeyCtrl))
		SaveFile();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Save File (Ctrl+S)");
	
	/*ImGui::SameLine();
	if (ImGui::Button(ICON_FA_WINDOW_RESTORE))
		SaveAll();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Save All (Ctrl+Shift+S)");*/
	
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();
	ImGui::Text("Theme:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::Combo("##style", &styleIdx, "Dark\0Light\0Classic\0\0");
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	
	ImGui::SameLine();
	ImGui::BeginDisabled(activeTab < 0);
	if (ImGui::Button(ICON_FA_RECTANGLE_LIST) ||
		(ImGui::IsKeyPressed(ImGuiKey_P, false) && io.KeyCtrl)) 
		ShowCode();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Preview Code (Ctrl+P)");
	ImGui::EndDisabled();
	
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_CUBES))
	{
		classWizard.codeGen = ctx.codeGen;
		classWizard.root = fileTabs[activeTab].rootNode.get();
		classWizard.OpenPopup();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Class Wizard");
	ImGui::EndDisabled();
	
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_CIRCLE_INFO) ||
		(ImGui::IsKeyPressed(ImGuiKey_F1, false)))
	{
		aboutDlg.OpenPopup();
	}

	ImGui::End();

	float sp = ImGui::GetStyle().ItemSpacing.x;
	ImGui::SetNextWindowPos({ viewport->Size.x - 520, 100 }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 140, 260 }, ImGuiCond_FirstUseEver);
	ImGui::Begin("Widgets", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
	int n = (int)std::max(1.f, ImGui::GetContentRegionAvail().x / (BTN_SIZE + sp));
	ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0.5f, 0.5f });
	for (const auto& cat : tbButtons)
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
		if (ImGui::CollapsingHeader(cat.first.c_str()))
		{
			int rows = (stx::ssize(cat.second) + n - 1) / n;
			float h = rows * BTN_SIZE + (rows - 1) * 5;
			ImGui::BeginChild(("cat" + cat.first).c_str(), { 0, h });
			ImGui::Columns(n, nullptr, false);
			for (const auto& tb : cat.second)
			{
				if (ImGui::Selectable(tb.label.c_str(), activeButton == tb.name, 0, ImVec2(BTN_SIZE, BTN_SIZE)))
					NewWidget(tb.name);
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && tb.name != "")
					ImGui::SetTooltip(tb.name.c_str());

				ImGui::NextColumn();
			}
			ImGui::EndChild();
		}
	}
	ImGui::PopStyleVar();
	ImGui::End();
}

void TabsUI()
{
	/*ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + TB_SIZE));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, TAB_SIZE));
	ImGui::SetNextWindowViewport(viewport->ID);
	*/
	ImGuiWindowFlags window_flags = 0
		//| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoScrollbar
		//| ImGuiWindowFlags_NoSavedSettings
		;
	/*bool tmp;
	ImGui::Begin("Moje.cpp", &tmp, window_flags);
	ImGui::End();
	ImGui::DockBuilderDockWindow("Tvoje.cpp", dock_id_top);
	ImGui::Begin("Tvoje.cpp", &tmp, window_flags);
	ImGui::End();
	*/
	bool notClosed = true;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 0.0f));
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;
	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("FileTabs", 0, window_flags);
	if (ImGui::BeginTabBar(".Tabs", ImGuiTabBarFlags_NoTabListScrollingButtons))
	{
		int untitled = 0;
		for (int i = 0; i < (int)fileTabs.size(); ++i)
		{
			const auto& tab = fileTabs[i];
			std::string fname = fs::path(tab.fname).filename().string();
			if (fname == "")
				fname = UNTITLED + std::to_string(++untitled);
			if (tab.modified)
				fname += " *";
			if (ImGui::BeginTabItem(fname.c_str(), &notClosed, i == activeTab ? ImGuiTabItemFlags_SetSelected : 0))
			{
				ImGui::EndTabItem();
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && tab.fname != "")
				ImGui::SetTooltip(tab.fname.c_str());

			if (!i)
			{
				auto min = ImGui::GetItemRectMin();
				auto max = ImGui::GetItemRectMax();
				ctx.wpos.x = min.x + 20;
				ctx.wpos.y = max.y + 20;
				//const auto* viewport = ImGui::GetMainViewport();
				//ctx.wpos = viewport->GetCenter() + ImVec2(min.x, max.y) / 2;
			}
			if (!notClosed)
			{
				ActivateTab(i);
				CloseFile();
			}
			else if (ImGui::IsItemActivated() && i != activeTab)
			{
				ActivateTab(i);
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void HierarchyUI()
{
	ImGui::Begin("Hierarchy");
	if (activeTab >= 0 && fileTabs[activeTab].rootNode) 
		fileTabs[activeTab].rootNode->TreeUI(ctx);
	ImGui::End();
}

void PropertyRowsUI(bool pr)
{
	ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable(pr ? "pg" : "pge", 2, flags))
	{
		ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

		//determine common properties for a selection set
		std::vector<std::string_view> pnames;
		std::vector<std::string> pvals;
		for (auto* node : ctx.selected)
		{
			std::vector<std::string_view> pn;
			auto props = pr ? node->Properties() : node->Events();
			for (auto& p : props)
				pn.push_back(p.name);
			stx::sort(pn);
			if (node == ctx.selected[0])
				pnames = std::move(pn);
			else {
				std::vector<std::string_view> pres;
				stx::set_intersection(pnames, pn, std::back_inserter(pres));
				pnames = std::move(pres);
			}
		}
		//edit first widget
		auto props = pr ? ctx.selected[0]->Properties() : ctx.selected[0]->Events();
		std::string_view pname;
		std::string pval;
		ImGui::Indent(); //to align TreeNodes in the table
		for (int i = 0; i < (int)props.size(); ++i)
		{
			if (!stx::count(pnames, props[i].name))
				continue;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			//ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
			bool change = pr ? ctx.selected[0]->PropertyUI(i, ctx) : ctx.selected[0]->EventUI(i, ctx);
			if (change) {
				fileTabs[activeTab].modified = true;
				if (props[i].property) {
					pname = props[i].name;
					pval = props[i].property->to_arg();
				}
			}
			//ImGui::PopItemFlag();
		}
		ImGui::Unindent();
		ImGui::EndTable();

		//copy changes to other widgets
		for (size_t i = 1; i < ctx.selected.size(); ++i)
		{
			auto props = pr ? ctx.selected[i]->Properties() : ctx.selected[i]->Events();
			for (auto& p : props)
			{
				if (p.name == pname) {
					auto prop = const_cast<property_base*>(p.property);
					prop->set_from_arg(pval);
				}
			}
		}
	}
}

void PropertyUI()
{
	pgFocused = false;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
	ImGui::Begin("Events");
	if (!ctx.selected.empty())
	{
		PropertyRowsUI(0);
	}
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
		pgFocused = true;
	ImGui::End();

	ImGui::Begin("Properties");
	if (!ctx.selected.empty())
	{
		PropertyRowsUI(1);
	}
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
		pgFocused = true;
	ImGui::End();
	ImGui::PopStyleVar();
}

void PopupUI()
{
	newFieldPopup.Draw();

	tableColumns.Draw();

	comboDlg.Draw();

	messageBox.Draw();

	classWizard.Draw();

	aboutDlg.Draw();

	bindingDlg.Draw();
}

void Draw()
{
	if (activeTab >= 0 && fileTabs[activeTab].rootNode)
	{
		auto tmpStyle = ImGui::GetStyle();
		switch (styleIdx) {
			case 0: ImGui::StyleColorsDark(); break;
			case 1: ImGui::StyleColorsLight(); break;
			case 2: ImGui::StyleColorsClassic(); break;
		}
		ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];

		fileTabs[activeTab].rootNode->Draw(ctx);
	
		ImGui::GetStyle() = tmpStyle;
	}
}

std::pair<UINode*, size_t>
FindParentIndex(UINode* node, const UINode* n)
{
	for (size_t i = 0; i < node->children.size(); ++i)
	{
		if (node->children[i].get() == n)
			return { node, i };
		auto pi = FindParentIndex(node->children[i].get(), n);
		if (pi.first)
			return pi;
	}
	return { nullptr, 0 };
}

void Work()
{
	if (ImGui::GetTopMostAndVisiblePopupModal())
		return;

	if (ctx.snapMode)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ctx.snapMode = false;
			activeButton = "";
		}
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && 
			ctx.snapParent)
		{
			ctx.selected = { newNode.get() };
			newNode->sameLine = ctx.snapSameLine[0];
			if (newNode->sameLine)
				newNode->spacing = 1;
			newNode->nextColumn = ctx.snapNextColumn[0];
			newNode->beginGroup = ctx.snapBeginGroup[0];
			if (ctx.snapIndex < ctx.snapParent->children.size())
			{
				auto& next = ctx.snapParent->children[ctx.snapIndex];
				next->sameLine = ctx.snapSameLine[1];
				next->nextColumn = ctx.snapNextColumn[1];
				next->beginGroup = ctx.snapBeginGroup[1];
			}
			ctx.snapParent->children.insert(ctx.snapParent->children.begin() + ctx.snapIndex, std::move(newNode));
			
			ctx.snapMode = false;
			activeButton = "";
			fileTabs[activeTab].modified = true;
		}
	}
	else if (!ctx.selected.empty())
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !pgFocused)
		{
			for (UINode* node : ctx.selected)
			{
				if (node == fileTabs[activeTab].rootNode.get())
					continue;
				auto pi = FindParentIndex(fileTabs[activeTab].rootNode.get(), node);
				if (!pi.first)
					continue;
				Widget* wdg = dynamic_cast<Widget*>(node);
				bool sameLine = wdg->sameLine;
				bool nextColumn = wdg->nextColumn;
				pi.first->children.erase(pi.first->children.begin() + pi.second);
				if (pi.second < pi.first->children.size())
				{
					wdg = dynamic_cast<Widget*>(pi.first->children[pi.second].get());
					if (nextColumn)
						wdg->nextColumn = true;
					if (!sameLine)
						wdg->sameLine = false;
				}
			}
			ctx.selected.clear();
			fileTabs[activeTab].modified = true;
		}
	}
}

#ifdef WIN32
int WINAPI wWinMain(
	HINSTANCE   hInstance,
	HINSTANCE   hPrevInstance,
	PWSTR       lpCmdLine,
	int         nCmdShow
)
#else
int main(int argc, const char* argv[])
#endif
{
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	window = glfwCreateWindow(1280, 720, VER_STR.c_str(), NULL, NULL);
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync
	glfwMaximizeWindow(window);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	StyleColors();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 20.0f);
	ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
	ImFontConfig icons_config; 
	icons_config.MergeMode = true; 
	icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAR, 18.0f, &icons_config, icons_ranges);
	io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 18.0f, &icons_config, icons_ranges);
	//icons_ranges[0] = ICON_MIN_FA; icons_ranges[1] = ICON_MAX_16_FA;
	//io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 20.0f, &icons_config, icons_ranges);
	ImFont* fontBig = io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 32.0f);

	//io.Fonts->AddFontFromFileTTF("misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	NewFile();

	firstTime = true;
	bool lastVisible = true;
	while (true)
	{
		if (glfwWindowShouldClose(window)) 
		{
#ifdef NDEBUG
			if (fileTabs.empty())
				break;
			if (!ImGui::IsPopupOpen(-1, ImGuiPopupFlags_AnyPopupLevel))
				CloseFile();
#else
			break;
#endif
		}

		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		bool visible = glfwGetWindowAttrib(window, GLFW_FOCUSED);
		if (visible && !lastVisible)
			ReloadFiles();
		lastVisible = visible;

		DockspaceUI();
		ToolbarUI();
		TabsUI();
		HierarchyUI();
		PropertyUI();
		PopupUI();
		Draw();
		Work();
		
		//ImGui::ShowDemoWindow();

		firstTime = false;
		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
