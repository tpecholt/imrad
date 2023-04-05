#pragma once
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include <functional>

class MessageBoxxx
{
public:
	void OpenPopup(std::function<void()> f = {});
	void Draw();

public:
	std::string title = "title";
	std::string message;
	std::string error;
	enum Buttons { OK = 1, Cancel = 2, Yes = 4, No = 8 };
	int buttons = OK;

private:
	ImGuiID ID;
	std::function<void()> callback;
};

extern MessageBoxxx messageBox;