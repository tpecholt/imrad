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
    if (mode == RenameField) {
        varName = varOldName;
        auto* var = codeGen->GetVar(varOldName);
        if (var) {
            varInit = var->init;
            varType = var->type;
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
    ImGui::SetNextWindowSizeConstraints({ 400, 350 }, { FLT_MAX, FLT_MAX });
    bool tmp = true;
    if (ImGui::BeginPopupModal(title.c_str(), &tmp, 0))
    {
        if (requestClose) {
            ImGui::CloseCurrentPopup();
            requestClose = false;
        }

        messageBox.Draw();

        bool change = false;

        if (mode == RenameField || mode == RenameStruct || mode == RenameWindow)
        {
            ImGui::Text("Old name:");
            ImGui::BeginDisabled(true);
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##varOldName", &varOldName, ImGuiInputTextFlags_CharsNoBlank);
            ImGui::EndDisabled();
        }
        else
        {
            ImGui::Text("Field type:");
            ImGui::BeginDisabled(varTypeDisabled);
            if (ImGui::IsWindowAppearing() && varType == "")
                ImGui::SetKeyboardFocusHere();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##varType", &varType, ImGuiInputTextFlags_CharsNoBlank))
                change = true;
            ImGui::EndDisabled();
        }

        ImGui::Spacing();
        ImGui::Text(mode == RenameWindow || mode == RenameStruct ? "New name:" :
            mode == NewEvent ? "Method name:" :
            "Field name:");
        if (ImGui::IsWindowAppearing() && varType != "")
            ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##varName", &varName, ImGuiInputTextFlags_CharsNoBlank))
            change = true;

        if (mode == NewField || mode == RenameField)
        {
            ImGui::Spacing();
            ImGui::Text("Initial value:");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##init", &varInit, ImGuiInputTextFlags_CallbackCharFilter, DefaultCharFilter))
                change = true;
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

        ImGui::PushStyleColor(ImGuiCol_Text, clr);
        ImGui::TextWrapped("%s", hint.c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - 30);
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - 2 * 100));
        ImGui::BeginDisabled(hint != "");
        if (ImGui::Button("OK", { 100, 30 }))
        {
            CheckVarName();

            if (mode == NewField || mode == NewStruct || mode == NewEvent)
            {
                std::string type = mode == NewStruct ? "struct" : varType;
                auto ret = codeGen->CheckVarExpr(varName, type, scope);
                if (ret == CppGen::New_ImplicitStruct) {
                    messageBox.title = "New";
                    messageBox.message = "Create a new struct?";
                    messageBox.buttons = ImRad::Yes | ImRad::No;
                    messageBox.OpenPopup([this, type](ImRad::ModalResult mr) {
                        if (mr != ImRad::Yes)
                            return;
                        codeGen->CreateVarExpr(varName, type, scope);
                        ClosePopup();
                        callback();
                        });
                }
                else if (ret == CppGen::New) {
                    codeGen->CreateVarExpr(varName, type, varInit, scope);
                    ClosePopup();
                    callback();
                }
            }
            else if (mode == RenameField)
            {
                assert(varType != "");
                codeGen->ChangeVar(varOldName, varType, varInit, scope);
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
    hint = "";
    if (mode == RenameWindow)
    {
        if (!cpp::is_id(varName)) {
            hint = "expected simple id";
            clr = IM_COL32(255, 0, 0, 255);
        }
    }
    else if (mode == RenameStruct)
    {
        if (!cpp::is_id(varName)) {
            hint = "expected simple id";
            clr = IM_COL32(255, 0, 0, 255);
        }
        else if (varName != varOldName && stx::count(codeGen->GetStructTypes(), varName)) {
            hint = "same field already exists";
            clr = IM_COL32(255, 0, 0, 255);
        }
    }
    else if (mode == RenameField)
    {
        if (!cpp::is_id(varName)) {
            hint = "expected simple id";
            clr = IM_COL32(255, 0, 0, 255);
        }
        else if (varName != varOldName && codeGen->CheckVarExpr(varName, varType, scope) != CppGen::New) {
            hint = "same field already exists";
            clr = IM_COL32(255, 0, 0, 255);
        }
    }
    else if (mode == NewStruct || mode == NewEvent)
    {
        std::string type = mode == NewStruct ? "struct" : varType;
        if (!cpp::is_id(varName)) {
            hint = "expected simple id";
            clr = IM_COL32(255, 0, 0, 255);
        }
        else if (codeGen->CheckVarExpr(varName, type, scope) != CppGen::New) {
            hint = "same field already exists";
            clr = IM_COL32(255, 0, 0, 255);
        }
    }
    else if (mode == NewField)
    {
        if (varTypeArray && !cpp::is_id(varName)) {
            hint = "expected simple id";
            clr = IM_COL32(255, 0, 0, 255);
        }
        else if (varType == "") {
            hint = "syntax error";
            clr = IM_COL32(255, 0, 0, 255);
        }
        else switch (codeGen->CheckVarExpr(varName, varType, scope))
        {
        case CppGen::SyntaxError:
            hint = "syntax error";
            clr = IM_COL32(255, 0, 0, 255);
            break;
        case CppGen::ConflictError:
            hint = "field of different type already exists";
            clr = IM_COL32(255, 0, 0, 255);
            break;
        case CppGen::Existing:
            hint = "same field already exists";
            clr = IM_COL32(255, 128, 0, 255);
            break;
        default:
            break;
        }
    }
}
