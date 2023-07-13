#pragma once
#include <imgui.h>
#include <string>
#include <functional>
#include "imrad.h"

class MessageBox
{
public:
	void OpenPopup(std::function<void(ImRad::ModalResult)> f = [](ImRad::ModalResult){});
	void Draw();

public:
	std::string title = "title";
	std::string message;
	std::string error;
	int buttons = ImRad::Ok;

private:
	ImGuiID ID = 0;
	std::function<void(ImRad::ModalResult)> callback;
};

extern MessageBox messageBox;