#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef min
#undef max
#undef MessageBox
#endif
#if defined(__APPLE__)
#include <mach-o/dyld.h>
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
#include "ui_horiz_layout.h"
#include "ui_input_name.h"
#include "ui_settings_dlg.h"

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
const std::string DEFAULT_STYLE = "Dark";
const std::string DEFAULT_UNIT = "px";
const char* INI_FILE_NAME = "imgui.ini";

struct File
{
	std::string fname;
	CppGen codeGen;
	std::unique_ptr<TopWindow> rootNode;
	bool modified = false;
	fs::file_time_type time[2];
	std::string styleName;
	std::string unit;
};

enum ProgramState { Run, Init, Shutdown };
ProgramState programState;
std::string rootPath;
std::string initErrors, showError;
std::string fontName = "Roboto-Regular.ttf";
float fontSize = 19;
UIContext ctx;
std::unique_ptr<Widget> newNode;
std::vector<File> fileTabs;
int activeTab = -1;
std::vector<std::pair<std::string, std::string>> styleNames; //name, path
bool reloadStyle = true;
GLFWwindow* window = nullptr;
int addInputCharacter = 0;
std::string activeButton = "";
std::vector<std::unique_ptr<Widget>> clipboard;
GLFWcursor* curCross = nullptr;
ImRad::IOUserData ioUserData;

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
		{ "[a_]", "Input" }, //ICON_FA_CLOSED_CAPTIONING
		{ ICON_FA_SQUARE_CHECK, "CheckBox" },
		{ ICON_FA_CIRCLE_DOT, "RadioButton" },
		{ ICON_FA_SQUARE_CARET_DOWN, "Combo" },
		{ ICON_FA_SLIDERS, "Slider" },
		{ ICON_FA_CIRCLE_HALF_STROKE, "ColorEdit" },
		{ ICON_FA_BATTERY_HALF, "ProgressBar" },
		{ ICON_FA_IMAGE, "Image" },
		{ ICON_FA_LEFT_RIGHT, "Spacer" },
		{ ICON_FA_MINUS, "Separator" },
		{ ICON_FA_EXPAND, "CustomWidget" },
	}},
	{ "Containers", {
		{ ICON_FA_SQUARE_FULL, "Child" },
		{ ICON_FA_TABLE_CELLS_LARGE, "Table" },
		{ ICON_FA_ARROW_DOWN_WIDE_SHORT, "CollapsingHeader" },
		{ ICON_FA_FOLDER_CLOSED, "TabBar" },
		{ ICON_FA_SITEMAP, "TreeNode" },
		{ ICON_FA_ARROWS_LEFT_RIGHT_TO_LINE, "Splitter" },
		//{ ICON_FA_CLAPPERBOARD /*ELLIPSIS*/, "MenuBar" },
		{ ICON_FA_MESSAGE /*RECEIPT*/, "ContextMenu" },
	}}
};

//CancelShutdown identifier is used in winuser.h
void DoCancelShutdown()
{
	if (programState != Shutdown)
		return;

	glfwSetWindowShouldClose(window, false);
	programState = Run;
	reloadStyle = true;
	ImGui::GetIO().IniFilename = INI_FILE_NAME;
}

void DoReloadFile()
{
	if (activeTab < 0)
		return;
	auto& tab = fileTabs[activeTab];
	if (tab.fname == "" || !fs::is_regular_file(tab.fname))
		return;

	std::map<std::string, std::string> params;
	tab.rootNode = tab.codeGen.Import(tab.fname, params, messageBox.error);
	auto pit = params.find("style");
	tab.styleName = pit == params.end() ? DEFAULT_STYLE : pit->second;
	pit = params.find("unit");
	tab.unit = pit == params.end() ? DEFAULT_UNIT : pit->second;
	bool styleFound = stx::count_if(styleNames, [&](const auto& st) {
		return st.first == tab.styleName;
		});
	if (!styleFound) {
		messageBox.error = "Unknown style \"" + tab.styleName + "\" used\n" + messageBox.error;
		tab.styleName = DEFAULT_STYLE;
	}
	tab.modified = false;
	ctx.mode = UIContext::NormalSelection;
	ctx.selected = { tab.rootNode.get() };

	if (messageBox.error != "" && programState != Shutdown)
	{
		messageBox.title = "Reload";
		messageBox.buttons = ImRad::Ok;
		messageBox.OpenPopup();
	}
}

void ReloadFile()
{
	if (activeTab < 0)
		return;
	auto& tab = fileTabs[activeTab];
	if (tab.fname == "" || !fs::is_regular_file(tab.fname))
		return;
	auto time1 = fs::last_write_time(tab.fname);
	std::error_code err;
	auto time2 = fs::last_write_time(tab.codeGen.AltFName(tab.fname), err);
	if (time1 == tab.time[0] && time2 == tab.time[1])
		return;
	tab.time[0] = time1;
	tab.time[1] = time2;

	if (programState != Shutdown) 
	{
		auto fn = fs::path(tab.fname).filename().string();
		messageBox.title = "Reload";
		messageBox.message = "File content of '" + fn + "' has changed. Reload?";
		messageBox.buttons = ImRad::Yes | ImRad::No;

		messageBox.OpenPopup([&](ImRad::ModalResult mr) {
			if (mr == ImRad::Yes)
				DoReloadFile();
			});
	}
	else {
		DoReloadFile();
	}
}

void ActivateTab(int i)
{
	/*doesn't work when activeTab is closed
	std::string lastStyle;
	if (activeTab >= 0)
		lastStyle = fileTabs[activeTab].styleName;*/
	if (i >= fileTabs.size())
		i = (int)fileTabs.size() - 1;
	if (i < 0) {
		activeTab = -1;
		ctx.selected.clear();
		ctx.codeGen = nullptr;
		return;
	}
	activeTab = i;
	auto& tab = fileTabs[i];
	ctx.selected = { tab.rootNode.get() };
	ctx.codeGen = &tab.codeGen;
	ReloadFile();

	if (programState != Shutdown)
		reloadStyle = true;
}

void NewFile(TopWindow::Kind k)
{
	ctx.kind = k;
	auto top = std::make_unique<TopWindow>(ctx);
	File file;
	file.rootNode = std::move(top);
	file.styleName = DEFAULT_STYLE;
	file.unit = k == TopWindow::Activity ? "dp" : DEFAULT_UNIT;
	file.modified = true;
	fileTabs.push_back(std::move(file));
	ActivateTab((int)fileTabs.size() - 1);
}

void CopyFileReplace(const std::string& from, const std::string& to, std::vector<std::pair<std::string, std::string>>& repl)
{
	std::ifstream fin(from);
	if (!fin)
		throw std::runtime_error("can't read from " + from);
	std::ofstream fout(to);
	if (!fout)
		throw std::runtime_error("can't write to " + to);
	
	std::string line;
	while (std::getline(fin, line)) 
	{
		size_t i = 0;
		while (true) 
		{
			i = line.find("${", i);
			if (i == std::string::npos)
				break;
			size_t j = line.find("}", i);
			if (j == std::string::npos)
				break;
			for (const auto& r : repl)
				if (!line.compare(i + 2, j - i - 2, r.first)) {
					line.replace(line.begin() + i, line.begin() + j + 1, r.second);
					j = i + r.second.size();
					break;
				}
			i = j;
		}
		fout << line << "\n";
	}
}

void DoNewTemplate(int type, const std::string& name)
{
	nfdchar_t *outPath = NULL;
	nfdfilteritem_t filterItem[1] = { { (const nfdchar_t *)"Source code", (const nfdchar_t *)"cpp" } };
	nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "main.cpp");
	if (result != NFD_OKAY)
		return;
	fs::path p = outPath;
	NFD_FreePath(outPath);
	if (!p.has_extension())
		p.replace_extension(".cpp");

	try {
		switch (type)
		{
		case 0:
			fs::copy_file(rootPath + "/template/glfw/main.cpp", p, fs::copy_options::overwrite_existing);
			break;
		case 1: {
			std::string jni = name;
			stx::replace(jni, '.', '_');
			if (name.find('.') == std::string::npos)
				throw std::runtime_error("invalid package name");
			std::string lib = name.substr(name.rfind('.') + 1);
			
			std::vector<std::pair<std::string, std::string>> repl{
				{ "JAVA_PACKAGE", name },
				{ "JNI_PACKAGE", jni },
				{ "LIB_NAME", lib },
				{ "IMRAD_INCLUDE", rootPath + "/include" },
			};
			CopyFileReplace(rootPath + "/template/android/main.cpp", p.string(), repl);
			CopyFileReplace(rootPath + "/template/android/CMakeLists.txt", p.replace_filename("CMakeLists.txt").string(), repl);
			CopyFileReplace(rootPath + "/template/android/MainActivity.java", p.replace_filename("MainActivity.java").string(), repl);
			CopyFileReplace(rootPath + "/template/android/AndroidManifest.xml", p.replace_filename("AndroidManifest.xml").string(), repl);
			break;
		}
		}
	}
	catch (std::exception& e) {
		messageBox.title = "error";
		messageBox.message = e.what();
		messageBox.buttons = ImRad::Ok;
		messageBox.OpenPopup();
	}
}

void NewTemplate(int type)
{
	if (type == 1)
	{
		//android template
		inputName.title = "Java package name";
		inputName.hint = "com.example.test";
		inputName.OpenPopup([type](ImRad::ModalResult mr) {
			DoNewTemplate(type, inputName.name);
			});
	}
	else
		DoNewTemplate(type, "");
}

void DoOpenFile(const std::string& path, std::string* errs = nullptr)
{
	if (!fs::is_regular_file(path)) {
		if (errs)
			*errs += "Can't read '" + path + "'\n";
		else {
			messageBox.title = "ImRAD";
			messageBox.message = "Can't read '" + path + "'";
			messageBox.buttons = ImRad::Ok;
			messageBox.OpenPopup();
		}
		return;
	}

	File file;
	file.fname = path;
	file.time[0] = fs::last_write_time(file.fname);
	std::error_code err;
	file.time[1] = fs::last_write_time(file.codeGen.AltFName(file.fname), err);
	std::map<std::string, std::string> params;
	file.rootNode = file.codeGen.Import(file.fname, params, messageBox.error);
	auto pit = params.find("style");
	file.styleName = pit == params.end() ? DEFAULT_STYLE : pit->second;
	pit = params.find("unit");
	file.unit = pit == params.end() ? DEFAULT_UNIT : pit->second;
	if (!file.rootNode) {
		if (errs)
			*errs += "Unsuccessful import of '" + path + "'\n";
		else {
			messageBox.title = "CodeGen";
			messageBox.message = "Unsuccessful import because of errors";
			messageBox.buttons = ImRad::Ok;
			messageBox.OpenPopup();
		}
		return;
	}
	
	bool styleFound = stx::count_if(styleNames, [&](const auto& st) {
		return st.first == file.styleName;
		});
	if (!styleFound) {
		if (errs)
			*errs += "Uknown style \"" + file.styleName + "\" used in '" + path + "'\n";
		else
			messageBox.error = "Unknown style \"" + file.styleName + "\" used\n" + messageBox.error; 
		file.styleName = DEFAULT_STYLE;
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
		if (errs)
			*errs += messageBox.error + "\n";
		else {
			messageBox.title = "CodeGen";
			messageBox.message = "Import finished with errors";
			messageBox.buttons = ImRad::Ok;
			messageBox.OpenPopup();
		}
	}
}

void OpenFile()
{
	nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "Headers", "h,hpp" } };
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
	if (result != NFD_OKAY)
		return;
	
	ctx.mode = UIContext::NormalSelection;
	ctx.selected.clear();
	auto it = stx::find_if(fileTabs, [&](const File& f) { return f.fname == outPath; });
	if (it != fileTabs.end()) 
	{
		ActivateTab(int(it - fileTabs.begin()));
		if (it->modified) {
			messageBox.title = "Open File";
			messageBox.message = "Reload and lose unsaved changes?";
			messageBox.buttons = ImRad::Yes | ImRad::No;
			messageBox.OpenPopup([=](ImRad::ModalResult mr) {
				if (mr == ImRad::Yes)
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
    NFD_FreePath(outPath);
}

void CloseFile();
bool SaveFile(bool thenClose);

void DoCloseFile()
{
	ctx.root = nullptr;
	ctx.selected.clear();
	fileTabs.erase(fileTabs.begin() + activeTab);
	ActivateTab(activeTab);
	if (programState == Shutdown)
		CloseFile();
}

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
		messageBox.buttons = ImRad::Yes | ImRad::No | ImRad::Cancel;
		messageBox.OpenPopup([=](ImRad::ModalResult mr) {
			if (mr == ImRad::Yes)
				SaveFile(true);
			else if (mr == ImRad::No)
				DoCloseFile();
			else
				DoCancelShutdown();
			});
	}
	else {
		DoCloseFile();
	}
}

void DoSaveFile(bool thenClose)
{
	auto& tab = fileTabs[activeTab];
	std::map<std::string, std::string> params{
		{ "style", tab.styleName },
		{ "unit", tab.unit },
	};
	if (!tab.codeGen.ExportUpdate(tab.fname, tab.rootNode.get(), params, messageBox.error))
	{
		DoCancelShutdown();
		messageBox.title = "CodeGen";
		messageBox.message = "Unsuccessful export due to errors";
		messageBox.buttons = ImRad::Ok;
		messageBox.OpenPopup();
		return;
	}

	std::error_code err;
	tab.modified = false;
	tab.time[0] = fs::last_write_time(tab.fname, err);
	tab.time[1] = fs::last_write_time(tab.codeGen.AltFName(tab.fname), err);
	if (messageBox.error != "" && programState != Shutdown)
	{
		messageBox.title = "CodeGen";
		messageBox.message = "Export finished with errors";
		messageBox.buttons = ImRad::Ok;
		messageBox.OpenPopup([=](ImRad::ModalResult) {
			if (thenClose)
				DoCloseFile();
			});
		return;
	}

	if (thenClose)
		DoCloseFile();
}

bool SaveFileAs(bool thenClose)
{
	nfdchar_t *outPath = NULL;
	nfdfilteritem_t filterItem[1] = { { (const nfdchar_t *)"Header File", (const nfdchar_t *)"h,hpp" } };
	nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, nullptr);
	if (result != NFD_OKAY) {
		DoCancelShutdown();
		return false;
	}
	auto& tab = fileTabs[activeTab];
	std::string oldName = tab.fname;
	std::string newName = fs::path(outPath).replace_extension(".h").string();
    NFD_FreePath(outPath);
	try {
		if (oldName != "") {
			//copy files first so CppGen can parse it and preserve user content as with Save
			fs::copy_file(oldName, newName, fs::copy_options::overwrite_existing);
			fs::copy_file(
				fs::path(oldName).replace_extension(".cpp"), 
				fs::path(newName).replace_extension(".cpp"),
				fs::copy_options::overwrite_existing);
		}
		else {
			fs::remove(newName);
			fs::remove(fs::path(newName).replace_extension(".cpp"));
		}
	}
	catch (std::exception& e) 
	{
		DoCancelShutdown();
		messageBox.title = "error";
		messageBox.message = e.what();
		messageBox.buttons = ImRad::Ok;
		messageBox.OpenPopup();
		return false;
	}

	if (oldName == "")
		tab.codeGen.SetNamesFromId(fs::path(newName).stem().string());
	tab.fname = newName;
	DoSaveFile(thenClose);
	return true;
}

bool SaveFile(bool thenClose)
{
	auto& tab = fileTabs[activeTab];
	if (tab.fname == "") {
		return SaveFileAs(thenClose);
	}
	else {
		DoSaveFile(thenClose);
		return true;
	}
}

//todo: dangerous because of no reloading
void SaveAll()
{
	int tmp = activeTab;
	for (activeTab = 0; activeTab < fileTabs.size(); ++activeTab)
	{
		if (!SaveFile(false))
			break;
	}
	activeTab = tmp;
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
	ShellExec(path);
}

void NewWidget(const std::string& name)
{
	if (name == "") 
	{
		activeButton = "";
		ctx.selected.clear();
		ctx.mode = UIContext::NormalSelection;
	}
	else if (name == "MenuBar") 
	{
		if (ctx.root->children.empty() || 
			!dynamic_cast<MenuBar*>(ctx.root->children[0].get())) 
		{
			dynamic_cast<TopWindow*>(ctx.root)->flags |= ImGuiWindowFlags_MenuBar;
			ctx.root->children.insert(ctx.root->children.begin(), std::make_unique<MenuBar>(ctx));
			ctx.selected = { ctx.root->children[0]->children[0].get() };
		}
		ctx.mode = UIContext::NormalSelection;
	}
	else if (name == "ContextMenu") 
	{
		activeButton = "";
		auto popup = std::make_unique<MenuIt>(ctx);
		popup->contextMenu = true;
		auto item = std::make_unique<MenuIt>(ctx);
		item->label = "Item";
		popup->children.push_back(std::move(item));
		size_t i = 0;
		for (; i < ctx.root->children.size(); ++i)
		{
			if (ctx.root->children[i]->Behavior() & Widget::SnapSides)
				break;
		}
		popup->label = "ContextMenu" + std::to_string(i + 1);
		ctx.root->children.insert(ctx.root->children.begin() + i, std::move(popup));
		ctx.mode = UIContext::NormalSelection;
		ctx.selected = { ctx.root->children[i]->children[0].get() };
	}
	else 
	{
		activeButton = name;
		ctx.selected.clear();
		ctx.mode = UIContext::Snap;
		newNode = Widget::Create(name, ctx);
	}
}

void StyleColors() 
{
	ImGui::StyleColorsLight();
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.FrameRounding = 3.0f;
	style.WindowRounding = 3.f;
	style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
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
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.86f, 0.59f, 0.59f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_TabSelectedOverline].w = 0;
	style.Colors[ImGuiCol_TabDimmedSelectedOverline].w = 0;
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	float alpha_ = 0.95f;
	if (false) //dark
	{
		for (int i = 0; i < ImGuiCol_COUNT; i++)
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
		for (int i = 0; i < ImGuiCol_COUNT; i++)
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

	//ImRad::SaveStyle(rootPath + "/style/debug.ini");
}

void GetStyles()
{
	styleNames = { 
		{ "Classic", "" }, 
		{ "Light", "" }, 
		{ "Dark", "" } 
	};
	for (fs::directory_iterator it(rootPath + "/style/"); it != fs::directory_iterator(); ++it)
	{
		if (it->path().extension() != ".ini")
			continue;
		styleNames.push_back({ it->path().stem().string(), it->path().string() });
	}
}

const std::array<ImU32, UIContext::Color::COUNT>& 
GetCtxColors(const std::string& styleName)
{
	static const std::array<ImU32, UIContext::Color::COUNT> classic{
		IM_COL32(255, 255, 0, 128),
		IM_COL32(255, 0, 0, 255),
		IM_COL32(128, 128, 255, 255),
		IM_COL32(255, 0, 255, 255),
		IM_COL32(0, 255, 255, 255),
		IM_COL32(0, 255, 0, 255),
	};
	static const std::array<ImU32, UIContext::Color::COUNT> light {
		0xb0996633,//IM_COL32(64, 64, 64, 128),
		IM_COL32(255, 0, 0, 255),
		IM_COL32(128, 128, 255, 255),
		IM_COL32(255, 0, 255, 255),
		IM_COL32(0, 128, 128, 255),
		IM_COL32(0, 255, 0, 255),
	};
	static const std::array<ImU32, UIContext::Color::COUNT> dark{
		IM_COL32(255, 255, 0, 128),
		IM_COL32(255, 0, 0, 255),
		IM_COL32(128, 128, 255, 255),
		IM_COL32(255, 0, 255, 255),
		IM_COL32(0, 255, 255, 255),
		IM_COL32(0, 255, 0, 255),
	};

	if (styleName == "Light")
		return light;
	else if (styleName == "Dark")
		return dark;
	else
		return classic;
}

void LoadStyle()
{
	float faSize = fontSize * 18.f / 20.f;
	std::string fontPath = rootPath + "/style/" + fontName;
	if (!reloadStyle)
		return;
	
	reloadStyle = false;
	auto& io = ImGui::GetIO();
	io.Fonts->Clear();

	//reload ImRAD UI first
	StyleColors();
	io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
	static ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
	ImFontConfig cfg;
	cfg.MergeMode = true;
	//icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF((rootPath + "/style/" + FONT_ICON_FILE_NAME_FAR).c_str(), faSize, &cfg, icons_ranges);
	io.Fonts->AddFontFromFileTTF((rootPath + "/style/" + FONT_ICON_FILE_NAME_FAS).c_str(), faSize, &cfg, icons_ranges);
	cfg.MergeMode = false;
	strcpy(cfg.Name, "imrad.H1");
	io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize * 1.5f, &cfg);
	strcpy(cfg.Name, "imrad.H3");
	io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize * 1.1f, &cfg);

	ctx.defaultFont = nullptr;
	ctx.fontNames.clear();
	stx::fill(ctx.colors, IM_COL32(0, 0, 0, 255));
	ctx.style = ImGuiStyle();
	
	if (activeTab >= 0)
	{
		std::string styleName = fileTabs[activeTab].styleName;
		if (styleName == "Classic")
		{
			ImGui::StyleColorsClassic(&ctx.style);
			ctx.defaultFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
			ctx.defaultFont->FallbackChar = '#';
			ctx.fontNames = { "" };
			ctx.colors = GetCtxColors(styleName);
		}
		else if (styleName == "Light")
		{
			ImGui::StyleColorsLight(&ctx.style);
			ctx.defaultFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
			ctx.defaultFont->FallbackChar = '#';
			ctx.fontNames = { "" };
			ctx.colors = GetCtxColors(styleName);
		}
		else if (styleName == "Dark")
		{
			ImGui::StyleColorsDark(&ctx.style);
			ctx.defaultFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
			ctx.defaultFont->FallbackChar = '#';
			ctx.fontNames = { "" };
			ctx.colors = GetCtxColors(styleName);
		}
		else
		{
			std::map<std::string, ImFont*> fontMap;
			std::map<std::string, std::string> extra;
			try {
				auto it = stx::find_if(styleNames, [&](const auto& s) { return s.first == styleName; });
				ImRad::LoadStyle(it->second, 1.f, &ctx.style, &fontMap, &extra);
			
				ctx.defaultFont = fontMap[""];
				for (const auto& f : fontMap) {
					ctx.fontNames.push_back(f.first);
				}
				for (const auto& ex : extra) {
					if (ex.first.compare(0, 13, "imrad.colors."))
						continue;
					std::istringstream is(ex.second);
					int r, g, b, a;
					is >> r >> g >> b >> a;
					auto clr = IM_COL32(r, g, b, a);
					std::string key = ex.first.substr(13);

	#define SET_CLR(a) if (key == #a) ctx.colors[UIContext::a] = clr;
					SET_CLR(Selected);
					SET_CLR(Hovered);
					SET_CLR(Snap1);
					SET_CLR(Snap2);
					SET_CLR(Snap3);
					SET_CLR(Snap4);
					SET_CLR(Snap5);
	#undef SET_CLR
				}
			}
			catch (std::exception& e)
			{
				//can't OpenPopup here, there is no window parent
				showError = e.what();
			}
		}
	}
	ImGui_ImplOpenGL3_DestroyFontsTexture();
	ImGui_ImplOpenGL3_CreateFontsTexture();
}

void DoCloneStyle(const std::string& name)
{
	auto FormatClr = [](ImU32 c) {
		std::ostringstream os; 
		os << (c & 0xff) << " " << 
			((c >> 8) & 0xff) << " " << 
			((c >> 16) & 0xff) << " " <<
			((c >> 24) & 0xff); 
		return os.str();
	};

	std::string from = fileTabs[activeTab].styleName;
	std::string path = rootPath + "/style/" + name + ".ini";
	try 
	{
		if (from == "Classic" || from == "Dark" || from == "Light") 
		{
			ImGuiStyle style;
			if (from == "Dark")
				ImGui::StyleColorsDark(&style);
			else if (from == "Light")
				ImGui::StyleColorsLight(&style);
			else
				ImGui::StyleColorsClassic(&style);

			std::map<std::string, std::string> extra;
			const auto& colors = GetCtxColors(from);
#define SET_CLR(a) extra[std::string("imrad.colors.") + #a] = FormatClr(colors[UIContext::Color::a]);
			SET_CLR(Selected);
			SET_CLR(Hovered);
			SET_CLR(Snap1);
			SET_CLR(Snap2);
			SET_CLR(Snap3);
			SET_CLR(Snap4);
			SET_CLR(Snap5);
#undef SET_CLR
			ImRad::SaveStyle(path, &style, extra);
		}
		else 
		{
			fs::copy_file(rootPath + "/style/" + from + ".ini", path, fs::copy_options::overwrite_existing);
		}

		fileTabs[activeTab].styleName = name;
		if (!stx::count_if(styleNames, [&](const auto& s) { return s.first == name; }))
			styleNames.push_back({ name, path });

		messageBox.title = "Style saved";
		messageBox.message = "New style was saved as '" + path + "'";
		messageBox.buttons = ImRad::Ok;
		messageBox.OpenPopup();
	}
	catch (std::exception& e) 
	{
		messageBox.title = "error";
		messageBox.message = e.what();
		messageBox.buttons = ImRad::Ok;
		messageBox.OpenPopup();
	}
}

void CloneStyle()
{
	inputName.title = "Clone Style";
	inputName.hint = "Enter new style name";
	inputName.OpenPopup([](ImRad::ModalResult mr)
		{
			std::string path = rootPath + "/style/" + inputName.name + ".ini";
			if (fs::exists(path)) {
				messageBox.title = "Confirmation";
				messageBox.message = "Overwrite existing style?";
				messageBox.buttons = ImRad::Yes | ImRad::No;
				messageBox.OpenPopup([=](ImRad::ModalResult mr) {
					if (mr == ImRad::Yes)
						DoCloneStyle(inputName.name);
					});
			}
			else
				DoCloneStyle(inputName.name);
		});
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
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingOverCentralNode);
	if (programState == Init)
	{
		ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
		ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 350.f / viewport->Size.x, nullptr, &dockspace_id);
		ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 300.f / (viewport->Size.x - 350), nullptr, &dockspace_id);
		ImGuiID dock_id_right1, dock_id_right2;
		ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Up, 230.f / (viewport->Size.y - TB_SIZE), &dock_id_right1, &dock_id_right2);
		float vh = viewport->Size.y - TB_SIZE;
		ImGuiID dock_id_top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, TAB_SIZE / vh, nullptr, &dockspace_id);
		
		ImGui::DockBuilderDockWindow("FileTabs", dock_id_top);
		ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left);
		ImGui::DockBuilderDockWindow("Widgets", dock_id_right1);
		ImGui::DockBuilderDockWindow("Properties", dock_id_right2);
		ImGui::DockBuilderDockWindow("Events", dock_id_right2);
		ImGui::DockBuilderFinish(dockspace_id);
	}

	if (fileTabs.empty())
	{
		const std::string label = "Start by clicking the New File button ";
		const std::string shortcut = "Ctrl + N";
		ImVec2 size = ImGui::CalcTextSize((label + shortcut).c_str());
		ImGui::SetCursorPos({ (viewport->WorkSize.x - size.x) / 2, (viewport->WorkSize.y - size.y) / 2 });
		ImVec4 clr = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		clr.w = 0.5f;
		ImGui::PushStyleColor(ImGuiCol_Text, clr);
		ImGui::TextUnformatted(label.c_str());
		ImGui::PopStyleColor();
		ImGui::SameLine(0, 0);
		//clr = ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogramHovered);
		clr.w = 1.f;
		ImGui::PushStyleColor(ImGuiCol_Text, clr);
		ImGui::TextUnformatted(shortcut.c_str());
		ImGui::PopStyleColor();
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
		| ImGuiWindowFlags_NoNavInputs
		;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::Begin("TOOLBAR", nullptr, window_flags);
	ImGui::PopStyleVar();

	const float BTN_SIZE = 30;
	const auto& io = ImGui::GetIO();
	if (ImGui::Button(ICON_FA_FILE " " ICON_FA_CARET_DOWN) ||
		ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal))
		ImGui::OpenPopup("NewMenu");
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("New File (Ctrl+N)");
	ImGui::SetNextWindowPos(ImGui::GetCursorPos());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 10 });
	if (ImGui::BeginPopup("NewMenu"))
	{
		if (ImGui::MenuItem(ICON_FA_TV " Main Window", "\tGLFW"))
			NewFile(TopWindow::MainWindow);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("ImGui window integrated into OS window (GLFW)");
		if (ImGui::MenuItem(ICON_FA_WINDOW_MAXIMIZE "  Window", ""))
			NewFile(TopWindow::Window);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Floating ImGui window");
		if (ImGui::MenuItem(ICON_FA_CLONE "  Popup", ""))
			NewFile(TopWindow::Popup);
		if (ImGui::MenuItem(ICON_FA_WINDOW_RESTORE "  Modal Popup", ""))
			NewFile(TopWindow::ModalPopup);
		if (ImGui::MenuItem(ICON_FA_MOBILE_SCREEN "  Activity", "\tAndroid"))
			NewFile(TopWindow::Activity);
		
		ImGui::Separator();
		if (ImGui::MenuItem(ICON_FA_FILE_PEN "  main.cpp", "\tGLFW"))
			NewTemplate(0);
		if (ImGui::MenuItem(ICON_FA_FILE_PEN "  main+java+manifest", "\tAndroid"))
			NewTemplate(1);

		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_FOLDER_OPEN) ||
		ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal))
		OpenFile();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Open File (Ctrl+O)");
	
	ImGui::BeginDisabled(activeTab < 0);
	
	ImGui::SameLine();
	float cx = ImGui::GetCursorPosX();
	if (ImGui::Button(ICON_FA_FLOPPY_DISK) ||
		ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal))
		SaveFile(false);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Save File (Ctrl+S)");
	
	ImGui::SameLine(0, 0);
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine(0, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2, ImGui::GetStyle().FramePadding.y });
	if (ImGui::Button(ICON_FA_CARET_DOWN)) 
		ImGui::OpenPopup("SaveMenu");
	ImGui::PopStyleVar();
	ImGui::SetNextWindowPos({ cx, ImGui::GetCursorPosY() });
	if (ImGui::BeginPopup("SaveMenu"))
	{
		if (ImGui::MenuItem("Save As..."))
			SaveFileAs(false);
		if (ImGui::MenuItem("Save All", "\tCtrl+Shift+S"))
			SaveAll();
		ImGui::EndPopup();
	}
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, ImGuiInputFlags_RouteGlobal))
		SaveAll();

	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();
	ImGui::Text("Style");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	std::string styleName;
	if (activeTab >= 0)
		styleName = fileTabs[activeTab].styleName;
	if (ImGui::BeginCombo("##style", styleName.c_str()))
	{
		for (size_t i = 0; i < styleNames.size(); ++i)
		{
			if (ImGui::Selectable(styleNames[i].first.c_str(), styleNames[i].first == styleName))
			{
				reloadStyle = true;
				assert(activeTab >= 0);
				fileTabs[activeTab].modified = true;
				fileTabs[activeTab].styleName = styleNames[i].first;
			}
			if (i == 2 && i + 1 < styleNames.size())
				ImGui::Separator();
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_CLONE))
		CloneStyle();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Clone Style");
	ImGui::SameLine();
	ImGui::Text("Units");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	const std::string unit = activeTab >= 0 ? fileTabs[activeTab].unit : "";
	std::array<const char*, 2> UNITS{ "px", /*"fs",*/ "dp" };
	int usel = 0;
	if (unit != "")
		usel = (int)(stx::find(UNITS, unit) - UNITS.begin());
	if (ImGui::Combo("##units", &usel, stx::join(UNITS, std::string_view("\0", 1)).c_str())) // "px\0fs\0dp\0"))
	{
		auto& tab = fileTabs[activeTab];
		tab.unit = UNITS[usel];
		tab.modified = true;
	}
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	
	ImGui::SameLine();
	bool showHelper = activeTab >= 0 && 
		ctx.selected.size() >= 1 && 
		(ctx.selected[0]->Behavior() & UINode::SnapSides);
	ImGui::BeginDisabled(!showHelper);
	if (ImGui::Button(ICON_FA_LEFT_RIGHT))
	{
		horizLayout.root = fileTabs[activeTab].rootNode.get();
		HorizLayout::ExpandSelection(ctx.selected, horizLayout.root);
		horizLayout.selected = ctx.selected;
		horizLayout.ctx = &ctx;
		horizLayout.OpenPopup();
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Table Layout Helper");
	ImGui::SameLine();
	
	ImGui::SameLine();
	ImGui::BeginDisabled(activeTab < 0);
	if (ImGui::Button(ICON_FA_BOLT) || // ICON_FA_BOLT, ICON_FA_RIGHT_TO_BRACKET) ||
		ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_P, ImGuiInputFlags_RouteGlobal))
		ShowCode();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Preview Code (Ctrl+P)");
	ImGui::EndDisabled();
	
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_CUBES))
	{
		classWizard.codeGen = ctx.codeGen;
		classWizard.root = fileTabs[activeTab].rootNode.get();
		classWizard.modified = &fileTabs[activeTab].modified;
		classWizard.OpenPopup();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("Class Wizard");
	
	ImGui::EndDisabled();
	
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_GEAR))
	{
		std::vector<std::string> fontNames;
		for (const auto& entry : fs::directory_iterator(rootPath + "/style/")) 
		{
			if (entry.is_regular_file() && entry.path().extension() == ".ttf")
				fontNames.push_back(entry.path().stem().string());
		}
		settingsDlg.fontNames = std::move(fontNames);
		settingsDlg.fontName = fontName.substr(0, fontName.size() - 4);
		settingsDlg.fontSize = std::to_string((int)fontSize);
		settingsDlg.OpenPopup([](ImRad::ModalResult) 
		{
			fontName = settingsDlg.fontName + ".ttf";
			fontSize = std::stof(settingsDlg.fontSize);
			ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
			reloadStyle = true;
		});
	}

	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_CIRCLE_INFO) ||
		ImGui::Shortcut(ImGuiKey_F1, ImGuiInputFlags_RouteGlobal))
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
			ImGui::BeginChild(("cat" + cat.first).c_str(), { 0, h }, ImGuiChildFlags_NavFlattened, 0);
			ImGui::Columns(n, nullptr, false);
			for (const auto& tb : cat.second)
			{
				if (ImGui::Selectable(tb.label.c_str(), activeButton == tb.name, 0, ImVec2(BTN_SIZE, BTN_SIZE)))
					NewWidget(tb.name);
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && tb.name != "")
					ImGui::SetTooltip("%s", tb.name.c_str());

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
			bool notClosed = true;
			if (ImGui::BeginTabItem(fname.c_str(), &notClosed, i == activeTab ? ImGuiTabItemFlags_SetSelected : 0))
			{
				ImGui::EndTabItem();
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && tab.fname != "")
				ImGui::SetTooltip("%s", tab.fname.c_str());

			if (!i)
			{
				auto min = ImGui::GetItemRectMin();
				auto max = ImGui::GetItemRectMax();
				ctx.designAreaMin.x = min.x;
				ctx.designAreaMin.y = max.y;
				const ImGuiWindow* win = ImGui::GetCurrentWindow();
				const auto* viewport = ImGui::GetMainViewport();
				ctx.designAreaMax.x = win->Pos.x + win->Size.x;
				ctx.designAreaMax.y = viewport->Pos.y + viewport->Size.y;
			}
			if (!notClosed ||
				(i == activeTab && ImGui::IsKeyPressed(ImGuiKey_F4, false) && ImGui::GetIO().KeyCtrl))
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
	//ImGui::PushFont(ctx.defaultFont); icons are FA
	ImGui::Begin("Hierarchy");
	if (activeTab >= 0 && fileTabs[activeTab].rootNode) 
		fileTabs[activeTab].rootNode->TreeUI(ctx);
	ImGui::End();
}

bool BeginPropGroup(const std::string& label, const UINode::Prop& prop, bool& open)
{
	ImVec2 pad = ImGui::GetStyle().FramePadding;
	ImGui::TableNextRow();
	if (label == "overlayPos") //hack
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, 164, 255));
	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();
	ImGui::Unindent();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, pad.y });
	open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
	ImGui::PopStyleVar();
	bool eatProp = false;
	if (label == "overlayPos") 
	{
		eatProp = true;
		ImGui::TableNextColumn();
		bool tmp = prop.property->to_arg() == "true";
		if (ImGui::Checkbox(("##" + prop.name).c_str(), &tmp))
			prop.property->set_from_arg(tmp ? "true" : "false");
	}
	else if (!open)
	{
		ImGui::TableNextColumn();
		ImGui::TextDisabled("...");
	}
	ImGui::Indent();
	return eatProp;
}

void EndPropGroup(bool open)
{
	if (open) 
	{
		ImGui::TreePop();
		//ImGui::Unindent();
	}
}

void PropertyRowsUI(bool pr)
{
	int keyPressed = 0;
	if (addInputCharacter)
	{
		//select all text when activating
		auto* g = ImGui::GetCurrentContext();
		g->NavNextActivateFlags &= ~ImGuiActivateFlags_TryToPreserveState;
		g->NavActivateFlags &= ~ImGuiActivateFlags_TryToPreserveState;
		//and send a character to overwrite it
		ImGui::GetIO().AddInputCharacter(addInputCharacter);
		addInputCharacter = 0;
	}
	if (!ImGui::GetIO().WantTextInput && 
		!(ImGui::GetIO().KeyMods & ~ImGuiMod_Shift) &&
		!ImGui::GetTopMostPopupModal()) //TopMostAndVisible doesn't work with ClassWizard open??
	{
		for (int key = ImGuiKey_A; key <= ImGuiKey_Z; ++key)
			if (ImGui::IsKeyPressed((ImGuiKey)key)) {
				keyPressed = key - ImGuiKey_A + (ImGui::GetIO().KeyShift ? 'A' : 'a');
				break;
			}
	}

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
		//when selecting other widget of same kind from Tree, value from previous widget
		//wants to be sent to the new widget because input IDs are the same
		//PushID widget ptr to prevent it
		//We PopID/PushID before a category treeNode to keep them open/close across all widgets
		//having same property
		ImGui::PushID(ctx.selected[0]); 
		//edit first widget
		auto props = pr ? ctx.selected[0]->Properties() : ctx.selected[0]->Events();
		std::string_view pname;
		std::string pval;
		std::string inCat;
		bool catOpen = false;
		ImGui::Indent(); //to align TreeNodes in the table
		for (int i = 0; i < (int)props.size(); ++i)
		{
			const auto& prop = props[i];
			if (!stx::count(pnames, prop.name))
				continue;
			std::string cat;
			auto i1 = prop.name.find('@');
			if (i1 != std::string::npos) {
				auto i2 = prop.name.find('.', i1);
				if (i2 != std::string::npos)
					cat = prop.name.substr(i1 + 1, i2 - i1 - 1);
			}
			bool skip = false;
			if (cat != inCat)
			{
				inCat = cat;
				if (inCat != "") {
					ImGui::PopID();
					skip = BeginPropGroup(cat, prop, catOpen);
					ImGui::PushID(ctx.selected[0]);
				}
				else 
					EndPropGroup(catOpen);
			}
			if (inCat != "" && !catOpen)
				continue;
			if (skip)
				continue;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			if (keyPressed && props[i].kbdInput) {
				addInputCharacter = keyPressed;
				ImGui::SetKeyboardFocusHere();
			}
			bool change = pr ? ctx.selected[0]->PropertyUI(i, ctx) : ctx.selected[0]->EventUI(i, ctx);
			if (change) {
				fileTabs[activeTab].modified = true;
				if (props[i].property) {
					pname = props[i].name;
					pval = props[i].property->to_arg();
				}
			}
		}
		if (inCat != "")
			EndPropGroup(catOpen);
		ImGui::Unindent();
		ImGui::PopID();
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
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
	
	ImGui::Begin("Events");
	if (!ctx.selected.empty())
	{
		PropertyRowsUI(0);
	}
	ImGui::End();

	ImGui::Begin("Properties");
	if (!ctx.selected.empty())
	{
		PropertyRowsUI(1);
	}
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

	settingsDlg.Draw();

	aboutDlg.Draw();

	bindingDlg.Draw();
	
	horizLayout.Draw();

	inputName.Draw();
}

void Draw()
{
	if (activeTab < 0 || !fileTabs[activeTab].rootNode)
		return;
	if (reloadStyle) //eliminates flicker
		return;
	
	auto& tab = fileTabs[activeTab];
	auto tmpStyle = ImGui::GetStyle();
	ImGui::GetStyle() = ctx.style;
	ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];
	ImGui::GetStyle().ScaleAllSizes(ctx.zoomFactor);
	ImGui::PushFont(ctx.defaultFont);
	
	ctx.workingDir = fs::path(tab.fname).parent_path().string();
	ctx.unit = tab.unit;
	ctx.modified = &tab.modified;
	tab.rootNode->Draw(ctx);
	
	if (ctx.isAutoSize && ctx.layoutHash != ctx.prevLayoutHash)
	{
		ctx.root->ResetLayout();
		if (ctx.rootWin)
			ctx.rootWin->HiddenFramesCannotSkipItems = 2; //flicker removal
	}

	ImGui::PopFont();
	ImGui::GetStyle() = tmpStyle;
}

std::vector<UINode*> SortSelection(const std::vector<UINode*>& sel)
{
	auto& tab = fileTabs[activeTab];
	std::vector<UINode*> allNodes = tab.rootNode->GetAllChildren();
	std::vector<UINode*> children;
	for (UINode* node : sel)
	{
		if (node == tab.rootNode.get())
			continue;
		auto it = stx::find(allNodes, node);
		if (it == allNodes.end())
			continue;
		auto ch = node->GetAllChildren();
		assert(ch.size() && ch[0] == node);
		children.insert(children.end(), ch.begin() + 1, ch.end());
	}
	std::vector<std::pair<int, UINode*>> sortedSel;
	for (UINode* node : sel)
	{
		if (node == tab.rootNode.get())
			continue;
		auto it = stx::find(allNodes, node);
		if (it == allNodes.end() || stx::find(children, node) != children.end())
			continue;
		sortedSel.push_back({ int(it - allNodes.begin()), node });
		auto ch = node->GetAllChildren();
		children.insert(children.end(), ch.begin(), ch.end());
	}

	stx::sort(sortedSel);
	allNodes.clear();
	for (const auto& sel : sortedSel)
		allNodes.push_back(sel.second);
	return allNodes;
}

std::vector<std::unique_ptr<Widget>> 
RemoveSelected()
{
	auto& tab = fileTabs[activeTab];
	std::vector<UINode*> sortedSel = SortSelection(ctx.selected);
	if (sortedSel.empty())
		return {};

	std::vector<std::unique_ptr<Widget>> remove;
	auto pi1 = tab.rootNode->FindChild(sortedSel[0]);
	for (UINode* node : sortedSel)
	{
		auto pi = tab.rootNode->FindChild(node);
		tab.modified = true;
		Widget* wdg = dynamic_cast<Widget*>(node);
		bool sameLine = wdg->sameLine;
		int nextColumn = wdg->nextColumn;
		int spacing = wdg->spacing;
		remove.push_back(std::move(pi->first->children[pi->second]));
		pi->first->children.erase(pi->first->children.begin() + pi->second);
		if (pi->second < pi->first->children.size() &&
			!stx::count(ctx.selected, pi->first->children[pi->second].get()))
		{
			wdg = dynamic_cast<Widget*>(pi->first->children[pi->second].get());
			wdg->nextColumn += nextColumn;
			if (!sameLine) {
				wdg->sameLine = false;
				wdg->spacing = spacing;
			}
		}
	}
	//move selection. Useful for things like menu items
	if (sortedSel.size() == 1 && pi1)
	{
		if (pi1->second < pi1->first->children.size())
			ctx.selected = { pi1->first->children[pi1->second].get() };
		else if (pi1->second)
			ctx.selected = { pi1->first->children[pi1->second - 1].get() };
		else
			ctx.selected = { pi1->first };
	}
	else
	{
		ctx.selected.clear();
	}

	return remove;
}

void Work()
{
	if (ImGui::GetTopMostAndVisiblePopupModal())
		return;

	if (initErrors != "")
	{
		messageBox.title = "Startup";
		messageBox.message = "Errors occured";
		messageBox.buttons = ImRad::Ok;
		messageBox.error = initErrors;
		messageBox.OpenPopup();
		initErrors = "";
	}
	else if (showError != "")
	{
		messageBox.title = "Error";
		messageBox.message = showError;
		messageBox.buttons = ImRad::Ok;
		messageBox.OpenPopup();
		showError = "";
	}

	if (programState == Shutdown)
	{
		CloseFile();
	}

	if (ctx.mode == UIContext::PickPoint)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		glfwSetCursor(window, curCross);
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ctx.mode = UIContext::NormalSelection;
			activeButton = "";
		}
		else if (!ImGui::GetIO().KeyCtrl)
		{
			ctx.mode = UIContext::Snap;
		}
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && 
			ctx.snapParent) 
		{
			assert(activeButton != "");
			newNode->hasPos = true;
			ImVec2 pos = ImGui::GetMousePos() - ctx.snapParent->cached_pos; //win->InnerRect.Min;
			if (pos.x < ctx.snapParent->cached_size.x / 2)
				newNode->pos_x = pos.x / ctx.zoomFactor;
			else
				newNode->pos_x = (pos.x - ctx.snapParent->cached_size.x) / ctx.zoomFactor;
			if (pos.y < ctx.snapParent->cached_size.y / 2)
				newNode->pos_y = pos.y / ctx.zoomFactor;
			else
				newNode->pos_y = (pos.y - ctx.snapParent->cached_size.y) / ctx.zoomFactor;

			ctx.selected = { newNode.get() };
			ctx.snapParent->children.push_back(std::move(newNode));
			ctx.mode = UIContext::NormalSelection;
			activeButton = "";
			fileTabs[activeTab].modified = true;
			ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event
		}
	}
	else if (ctx.mode == UIContext::Snap)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ctx.mode = UIContext::NormalSelection;
			activeButton = "";
		}
		else if (ImGui::GetIO().KeyCtrl && activeButton != "" && 
			(newNode->Behavior() & UINode::SnapSides))
		{
			ctx.mode = UIContext::PickPoint;
		}
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && //MouseReleased to avoid confusing RectSelection
			ctx.snapParent)
		{
			int n;
			std::unique_ptr<Widget>* newNodes;
			if (activeButton != "") {
				n = 1;
				newNodes = &newNode;
			}
			else {
				n = (int)clipboard.size();
				newNodes = clipboard.data();
			}
			if (n)
			{
				bool firstItem = true;
				for (size_t i = 0; i < ctx.snapIndex; ++i) 
				{
					auto* ch = ctx.snapParent->children[i].get();
					if (!ch->hasPos && (ch->Behavior() & UINode::SnapSides))
						firstItem = false;
				}
				newNodes[0]->sameLine = ctx.snapSameLine;
				newNodes[0]->spacing = firstItem ? 0 : 1;
				newNodes[0]->nextColumn = ctx.snapNextColumn;
			}
			if (ctx.snapIndex < ctx.snapParent->children.size())
			{
				auto& next = ctx.snapParent->children[ctx.snapIndex];
				if (ctx.snapSetNextSameLine) 
				{
					next->nextColumn = false;
					next->sameLine = true;
					if (!ctx.snapSameLine) {
						newNodes[0]->spacing = next->spacing;
						next->spacing = 1;
					}
					else
						next->spacing = std::max((int)next->spacing, 1);
				}
				if (ctx.snapClearNextNextColumn) 
				{
					next->nextColumn = 0;
					next->sameLine = false;
					next->spacing = std::max((int)next->spacing, 1);
				}
				if (next->sameLine)
					next->indent = 0; //otherwise creates widgets overlaps
			}
			ctx.selected.clear();
			if (activeButton != "")
			{
				ctx.selected.push_back(newNodes[0].get());
				ctx.snapParent->children.insert(ctx.snapParent->children.begin() + ctx.snapIndex, std::move(newNodes[0]));
			}
			else
			{
				for (int i = 0; i < n; ++i)
				{
					//paste original widgets and push cloned widgets back to the clipboard
					//so original variables will be used in a pasted widget
					auto wdg = newNodes[i]->Clone(ctx);
					ctx.selected.push_back(newNodes[i].get());
					ctx.snapParent->children.insert(ctx.snapParent->children.begin() + ctx.snapIndex + i, std::move(newNodes[i]));
					newNodes[i] = std::move(wdg);
				}
			}
			ctx.mode = UIContext::NormalSelection;
			activeButton = "";
			fileTabs[activeTab].modified = true;
			ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left] = false; //eat event
		}
	}
	else 
	{
		//don't IsMouseReleased otherwise closing modal popup will fire here too
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			ctx.rootWin &&
			ImRect(ctx.designAreaMin, ctx.designAreaMax).Contains(ImGui::GetMousePos()) &&
			!ImRect(ctx.rootWin->Pos, ctx.rootWin->Pos + ctx.rootWin->SizeFull).Contains(ImGui::GetMousePos()))
		{
			bool skip = false;
			for (ImGuiWindow* popup : ctx.activePopups)
				if (popup->Rect().Contains(ImGui::GetMousePos()))
					skip = true;
			if (!skip)
				ctx.selected = { ctx.root };
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_KeypadAdd, ImGuiInputFlags_RouteGlobal) ||
			ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal, ImGuiInputFlags_RouteGlobal))
		{
			ctx.zoomFactor = std::round(100 * ctx.zoomFactor * 1.2f) / 100.f;
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_KeypadSubtract, ImGuiInputFlags_RouteGlobal) ||
			ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus, ImGuiInputFlags_RouteGlobal))
		{
			ctx.zoomFactor = std::round(100 * ctx.zoomFactor / 1.2f) / 100.f;
		}
		if (!ImGui::GetIO().WantTextInput)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Delete))
			{
				RemoveSelected();
			}
			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, ImGuiInputFlags_RouteGlobal) &&
				!ctx.selected.empty() &&
				ctx.selected[0] != fileTabs[activeTab].rootNode.get())
			{
				clipboard.clear();
				ctx.createVars = false;
				auto sortedSel = SortSelection(ctx.selected);
				for (UINode* node : sortedSel)
				{
					auto* wdg = dynamic_cast<Widget*>(node);
					clipboard.push_back(wdg->Clone(ctx));
				}
				ctx.createVars = true;
			}
			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_X, ImGuiInputFlags_RouteGlobal) &&
				!ctx.selected.empty() &&
				ctx.selected[0] != fileTabs[activeTab].rootNode.get())
			{
				clipboard = RemoveSelected();
			}
			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V, ImGuiInputFlags_RouteGlobal) &&
				clipboard.size())
			{
				activeButton = "";
				ctx.mode = UIContext::Snap;
				ctx.selected = {};
			}
		}
	}
}

//This writes and reads records from [Recent] section
void AddINIHandler()
{
	ImGuiSettingsHandler ini_handler;
	ini_handler.TypeName = "ImRAD";
	ini_handler.TypeHash = ImHashStr("ImRAD");
	ini_handler.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void*
		{
			return (void*)name;
		};
	ini_handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
		{
			if (programState != Init)
				return;
			char buf[1000];
			if (!strcmp((const char*)entry, "Recent")) {
				int i;
				if (sscanf(line, "File%d=%s", &i, buf) == 2) {
					if (i == 1) {
						fileTabs.clear();
						ActivateTab(-1);
					}
					DoOpenFile(buf, &initErrors);
				}
				else if (sscanf(line, "ActiveTab=%d", &i) == 1) {
					ActivateTab(i);
				}
			}
			else if (!strcmp((const char*)entry, "UI")) {
				if (!strncmp(line, "FontName=", 9))
					fontName = line + 9;
				else if (!strncmp(line, "FontSize=", 9))
					fontSize = (float)std::atof(line + 9);
			}
		};
	ini_handler.ApplyAllFn = nullptr; 
	ini_handler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) 
		{
			buf->append("[ImRAD][Recent]\n");
			for (int i = 0; i < fileTabs.size(); ++i) {
				buf->appendf("File%d=%s\n", i+1, fileTabs[i].fname.c_str());
			}
			buf->appendf("ActiveTab=%d\n", activeTab);
			buf->append("\n");

			buf->append("[ImRAD][UI]\n");
			buf->appendf("FontName=%s\n", fontName.c_str());
			buf->appendf("FontSize=%f\n", fontSize);
			buf->append("\n");
		};
	ImGui::GetCurrentContext()->SettingsHandlers.push_back(ini_handler);
}

std::string GetRootPath()
{
#ifdef WIN32
	wchar_t tmp[1024];
	int n = GetModuleFileNameW(NULL, tmp, sizeof(tmp));
	return fs::path(tmp).parent_path().generic_string(); //need generic for CMake path output
#elif __APPLE__
	char executablePath[PATH_MAX];
	uint32_t len;
	if (_NSGetExecutablePath(executablePath, &len) != 0) {
		IM_ASSERT_USER_ERROR(0, "Could not get root path!");
	}
	fs::path pexe = fs::canonical(executablePath);
	return pexe.parent_path().string();
#else
	fs::path pexe = fs::canonical("/proc/self/exe");
	return pexe.parent_path().string();
#endif
}

#ifdef WIN32
int WINAPI wWinMain(
	HINSTANCE   hInstance,
	HINSTANCE   hPrevInstance,
	PWSTR       lpCmdLine,
	int         nCmdShow
) 
{
#else
int main(int argc, const char* argv[]) 
{
#endif	
	rootPath = GetRootPath();

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

	// Initialize the native file dialog
	NFD_Init();

	// Create window with graphics context
	window = glfwCreateWindow(1280, 720, VER_STR.c_str(), NULL, NULL);
	if (window == NULL)
		return 1;
	GLFWimage icons[2];
	icons[0].pixels = stbi_load((rootPath + "/style/icon-40.png").c_str(), &icons[0].width, &icons[0].height, 0, 4);
	icons[1].pixels = stbi_load((rootPath + "/style/icon-100.png").c_str(), &icons[1].width, &icons[1].height, 0, 4);
	if (icons[0].pixels && icons[1].pixels)
		glfwSetWindowIcon(window, 2, icons);
	stbi_image_free(icons[0].pixels);
	stbi_image_free(icons[1].pixels);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync
	glfwMaximizeWindow(window);
	curCross = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& g = *ImGui::GetCurrentContext();
	g.ConfigNavWindowingKeyNext = g.ConfigNavWindowingKeyPrev = ImGuiKey_None; //disable imgui ctrl+tab menu
	ImGui::GetIO().IniFilename = INI_FILE_NAME;
	AddINIHandler();
	ImGuiIO& io = ImGui::GetIO(); 
	io.UserData = &ioUserData;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	
	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ctx.dashTexId = ImRad::LoadTextureFromFile(
		(rootPath + "/style/dash.png").c_str(), GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT).id;
	
	GetStyles();
	programState = (ProgramState)-1;
	bool lastVisible = true;
	while (true)
	{
		if (programState == -1)
		{
			programState = Init;
		}
		else if (programState == Init)
		{
			programState = Run;
		}
		else if (programState == Shutdown) 
		{
			if (fileTabs.empty()) //Work() will close the tabs
				break;
		}
		else if (programState == Run && glfwWindowShouldClose(window))
		{
			if (ImGui::GetTopMostAndVisiblePopupModal())
			{
				glfwSetWindowShouldClose(window, false);
			}
			else
			{	
				programState = Shutdown;
				//save state before files close
				ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
				ImGui::GetIO().IniFilename = nullptr;

			}
		}

		//font loading must be called before NewFrame
		LoadStyle();

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
			ReloadFile();
		lastVisible = visible;

		DockspaceUI();
		ToolbarUI();
		TabsUI();
		HierarchyUI();
		PropertyUI();
		PopupUI();
		Work();
		Draw(); //last
		
		//ImGui::ShowDemoWindow();

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
	NFD_Quit();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyCursor(curCross);
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
