#include "ui_new_field.h"
#include "ui_message_box.h"
#include "cppgen.h"
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_internal.h>

NewFieldPopup newFieldPopup;

void NewFieldPopup::OpenPopup(std::function<void()> clb)
{
    callback = clb;
    requestClose = false;
    ImGui::OpenPopup(ID);

    if (mode == NewStruct)
        varType = "struct";
    varTypeDisabled = varType != "";
    varTypeArray = varType == "[]";
    if (varTypeArray)
        varType = "std::vector<>";
    if (mode != NewEvent)
        varName = "";
    varInit = "";
    varPublic = mode == NewEvent ? false : true;
    if (mode == RenameField) {
        varName = varOldName;
        auto* var = codeGen->GetVar(varOldName);
        if (var) {
            varInit = var->init;
            varType = var->type;
            varPublic = var->flags & CppGen::Var::Interface;
        }
    }
    CheckVarName();
}

void NewFieldPopup::ClosePopup()
{
    requestClose = true;
}

void NewFieldPopup::Draw()
{
    std::string title = mode == NewField ? "New Field" :
        mode == NewStruct ? "New Struct" :
        mode == NewEvent ? "New Method" :
        mode == RenameField ? "Rename Field" :
        mode == RenameStruct ? "Rename Struct" :
        "Rename Window";
    title += "###NewFieldPopup";

    ID = ImGui::GetID("###NewFieldPopup");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12, 12 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10, 10 });
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSizeConstraints({ 400, 100 }, { FLT_MAX, FLT_MAX });
    bool tmp = true;
    if (ImGui::BeginPopupModal(title.c_str(), &tmp, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (requestClose) {
            ImGui::CloseCurrentPopup();
            requestClose = false;
        }

        messageBox.Draw();

        bool change = false;

        if (mode == RenameField || mode == RenameStruct || mode == RenameWindow)
        {
            ImGui::Text("Old name");
            ImGui::BeginDisabled(true);
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##varOldName", &varOldName, ImGuiInputTextFlags_CharsNoBlank);
            ImGui::EndDisabled();
            ImGui::Spacing();
        }

        ImGui::Text(
            mode == RenameWindow || mode == RenameStruct || mode == RenameField ? "New name" :
            mode == NewEvent ? "Method name" :
            mode == NewStruct ? "Struct name" :
            "Field name"
        );

        ImGui::SameLine();
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleColor(ImGuiCol_Text, 0xff0000ff);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 1.0f, 0 });
        ImGui::Selectable((nameError + "##nameError").c_str(), false);
        ImGui::PopStyleVar();
        ImGui::PopItemFlag();
        ImGui::PopStyleColor();

        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##varName", &varName, ImGuiInputTextFlags_CharsNoBlank))
            change = true;

        if (mode == NewField || mode == NewStruct || mode == NewEvent)
        {
            ImGui::Text("Field type");

            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleColor(ImGuiCol_Text, 0xff0000ff);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 1.0f, 0 });
            ImGui::Selectable((typeError + "##typeError").c_str(), false);
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::PopItemFlag();

            ImGui::BeginDisabled(varTypeDisabled);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##varType", &varType, ImGuiInputTextFlags_CharsNoBlank))
                change = true;
            ImGui::EndDisabled();
        }

        if (mode == NewField || mode == RenameField)
        {
            ImGui::Spacing();
            ImGui::Text("Initial value (optional)");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##init", &varInit, ImGuiInputTextFlags_CallbackCharFilter, DefaultCharFilter))
                change = true;
        }

        if (mode == NewField || mode == RenameField)
        {
            ImGui::Spacing();
            ImGui::Text("Access");
            if (ImGui::BeginChild("access", { -1, 0 }, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
            {
                if (ImGui::RadioButton("Interface", varPublic))
                    varPublic = true;
                ImGui::SameLine(0, 3 * ImGui::GetStyle().ItemSpacing.x);
                if (ImGui::RadioButton("Implementation", !varPublic))
                    varPublic = false;
                ImGui::EndChild();
            }
        }

        if (change)
        {
            if (varTypeArray && varName != "") {
                std::string stype = (char)std::toupper(varName[0]) + varName.substr(1);
                if (stype.back() == 's' || stype.back() == 'S')
                    stype.pop_back();
                varType = "std::vector<" + stype + ">";
            }

            CheckVarName();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - 2 * 100));
        ImGui::BeginDisabled(nameError != "" || typeError != "");
        if (ImGui::Button("OK", { 100, 30 }))
        {
            CheckVarName();
            int flags = varPublic ? CppGen::Var::Interface : CppGen::Var::Impl;

            if (mode == NewField || mode == NewStruct || mode == NewEvent)
            {
                std::string type = mode == NewStruct ? "struct" : varType;
                auto ret = codeGen->CheckVarExpr(varName, type, scope);
                if (ret == CppGen::New_ImplicitStruct) {
                    messageBox.title = "New";
                    messageBox.message = "Create a new struct?";
                    messageBox.buttons = ImRad::Yes | ImRad::No;
                    messageBox.OpenPopup([this, type, flags](ImRad::ModalResult mr) {
                        if (mr != ImRad::Yes)
                            return;
                        codeGen->CreateVarExpr(varName, type, varInit, flags, scope);
                        ClosePopup();
                        callback();
                        });
                }
                else if (ret == CppGen::New) {
                    codeGen->CreateVarExpr(varName, type, varInit, flags, scope);
                    ClosePopup();
                    callback();
                }
            }
            else if (mode == RenameField)
            {
                assert(varType != "");
                codeGen->ChangeVar(varOldName, varType, varInit, flags, scope);
                codeGen->RenameVar(varOldName, varName, scope);
                ClosePopup();
                callback();
            }
            else if (mode == RenameStruct)
            {
                codeGen->RenameStruct(varOldName, varName);
                ClosePopup();
                callback();
            }
            else if (mode == RenameWindow)
            {
                codeGen->SetNamesFromId(varName);
                ClosePopup();
                callback();
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 100, 30 }) || ImGui::Shortcut(ImGuiKey_Escape))
        {
            const auto& g = ImGui::GetCurrentContext();
            ClosePopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

void NewFieldPopup::CheckVarName()
{
    nameError = typeError = "";
    if (mode == RenameWindow)
    {
        if (!cpp::is_id(varName)) {
            nameError = "expected simple id";
        }
    }
    else if (mode == RenameStruct)
    {
        if (!cpp::is_id(varName)) {
            nameError = "expected simple id";
        }
        else if (varName != varOldName && stx::count(codeGen->GetStructTypes(), varName)) {
            nameError = "same field already exists";
        }
    }
    else if (mode == RenameField)
    {
        if (!cpp::is_id(varName)) {
            nameError = "expected simple id";
        }
        else if (varName != varOldName && codeGen->CheckVarExpr(varName, varType, scope) != CppGen::New) {
            nameError = "same field already exists";
        }
    }
    else if (mode == NewStruct || mode == NewEvent)
    {
        std::string type = mode == NewStruct ? "struct" : varType;
        if (!cpp::is_id(varName)) {
            nameError = "expected simple id";
        }
        else if (codeGen->CheckVarExpr(varName, type, scope) != CppGen::New) {
            nameError = "same field already exists";
        }
    }
    else if (mode == NewField)
    {
        switch (codeGen->CheckVarExpr(varName, varType, scope))
        {
        case CppGen::SyntaxError:
            nameError = "expected simple id";
            break;
        case CppGen::ConflictError:
            nameError = "field of different type already exists";
            break;
        case CppGen::Existing:
            nameError = "same field already exists";
            break;
        default:
            if (varTypeArray && !cpp::is_id(varName)) {
                nameError = "expected simple id";
            }
            else if (varType == "") {
                typeError = "invalid type";
            }
            break;
        }
    }
}
