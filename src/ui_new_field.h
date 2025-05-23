#pragma once
#include "cppgen.h"
#include <imgui.h>
#include <string>
#include <functional>

class NewFieldPopup
{
public:
    void OpenPopup(std::function<void()> = []{});
    void ClosePopup();
    void Draw();

public:
    std::string varType;
    std::string varOldName;
    std::string varName; //auto cleared except for NewEvent
    std::string varInit; //auto filled
    bool varPublic;
    std::string scope;
    CppGen* codeGen = nullptr;
    enum mode_t { NewField, NewStruct, NewEvent, RenameField, RenameStruct, RenameWindow };
    mode_t mode = NewField;

private:
    void CheckVarName();

    ImGuiID ID;
    bool requestClose;
    std::function<void()> callback;

    bool varTypeDisabled = false;
    bool varTypeArray = false;
    std::string nameError, typeError;
    ImU32 clr;
};

extern NewFieldPopup newFieldPopup;