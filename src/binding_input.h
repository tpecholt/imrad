#pragma once
#include <array>
#include <optional>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include "stx.h"
#include "utils.h"
#include "binding_property.h"
#include "cppgen.h"
#include "ui_new_field.h"
#include "ui_binding.h"


template <class T>
inline bool BindingButton(const char* label, bindable<T>* val, const std::string& type, UIContext& ctx)
{
    if (val == ctx.setProp)
    {
        //doing it here imrad.cpp will be able to set property in all selected widgets
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool bound;
    if constexpr (std::is_same_v<T, std::string> || 
                  std::is_same_v<T, std::vector<std::string>> ||
                  std::is_same_v<T, color32>)
        bound = !val->used_variables().empty();
    else
        bound = !val->empty() && !val->has_value();
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bound ? 0xff0080ff : 0xffffffff);
    float sp = (ImGui::GetFrameHeight() - 11) / 2;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sp);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + sp);
    ImGui::BeginChild(("child" + std::string(label)).c_str(), { 11, 11 }, true);
    bool pushed = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()
        && !ImRad::IsItemDisabled();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    if (pushed)
    {
        bindingDlg.font = type.find("std::string") != std::string::npos ? ctx.defaultStyleFont : nullptr;
        bindingDlg.codeGen = ctx.codeGen;
        bindingDlg.name = label;
        bindingDlg.expr = val->c_str();
        bindingDlg.type = type;
        bindingDlg.OpenPopup([&ctx, val](ImRad::ModalResult) {
            ctx.setProp = val;
            ctx.setPropValue = bindingDlg.expr;
            });
        return true;
    }
    
    return false;
}

template <class T>
inline bool BindingButton(const char* label, bindable<T>* val, UIContext& ctx)
{
    return BindingButton(label, val, typeid_name<T>(), ctx);
}

//---------------------------------------------------------------------------------

inline bool InputDirectVal(direct_val<dimension>* val, UIContext& ctx)
{
    std::string id = "##" + std::to_string((uint64_t)val);
    return ImGui::InputFloat(id.c_str(), val->access(), 0, 0, "%.0f");
}

inline bool InputDirectVal(direct_val<pzdimension>* val, UIContext& ctx)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(val->has_value() ? ImGuiCol_Text : ImGuiCol_TextDisabled));
    std::string id = "##" + std::to_string((uint64_t)val);
    bool ch = ImGui::InputFloat(id.c_str(), val->access(), 0, 0, "%.0f");
    ImGui::PopStyleColor();
    return ch;
}

inline bool InputDirectVal(direct_val<pzdimension2>* val, UIContext& ctx)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(val->has_value() ? ImGuiCol_Text : ImGuiCol_TextDisabled));
    std::string id = "##" + std::to_string((uint64_t)val);
    bool ch = ImGui::InputFloat2(id.c_str(), (float*)val->access(), "%.0f");
    ImGui::PopStyleColor();
    return ch;
}

inline bool InputDirectVal(direct_val<bool>* val, UIContext& ctx)
{
    std::string id = "##" + std::to_string((uint64_t)val);
    return ImGui::Checkbox(id.c_str(), val->access());
}

inline bool InputDirectVal(direct_val<std::string>* val, UIContext& ctx)
{
    ImGui::PushFont(ctx.defaultStyleFont);
    std::string id = "##" + std::to_string((uint64_t)val);
    bool ch = ImGui::InputText(id.c_str(), val->access(), ImGuiInputTextFlags_CallbackCharFilter, InputTextCharExprFilter);
    ImGui::PopFont();
    return ch;
}

template <class T, class = std::enable_if_t<std::is_enum_v<T>>>
bool InputDirectVal(direct_val<T>* val, UIContext& ctx)
{
    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    if (ImGui::BeginCombo(id.c_str(), val->get_id().c_str()))
    {
        changed = true;
        for (const auto& item : val->get_ids())
        {
            if (ImGui::Selectable(item.first.c_str(), *val->access() == item.second))
                *val->access() = item.second;
        }
        ImGui::EndCombo();
    }
    return changed;
}

inline bool InputDirectVal(direct_val<shortcut_>* val, bool button, UIContext& ctx)
{
    if (button) {
        const auto& nextItemData = ImGui::GetCurrentContext()->NextItemData;
        bool hasWidth = nextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth;
        ImGui::SetNextItemWidth((hasWidth ? nextItemData.Width : 0) - ImGui::GetFrameHeight());
    }

    std::string id = "##" + std::to_string((uint64_t)val);
    bool changed = ImGui::InputText(id.c_str(), val->access(), ImGuiInputTextFlags_CallbackCharFilter, InputTextCharExprFilter);

    if (button) 
    {
        ImGui::SameLine(0, 0);
        std::string buttonId = id + "##But";
        std::string popupId = id + "##DropDown";
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[val->flags() ? ImGuiCol_ButtonActive : ImGuiCol_Button]);
        if (ImGui::Button((ICON_FA_ELLIPSIS + buttonId).c_str(), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
        {
            ImGui::OpenPopup(popupId.c_str());
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8, 8 });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 8 });
        if (ImGui::BeginPopup(popupId.c_str()))
        {
            if (ImGui::MenuItem("Global", nullptr, val->flags() & ImGuiInputFlags_RouteGlobal))
                val->set_flags(val->flags() ^ ImGuiInputFlags_RouteGlobal);
            if (ImGui::MenuItem("Repeat", nullptr, val->flags() & ImGuiInputFlags_Repeat))
                val->set_flags(val->flags() ^ ImGuiInputFlags_Repeat);
            
           ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
    }

    return changed;
}

//-------------------------------------------------------------------------------

inline bool InputBindable(bindable<bool>* val, bool defval, UIContext& ctx)
{
    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    if (ImGui::BeginCombo(id.c_str(), val->c_str()))
    {
        if (ImGui::Selectable("true", val->has_value() && val->value()))
        {
            *val = true; 
            changed = true;
        }
        if (ImGui::Selectable("false", val->has_value() && !val->value()))
        {
            *val = false; 
            changed = true;
        }
        ImGui::EndCombo();
    }

    return changed;
}

inline bool InputBindable(bindable<font_name>* val, UIContext& ctx)
{
    std::string fn = val->has_value() ? val->eval(ctx) : val->c_str();
    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    if (ImGui::BeginCombo(id.c_str(), fn.c_str()))
    {
        for (const auto& f : ctx.fontNames)
        {
            if (ImGui::Selectable(f == "" ? " " : f.c_str(), f == fn)) {
                changed = true;
                val->set_font_name(f);
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

inline bool InputBindable(bindable<color32>* val, int def, UIContext& ctx)
{
    static std::string lastColor;
    static int lastStyleClr;
    
    auto nextData = ImGui::GetCurrentContext()->NextItemData; //copy
    int styleClr = val->style_color();
    ImVec4 buttonClr = val->empty() ? 
        ctx.style.Colors[def] : 
        ImGui::ColorConvertU32ToFloat4(val->eval(def, ctx));
    std::string id = "##" + std::to_string((uint64_t)val);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    if (ImGui::ColorButton(id.c_str(), buttonClr, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview))
    {
        lastColor = val->c_str();
        lastStyleClr = styleClr;
        ImGui::OpenPopup(id.c_str());
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    ImVec2 sz{ 0, 0 };
    if (nextData.HasFlags & ImGuiNextItemDataFlags_HasWidth)
    {
        sz.x = nextData.Width;
        if (sz.x < 0)
            sz.x += ImGui::GetContentRegionAvail().x;
    }
    std::string clrName = styleClr >= 0 ? ImGui::GetStyleColorName(styleClr) : 
        val->empty() ? ImGui::GetStyleColorName(def) :
        val->c_str();
    clrName += id;
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[val->empty() ? ImGuiCol_TextDisabled : ImGuiCol_Text]);
    if (ImGui::Selectable(clrName.c_str(), false, 0, sz))
    {
        lastColor = val->c_str();
        lastStyleClr = styleClr;
        ImGui::OpenPopup(id.c_str());
    }
    ImGui::PopStyleColor();

    static color32 COLORS[]{
        /* 5x5 palette
        IM_COL32(255, 255, 255, 255),
        IM_COL32(246, 246, 194, 255),
        IM_COL32(202, 23, 55, 255),
        IM_COL32(246, 58, 56, 255),
        IM_COL32(162, 34, 31, 255),
        IM_COL32(7, 17, 115, 255),
        IM_COL32(159, 215, 252, 255),
        IM_COL32(15, 63, 25, 255),
        IM_COL32(96, 231, 66, 255),
        IM_COL32(205, 255, 202, 255),
        IM_COL32(246, 253, 85, 255),
        IM_COL32(248, 117, 0, 255),
        IM_COL32(244, 180, 118, 255),
        IM_COL32(104, 102, 90, 255),
        IM_COL32(165, 166, 160, 255),
        IM_COL32(54, 8, 81, 255),
        IM_COL32(148, 35, 213, 255),
        IM_COL32(248, 181, 248, 255),
        IM_COL32(204, 22, 182, 255),
        IM_COL32(219, 190, 2, 255),
        IM_COL32(10, 226, 211, 255),
        IM_COL32(2, 251, 171, 255),
        IM_COL32(143, 105, 69, 255),
        IM_COL32(59, 33, 8, 255),
        IM_COL32(0, 0, 0, 255),*/

        //8x5 palette
        IM_COL32(0,0,0,255),
        IM_COL32(152,51,0,255),
        IM_COL32(51,51,0,255),
        IM_COL32(0,51,0,255),
        IM_COL32(0,51,102,255),
        IM_COL32(0,0,128,255),
        IM_COL32(51,51,153,255),
        IM_COL32(50,52,50,255),

        IM_COL32(128,0,0,255),
        IM_COL32(255,102,0,255),
        IM_COL32(128,128,0,255),
        IM_COL32(0,128,0,255),
        IM_COL32(0,128,128,255),
        IM_COL32(0,0,255,255),
        IM_COL32(102,102,153,255),
        IM_COL32(127,129,127,255),

        IM_COL32(255,0,0,255),
        IM_COL32(255,153,0,255),
        IM_COL32(153,204,0,255),
        IM_COL32(51,153,102,255),
        IM_COL32(51,204,204,255),
        IM_COL32(51,102,255,255),
        IM_COL32(128,0,128,255),
        IM_COL32(152,154,152,255),

        IM_COL32(254,0,255,255),
        IM_COL32(255,203,0,255),
        IM_COL32(255,255,0,255),
        IM_COL32(0,255,0,255),
        IM_COL32(0,255,254,255),
        IM_COL32(0,204,255,255),
        IM_COL32(153,51,102,255),
        IM_COL32(194,194,194,255),

        IM_COL32(255,153,204,255),
        IM_COL32(255,204,151,255),
        IM_COL32(255,255,153,255),
        IM_COL32(205,255,205,255),
        IM_COL32(205,255,255,255),
        IM_COL32(153,204,254,255),
        IM_COL32(204,154,255,255),
        IM_COL32(255,255,255,255),
    };
    bool changed = false;
    static std::string lastOpen;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 5 });
    ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos() + ImVec2{ ImGui::GetContentRegionAvail().x, 0 }, ImGuiCond_Always, { 1, 0 });
    if (ImGui::BeginPopup(id.c_str(), ImGuiWindowFlags_AlwaysAutoResize))
    {
        lastOpen = id.c_str();
        *val->access() = lastColor;
        //if (ImGui::Button("Automatic", { -ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
        bool autoSel = ImGui::ColorButton("tooltip", ctx.style.Colors[def], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview);
        bool autoHover = ImGui::IsItemHovered();
        ImGui::SameLine(0, 0);
        ImGui::Selectable("  Automatic", false, ImGuiSelectableFlags_NoPadWithHalfSpacing, 
            { ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight(), 0 });
        autoHover = autoHover || ImGui::IsItemHovered();
        autoSel = autoSel || ImGui::IsItemClicked(ImGuiMouseButton_Left);
        if (autoSel)
        {
            changed = true;
            lastColor = "";
            *val = color32();
        }
        if (autoHover)
        {
            *val = color32();
        }
        ImGui::SameLine(0, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4, 4 });
        const color32 trc({ 1, 1, 1, 0 });
        ImGui::PushStyleColor(ImGuiCol_Button, trc);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, trc);
        if (ImGui::ColorButton("##transparent", { 1, 1, 1, 0 }, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview))
        //if (ImGui::Button("##transparent", { 30, 30 }))
        {
            changed = true;
            *val = trc;
            lastColor = *val->access();
        }
        if (ImGui::IsItemHovered())
        {
            *val = trc;
        }
        ImGui::PopStyleColor(2);

        for (int i = 0; i < stx::ssize(COLORS); ++i)
        {
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, COLORS[i]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COLORS[i]);
            if (ImGui::Button("##clr", { 30, 30 }))
            {
                changed = true;
                *val = COLORS[i];
                lastColor = *val->access();
            }
            if (ImGui::IsItemHovered())
            {
                *val = COLORS[i];
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();
            if ((i + 1) % 8)
                ImGui::SameLine();
        }
        ImGui::PopStyleVar(2);
        ImGui::Spacing();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        int clri = val->style_color();
        std::string clrName = lastStyleClr >= 0 ? ImGui::GetStyleColorName(lastStyleClr) : "Style Color...";
        if (ImGui::BeginCombo("##cmb", clrName.c_str()))
        {
            for (int i = 0; i < ImGuiCol_COUNT; ++i)
            {
                auto cc = ctx.style.Colors[i];
                auto ccc = ImGui::ColorConvertFloat4ToU32(cc);
                ImGui::ColorButton("##color", cc, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_AlphaPreview);
                ImGui::SameLine();
                if (ImGui::Selectable(ImGui::GetStyleColorName(i), clri == i))
                {
                    changed = true;
                    val->set_style_color(i);
                    lastColor = *val->access();
                    break;
                }
                ImRect r{ ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
                r.Min.y -= ImGui::GetStyle().ItemSpacing.y;
                if (r.Contains(ImGui::GetMousePos())) //more stable than IsItemHovered
                {
                    val->set_style_color(i);
                }
            }
            ImGui::EndCombo();
        }
        if (changed)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    else if (lastOpen == id.c_str())
    {
        lastOpen = "";
        //return last value if the current color was hovered only
        *val->access() = lastColor;
    }
    ImGui::PopStyleVar();
    
    return changed;
}

inline bool InputBindable(bindable<std::string>* val, UIContext& ctx)
{
    ImGui::PushFont(ctx.defaultStyleFont);
    std::string id = "##" + std::to_string((uint64_t)val);
    bool changed = ImGui::InputText(id.c_str(), val->access(), ImGuiInputTextFlags_CallbackCharFilter, InputTextCharExprFilter);
    ImGui::PopFont();
    return changed;
}

enum {
    InputBindable_StretchButton = 0x1,
    InputBindable_StretchButtonDisabled = InputBindable_StretchButton | 0x2,
};
inline bool InputBindable(bindable<dimension>* val, dimension defVal, int flags, UIContext& ctx)
{
    if (flags & InputBindable_StretchButton) {
        const auto& nextItemData = ImGui::GetCurrentContext()->NextItemData;
        bool hasWidth = nextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth;
        ImGui::SetNextItemWidth((hasWidth ? nextItemData.Width : 0) - ImGui::GetFrameHeight());
    }
    
    bool stretch = val->stretched();
    std::string id = "##" + std::to_string((uint64_t)val);
    bool changed = ImGui::InputText(id.c_str(), val->access(), ImGuiInputTextFlags_CallbackCharFilter, InputTextCharExprFilter);

    //disallow empty state except string values
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        if (!stretch && val->empty())
            *val = defVal;
        else if (stretch && !val->has_value())
            *val = 1.0f;
    }

    if (flags & InputBindable_StretchButton) {
        ImGui::SameLine(0, 0);
        ImGui::PushFont(ImGui::GetDefaultFont());
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[stretch ? ImGuiCol_ButtonActive : ImGuiCol_Button]);
        ImGui::BeginDisabled(flags & InputBindable_StretchButtonDisabled);
        if (ImGui::Button((ICON_FA_LEFT_RIGHT + id).c_str(), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
        {
            changed = true;
            stretch = !stretch;
            *val = stretch ? 1.0f : (float)(int)defVal;
            val->stretch(stretch);
        };
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    return changed;
}

template <class T, 
    class = std::enable_if_t<!std::is_same_v<T, dimension>> >
inline bool InputBindable(bindable<T>* val, T defVal, UIContext& ctx)
{
    std::string id = "##" + std::to_string((uint64_t)val);
    bool changed = ImGui::InputText(id.c_str(), val->access(), ImGuiInputTextFlags_CallbackCharFilter, InputTextCharExprFilter);

    //disallow empty state except string values
    if (ImGui::IsItemDeactivatedAfterEdit() &&
        val->access()->empty())
    {
        *val = defVal;
    }

    return changed;
}

template <class T>
inline bool InputFieldRef(field_ref<T>* val, const std::string& type, bool allowEmpty, UIContext& ctx)
{
    if (val == ctx.setProp)
    {
        //doing it here imrad.cpp will be able to set property in all selected widgets
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    if (ImGui::BeginCombo(id.c_str(), val->c_str()))
    {
        if (allowEmpty && ImGui::Selectable("None"))
        {
            *val->access() = "";
            changed = true;
        }
        
        if (!val->empty() && ImGui::Selectable("Rename..."))
        {
            auto vars = val->used_variables();
            assert(vars.size() == 1);
            newFieldPopup.codeGen = ctx.codeGen;
            newFieldPopup.varOldName = vars[0];
            newFieldPopup.mode = NewFieldPopup::RenameField;
            newFieldPopup.OpenPopup([val, root=ctx.root] {
                root->RenameFieldVars(newFieldPopup.varOldName, newFieldPopup.varName);
                });
        }
        
        if (ImGui::Selectable("New Variable..."))
        {
            newFieldPopup.varType = type;
            newFieldPopup.codeGen = ctx.codeGen;
            newFieldPopup.mode = NewFieldPopup::NewField;
            newFieldPopup.OpenPopup([&ctx, val] {
                ctx.setProp = val;
                ctx.setPropValue = newFieldPopup.varName;
                });
        }
        
        ImGui::Separator();
        const auto& vars = ctx.codeGen->GetVars();
        for (const auto& v : vars)
        {
            if (v.type == type && ImGui::Selectable(v.name.c_str(), v.name == val->c_str()))
            {
                *val->access() = v.name;
                changed = true;
            }
        }
    
        ImGui::EndCombo();
    }
    return changed;
}

template <class T>
inline bool InputFieldRef(field_ref<T>* val, bool allowEmpty, UIContext& ctx)
{
    std::string type = typeid_name<T>();
    return InputFieldRef(val, type, allowEmpty, ctx);
}

template <>
inline bool InputFieldRef(field_ref<void>* val, bool allowEmpty, UIContext& ctx) = delete;


inline bool InputDataSize(bindable<int>* val, bool allowEmpty, UIContext& ctx)
{
    if (val == ctx.setProp)
    {
        //doing it here imrad.cpp will be able to set property in all selected widgets
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    if (ImGui::BeginCombo(id.c_str(), val->c_str()))
    {
        if (allowEmpty && ImGui::Selectable("None"))
        {
            *val->access() = "";
            changed = true;
        }

        if (ImGui::Selectable("New Variable..."))
        {
            newFieldPopup.varType = ""; //allow to create std::vector etc.
            newFieldPopup.codeGen = ctx.codeGen;
            newFieldPopup.mode = NewFieldPopup::NewField;
            newFieldPopup.OpenPopup([&ctx, val] {
                ctx.setProp = val;
                if (cpp::is_std_container(newFieldPopup.varType))
                    ctx.setPropValue = newFieldPopup.varName + ".size()";
                else
                    ctx.setPropValue = newFieldPopup.varName;
                });
        }

        ImGui::Separator();
        const auto& vars = ctx.codeGen->GetVarExprs("int");
        for (const auto& v : vars)
        {
            if (ImGui::Selectable(v.first.c_str(), v.first == val->c_str()))
            {
                *val->access() = v.first;
                changed = true;
            }
        }

        ImGui::EndCombo();
    }
    return changed;
}

enum {
    InputEvent_GlobalFunction = 0x1,
};
template <class FuncSig>
inline bool InputEvent(event<FuncSig>* val, int flags, UIContext& ctx)
{
    if (val == ctx.setProp) 
    {
        //doing it here imrad.cpp will be able to set property in all selected widgets
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);

    if (flags & InputEvent_GlobalFunction)
    {
        const auto& nextItemData = ImGui::GetCurrentContext()->NextItemData;
        bool hasWidth = nextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth;
        ImGui::SetNextItemWidth((hasWidth ? nextItemData.Width : 0) - ImGui::GetFrameHeight());
        changed = ImGui::InputText(id.c_str(), val->access(), ImGuiInputTextFlags_CharsNoBlank);
        ImGui::SameLine(0, 0);
        ImGui::BeginDisabled();
        //if (ImGui::BeginCombo((label + std::string("BUT")).c_str(), "", ImGuiComboFlags_NoPreview))
        //    ImGui::EndCombo();
        ImGui::Button((" " + id).c_str(), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });
        ImGui::EndDisabled();
    }
    else
    {
        std::string type = typeid_name<FuncSig>();
        if (ImGui::BeginCombo(id.c_str(), val->c_str(), ImGuiComboFlags_HeightLarge))
        {
            if (ImGui::Selectable("None"))
            {
                changed = true;
                *val->access() = "";
            }
            if (ImGui::Selectable("New Method..."))
            {
                changed = true;
                newFieldPopup.varType = type;
                newFieldPopup.codeGen = ctx.codeGen;
                newFieldPopup.mode = NewFieldPopup::NewEvent;
                newFieldPopup.OpenPopup([&ctx, val] {
                    ctx.setProp = val;
                    ctx.setPropValue = newFieldPopup.varName;
                    });
            }

            ImGui::Separator();
            std::vector<std::string> events;
            for (const auto& v : ctx.codeGen->GetVars()) {
                if (v.type == type)
                    events.push_back(v.name);
            }
            stx::sort(events);
            for (const auto& ev : events)
            {
                if (ImGui::Selectable(ev.c_str(), ev == val->c_str()))
                {
                    changed = true;
                    *val->access() = ev;
                }
            }
            ImGui::EndCombo();
        }
    }
    return changed;
}

inline int CheckBoxFlags(flags_helper* flags)
{
    int changed = false;
    for (const auto& id : flags->get_ids())
    {
        if (id.first == "")
            ImGui::Separator();
        else if (ImGui::CheckboxFlags(id.first.c_str(), flags->access(), id.second))
            changed = id.second;
    }
    return changed;
}
