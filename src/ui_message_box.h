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
	enum Buttons { OK = 1, Cancel = 2, Yes = 4, No = 8 };
	int buttons = OK;

private:
	bool requestOpen = false;
	std::function<void(ImRad::ModalResult)> callback;
};

extern MessageBox messageBox;