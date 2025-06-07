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
#include "ui_combo_dlg.h"

inline const char* PARENT_STR = "inherit";
inline const uint32_t NONE_COLOR = 0xff800000;
inline const uint32_t NEW_COLOR = 0xff006000;
inline const uint32_t RENAME_COLOR = 0xff004080;

inline bool IsHighlighted(const std::string& id)
{
    ImGuiID iid = ImGui::GetID(id.c_str());
    if (ImGui::GetHoveredID() == iid || ImGui::GetActiveID() == iid || ImGui::GetFocusID() == iid)
        return true;
    ImGuiID popupId = ImGui::GetIDWithSeed("##ComboPopup", 0, iid);
    if (ImGui::IsPopupOpen(popupId, 0))
        return true;
    return false;
}

enum {
    BindingButton_ReferenceOnly = 0x1
};

template <class T>
inline bool BindingButton(const char* label, bindable<T>* val, const std::string& type, int flags, UIContext& ctx)
{
    if (val == ctx.setProp)
    {
        //commit dialog request
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool bound;
    if constexpr (std::is_same_v<T, std::string> ||
        std::is_same_v<T, std::vector<std::string>> ||
        std::is_same_v<T, color_t>)
    {
        bound = !val->used_variables().empty();
    }
    else if constexpr (std::is_same_v<T, bool>)
        bound = !val->empty(); //the default value is usually empty
    else
        bound = !val->empty() && !val->has_value();

    ImGui::PushStyleColor(ImGuiCol_Button, bound ? 0xff0080ff : 0xffc0c0c0);
    float sp = (ImGui::GetFrameHeight() - 12) / 2;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sp);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + sp);
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    bool pushed = ImGui::Button(("##" + std::string(label)).c_str(), { 12, 12 });
    ImGui::PopItemFlag();
    ImGui::PopStyleColor();
    if (pushed)
    {
        bindingDlg.font = type.find("std::string") != std::string::npos ? ctx.defaultStyleFont : nullptr;
        bindingDlg.codeGen = ctx.codeGen;
        bindingDlg.curArray = ctx.GetCurrentArray();
        bindingDlg.name = label;
        bindingDlg.expr = val->c_str();
        bindingDlg.type = type;
        bindingDlg.forceReference = flags & BindingButton_ReferenceOnly;
        bindingDlg.OpenPopup([&ctx, val](ImRad::ModalResult) {
            ctx.setProp = val;
            ctx.setPropValue = bindingDlg.expr;
            });
        return true;
    }

    return false;
}

template <class T, class = std::enable_if_t<!std::is_same_v<T, void>>>
inline bool BindingButton(const char* label, bindable<T>* val, int flags, UIContext& ctx)
{
    return BindingButton(label, val, typeid_name<T>(), flags, ctx);
}

template <class T, class = std::enable_if_t<!std::is_same_v<T, void>>>
inline bool BindingButton(const char* label, bindable<T>* val, UIContext& ctx)
{
    return BindingButton(label, val, typeid_name<T>(), 0, ctx);
}

//---------------------------------------------------------------------------------

enum
{
    InputDirectVal_Modified = 0x1,
    InputDirectVal_ShortcutButton = 0x2,
};

template <class T>
bool InputDirectVal(direct_val<T, false>* val, int fl, UIContext& ctx)
{
    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && (fl & InputDirectVal_Modified) ?
        ctx.pgbFont : ctx.pgFont);
    std::string id = "##" + std::to_string((uint64_t)val);
    bool changed = false;
    if constexpr (std::is_same_v<T, int>) {
        bool high = IsHighlighted(id);
        ImGui::PushID(id.c_str());
        high |= IsHighlighted("+") || IsHighlighted("-");
        ImGui::PopID();
        if (ImRad::IsCurrentItemDisabled())
            high = false;
        changed = ImGui::InputInt(id.c_str(), val->access(), high ? 1 : 0);
    }
    else if constexpr (std::is_same_v<T, float>)
        changed = ImGui::InputFloat(id.c_str(), val->access(), 0, 0, "%.f");
    else
        assert(false);

    ImGui::PopFont();
    return changed;
}

inline bool InputDirectVal(direct_val<dimension_t>* val, int flags, UIContext& ctx)
{
    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && (flags & InputDirectVal_Modified) ?
        ctx.pgbFont : ctx.pgFont);
    std::string id = "##" + std::to_string((uint64_t)val);
    bool changed = ImGui::InputFloat(id.c_str(), val->access(), 0, 0, "%.0f");
    ImGui::PopFont();
    return changed;
}

inline bool InputDirectVal(direct_val<pzdimension_t>* val, UIContext& ctx)
{
    ImGui::PushFont(val->has_value() ? ctx.pgbFont : ctx.pgFont);
    std::string id = "##" + std::to_string((uint64_t)val);
    std::string tmp = val->has_value() ? std::to_string((int)*val->access()) : "";
    std::string hint = ImGui::GetActiveID() == ImGui::GetCurrentWindow()->GetID(id.c_str()) ? "" : PARENT_STR;
    bool changed = ImGui::InputTextWithHint(id.c_str(), hint.c_str(), &tmp, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
    if (changed)
    {
        std::istringstream is(tmp);
        is >> *val->access();
        if (!is && ImGui::IsItemDeactivatedAfterEdit())
            val->clear();
    }
    ImGui::PopFont();
    return changed;
}

inline bool InputDirectVal(direct_val<pzdimension2_t>* val, UIContext& ctx)
{
    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && val->has_value() ?
        ctx.pgbFont : ctx.pgFont);
    ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
    for (int i = 0; i < 2; ++i)
    {
        ImGui::PushID(i);
        if (i)
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        //bool ch = ImGui::InputFloat2(id.c_str(), (float*)val->access(), "%.0f");
        std::string tmp = val->has_value() ? std::to_string((int)(*val->access())[i]) : "";
        std::string hint = i || ImGui::GetActiveID() == ImGui::GetCurrentWindow()->GetID(id.c_str()) ? "" : PARENT_STR;
        if (ImGui::InputTextWithHint(id.c_str(), hint.c_str(), &tmp, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll))
        {
            changed = true;
            std::istringstream is(tmp);
            is >> (*val->access())[i];
            if (!is && ImGui::IsItemDeactivatedAfterEdit())
                val->clear();
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
    }
    ImGui::PopFont();
    return changed;
}

inline bool InputDirectVal(direct_val<bool>* val, int fl, UIContext& ctx)
{
    std::string id = "##" + std::to_string((uint64_t)val);
    bool val1 = *val;

    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImGui::GetStyleColorVec4(
        (fl & InputDirectVal_Modified) ? ImGuiCol_Text : ImGuiCol_TextDisabled));

    if ((fl & InputDirectVal_Modified) && !val1) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyleColorVec4(ImGuiCol_Text));
    }

    bool changed = ImGui::Checkbox(id.c_str(), val->access());

    if ((fl & InputDirectVal_Modified) && !val1) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor();

    return changed;
}

inline bool InputDirectVal(direct_val<std::string>* val, int fl, UIContext& ctx)
{
    ImGui::PushFont(
        !IsAscii(*val->access()) ? ctx.defaultStyleFont :
        !ImRad::IsCurrentItemDisabled() && (fl & InputDirectVal_Modified) ? ctx.pgbFont :
        ctx.pgFont
    );
    std::string id = "##" + std::to_string((uint64_t)val);
    bool ch = ImGui::InputText(id.c_str(), val->access(), ImGuiInputTextFlags_CallbackCharFilter, DefaultCharFilter);
    ImGui::PopFont();
    return ch;
}

inline bool InputDirectVal(direct_val<shortcut_t>* val, int flags, UIContext& ctx)
{
    if (flags & InputDirectVal_ShortcutButton)
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - ImGui::GetFrameHeight());

    std::string id = "##" + std::to_string((uint64_t)val);
    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() ? ctx.pgbFont : ctx.pgFont);
    bool changed = ImGui::InputText(id.c_str(), val->access(), ImGuiInputTextFlags_CallbackCharFilter, DefaultCharFilter);
    ImGui::PopFont();

    if (flags & InputDirectVal_ShortcutButton)
    {
        std::string buttonId = id + "But";
        std::string popupId = id + "DropDown";

        ImGui::SameLine(0, 0);
        if (IsHighlighted(id) || IsHighlighted(buttonId) ||
            ImGui::IsPopupOpen(ImGui::GetID(popupId.c_str()), 0))
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[val->flags() ? ImGuiCol_ButtonActive : ImGuiCol_Button]);
            if (ImGui::ArrowButton(buttonId.c_str(), ImGuiDir_Down)) // { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
            {
                ImGui::OpenPopup(popupId.c_str());
            }
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::InvisibleButton(buttonId.c_str(), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8, 8 });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 8 });
        if (ImGui::BeginPopup(popupId.c_str()))
        {
            if (ImGui::MenuItem("Global", nullptr, val->flags() & ImGuiInputFlags_RouteGlobal)) {
                changed = true;
                val->set_flags(val->flags() ^ ImGuiInputFlags_RouteGlobal);
            }
            if (ImGui::MenuItem("Repeat", nullptr, val->flags() & ImGuiInputFlags_Repeat)) {
                changed = true;
                val->set_flags(val->flags() ^ ImGuiInputFlags_Repeat);
           }
           ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
    }

    return changed;
}


inline bool InputDirectValContextMenu(direct_val<std::string>* val, UIContext& ctx)
{
    bool changed = false;
    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() ? ctx.pgbFont : ctx.pgFont);
    std::string id = "##" + std::to_string((uint64_t)val);
    if (ImGui::BeginCombo(id.c_str(), val->c_str(),
        IsHighlighted(id) ? 0 : ImGuiComboFlags_NoArrowButton))
    {
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetFont());

        if (ImGui::Selectable(" ", val->empty()))
        {
            changed = true;
            *val->access() = "";
        }
        for (const std::string& cm : ctx.contextMenus)
        {
            if (ImGui::Selectable(cm.c_str(), cm == val->c_str()))
            {
                changed = true;
                *val->access() = cm;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopFont();
    return changed;
}

template <class T>
inline bool InputDirectValEnum(direct_val<T, true>* val, int fl, UIContext& ctx)
{
    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    std::string value = val->get_id();
    size_t pre = 0;
    if (!value.compare(0, 5, "ImGui")) {
        pre = value.find("_");
        if (pre != std::string::npos && pre + 1 < value.size())
            ++pre;
        else
            pre = 0;
    }
    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && (fl & InputDirectVal_Modified) ?
        ctx.pgbFont : ctx.pgFont);
    if (ImGui::BeginCombo(id.c_str(), val->get_id().c_str() + pre,
        !ImRad::IsCurrentItemDisabled() && IsHighlighted(id) ? 0 : ImGuiComboFlags_NoArrowButton))
    {
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetFont());
        changed = true;
        for (const auto& item : val->get_ids())
        {
            if (!item.first.compare(0, 5, "ImGui")) {
                pre = item.first.find("_");
                if (pre != std::string::npos && pre + 1 < item.first.size())
                    ++pre;
                else
                    pre = 0;
            }
            if (ImGui::Selectable(item.first.c_str() + pre, *val->access() == item.second))
                *val->access() = item.second;
        }
        ImGui::EndCombo();
    }
    ImGui::PopFont();
    return changed;
}

template <class T>
inline int InputDirectValFlags(const char* name, direct_val<T, true>* val, int defVal, UIContext& ctx)
{
    int changed = false;
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    ImGui::Unindent();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, pad.y });
    //ImGui::PushStyleColor(ImGuiCol_NavCursor, { 0, 0, 0, 0 });
    ImGui::SetNextItemAllowOverlap();
    if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_SpanAllColumns /*| ImGuiTreeNodeFlags_NoNavFocus*/)) {
        ImGui::PopStyleVar();
        ImGui::Indent();
        ImGui::TableNextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { pad.x, 0 }); //row packing
        ImGui::Spacing();
        for (const auto& id : val->get_ids())
        {
            if (id.first == "") {
                ImGui::Separator();
                continue;
            }
            if (defVal != -1) {
                bool different = ((int)*val & id.second) != (defVal & id.second);
                ImGui::PushFont(ImRad::GetFontByName(different ? "imrad.pgb" : "imrad.pg"));
            }
            size_t pre = 0;
            if (!id.first.compare(0, 5, "ImGui")) {
                pre = id.first.find('_');
                if (pre != std::string::npos && pre + 1 < id.first.size())
                    ++pre;
                else
                    pre = 0;
            }
            ImU64 flags = *val;
            if (ImGui::CheckboxFlags(id.first.c_str() + pre, &flags, id.second)) {
                changed = id.second;
                *val->access() = (int)flags;
            }
            if (defVal != -1)
                ImGui::PopFont();
        }
        ImGui::PopStyleVar();
        ImGui::TreePop();
        ImGui::Unindent();
    }
    else
    {
        ImGui::PopStyleVar();
        ImGui::TableNextColumn();
        std::string label, tip;
        for (const auto& id : val->get_ids())
        {
            if (id.first != "" && (*val & id.second) == id.second)
            {
                size_t pre = 0;
                if (!id.first.compare(0, 5, "ImGui")) {
                    size_t i = id.first.find('_');
                    if (i != std::string::npos && i + 1 < id.first.size())
                        pre = i + 1;
                }
                tip += id.first.substr(pre) + "\n";
            }
        }
        if (tip == "")
            tip = "None";
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && *val != defVal ?
            ctx.pgbFont : ctx.pgFont);
        //float w = ImGui::GetContentRegionAvail().x;
        ImGui::Text("...");
        ImGui::PopFont();
        if (//ImGui::GetItemRectSize().x > w &&
            ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8, 8 });
            ImGui::SetTooltip("%s", tip.c_str());
            ImGui::PopStyleVar();
        }
    }
    ImGui::Indent();
    //ImGui::PopStyleColor();
    return changed;
}

//-------------------------------------------------------------------------------

enum
{
    InputBindable_StretchButton = 0x1,
    InputBindable_StretchButtonDisabled = InputBindable_StretchButton | 0x2,
    InputBindable_MultilineEdit = 0x4,
    InputBindable_CustomButton = 0x8,
    InputBindable_Modified = 0x10,
    InputBindable_ShowVariables = 0x20,
    InputBindable_ShowNone = 0x40,
    InputBindable_ParentStr = 0x80
};

template <class T,
    class = std::enable_if_t<!std::is_same_v<T, dimension_t>> >
inline bool InputBindable(bindable<T>* val, const std::string& type, int flags, UIContext& ctx)
{
    if (val == ctx.setProp)
    {
        //commit dialog request
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);

    if (!(flags & InputBindable_ShowVariables))
    {
        int fl = ImGuiInputTextFlags_CallbackCharFilter;
        if constexpr (!std::is_same_v<T, std::string>) {
            if (val->has_value())
                fl |= ImGuiInputTextFlags_AutoSelectAll;
        }
        ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && (flags & InputBindable_Modified) ?
            ctx.pgbFont : ctx.pgFont);
        changed = ImGui::InputText(id.c_str(), val->access(), fl, DefaultCharFilter);
        ImGui::PopFont();

        /*if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) &&
            ImGui::CalcTextSize(val->c_str()).x > ImGui::GetItemRectSize().x)
        {
            ImGui::SetItemTooltip(val->c_str());
        }*/
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            //disallow empty state
            std::string v = Trim(val->c_str());
            if (v.empty())
                *val = {};
            else
                *val->access() = v;
        }
    }
    else
    {
        ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && (flags & InputBindable_Modified) ?
            ctx.pgbFont : ctx.pgFont);
        if (ImGui::BeginCombo(id.c_str(), val->c_str(),
            !ImRad::IsCurrentItemDisabled() && IsHighlighted(id) ? 0 : ImGuiComboFlags_NoArrowButton))
        {
            ImGui::PopFont();
            ImGui::PushFont(ImGui::GetFont());

            ImGui::PushStyleColor(ImGuiCol_Text, NONE_COLOR);
            if ((flags & InputBindable_ShowNone) && ImGui::Selectable("None"))
            {
                *val->access() = "";
                changed = true;
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, RENAME_COLOR);
            if (val->has_single_variable() && ImGui::Selectable("Rename..."))
            {
                auto vars = val->used_variables();
                newFieldPopup.codeGen = ctx.codeGen;
                newFieldPopup.varOldName = vars[0];
                newFieldPopup.mode = NewFieldPopup::RenameField;
                newFieldPopup.OpenPopup([val, root = ctx.root]{
                    root->RenameFieldVars(newFieldPopup.varOldName, newFieldPopup.varName);
                    });
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, NEW_COLOR);
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
            ImGui::PopStyleColor();

            ImGui::Separator();
            auto vars = ctx.codeGen->GetVarExprs(type, true, ctx.GetCurrentArray());
            /*auto* dc = ImGui::GetCurrentWindow()->DrawList;
            float sp = ImGui::CalcTextSize(" ").x;*/ 
            for (const auto& v : vars)
            {
                if (ImGui::Selectable(v.first.c_str(), v.first == val->c_str()))
                {
                    *val->access() = v.first;
                    changed = true;
                }
                /*ImVec2 p = ImGui::GetCursorScreenPos();
                p.x += sp;
                p.y -= 0.5f * ImGui::GetTextLineHeight();
                dc->AddCircleFilled(p, 0.2f * ImGui::GetFontSize(), 0xff000000);*/
            }

            ImGui::EndCombo();
        }
        ImGui::PopFont();
    }

    return changed;
}

template <class T,
    class = std::enable_if_t<!std::is_same_v<T, void> && !std::is_same_v<T, dimension_t>> >
    inline bool InputBindable(bindable<T>* val, int flags, UIContext& ctx)
{
    return InputBindable(val, typeid_name<T>(), flags, ctx);
}

inline bool InputBindable(bindable<font_name_t>* val, UIContext& ctx)
{
    std::string fn = val->empty() ? PARENT_STR :
        val->has_value() ? val->value() : val->c_str();
    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    if (ImGui::BeginCombo(id.c_str(), nullptr,
        ImGuiComboFlags_CustomPreview | (IsHighlighted(id) ? 0 : ImGuiComboFlags_NoArrowButton)))
    {
        for (const auto& f : ctx.fontNames)
        {
            if (ImGui::Selectable(f == "" ? " " : f.c_str(), f == fn)) {
                changed = true;
                if (f == "")
                    *val->access() = f;
                else
                    val->set_font_name(f);
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginComboPreview())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(
            val->empty() ? ImGuiCol_TextDisabled : ImGuiCol_Text));
        ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && !val->empty() ?
            ctx.pgbFont : ctx.pgFont);
        ImGui::TextUnformatted(fn.c_str());
        ImGui::PopFont();
        ImGui::PopStyleColor();
        ImGui::EndComboPreview();
    }
    return changed;
}

inline bool InputBindable(bindable<color_t>* val, int defStyleCol, UIContext& ctx)
{
    static std::string lastColor;
    static int lastStyleClr;

    auto nextData = ImGui::GetCurrentContext()->NextItemData; //copy
    int styleClr = val->style_color();
    ImVec4 buttonClr = val->empty() ?
        ctx.style.Colors[defStyleCol] :
        ImGui::ColorConvertU32ToFloat4(val->eval(defStyleCol, ctx));
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
        val->empty() ? PARENT_STR :
        val->c_str();
    clrName += id;
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(val->empty() ? ImGuiCol_TextDisabled : ImGuiCol_Text));
    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && !val->empty() ?
        ctx.pgbFont : ctx.pgFont);
    if (ImGui::Selectable(clrName.c_str(), false, 0, sz))
    {
        lastColor = val->c_str();
        lastStyleClr = styleClr;
        ImGui::OpenPopup(id.c_str());
    }
    ImGui::PopFont();
    ImGui::PopStyleColor();

    static ImU32 COLORS[]{
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
        IM_COL32(255,255,255,255), //IM_COL32(0,0,0,255),
        IM_COL32(152,51,0,255),
        IM_COL32(51,51,0,255),
        IM_COL32(0,51,0,255),
        IM_COL32(0,51,102,255),
        IM_COL32(0,0,128,255),
        IM_COL32(51,51,153,255),
        IM_COL32(0,0,0,255),

        IM_COL32(128,0,0,255),
        IM_COL32(255,102,0,255),
        IM_COL32(128,128,0,255),
        IM_COL32(0,128,0,255),
        IM_COL32(0,128,128,255),
        IM_COL32(0,0,255,255),
        IM_COL32(102,102,153,255),
        IM_COL32(64,64,64,255),

        IM_COL32(255,0,0,255),
        IM_COL32(255,153,0,255),
        IM_COL32(153,204,0,255),
        IM_COL32(51,153,102,255),
        IM_COL32(51,204,204,255),
        IM_COL32(51,102,255,255),
        IM_COL32(128,0,128,255),
        IM_COL32(128,128,128,255),

        IM_COL32(254,0,255,255),
        IM_COL32(255,203,0,255),
        IM_COL32(255,255,0,255),
        IM_COL32(0,255,0,255),
        IM_COL32(0,255,254,255),
        IM_COL32(0,204,255,255),
        IM_COL32(153,51,102,255),
        IM_COL32(192,192,192,255),

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
    if (ImGui::BeginPopup(id.c_str(), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav))
    {
        lastOpen = id.c_str();
        *val->access() = lastColor;
        bool autoSel = ImGui::ColorButton("tooltip", ctx.style.Colors[defStyleCol], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview);
        bool autoHover = ImGui::IsItemHovered();
        ImGui::SameLine(0, 0);
        ImGui::Selectable("  Automatic", false, ImGuiSelectableFlags_NoPadWithHalfSpacing);
        autoHover = autoHover || ImGui::IsItemHovered();
        autoSel = autoSel || ImGui::IsItemClicked(ImGuiMouseButton_Left);
        if (autoSel)
        {
            changed = true;
            lastColor = "";
            *val = {};
        }
        if (autoHover)
        {
            *val = {};
        }
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4, 4 });
        for (int i = 0; i < stx::ssize(COLORS); ++i)
        {
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, COLORS[i]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COLORS[i]);
            //if (ImGui::ColorButton("##clr", ImGui::ColorConvertU32ToFloat4(COLORS[i]), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview, { 30, 30 }))
            if (ImGui::Button("##clr", { 30, 30 }))
            {
                changed = true;
                *val = i ? COLORS[i] : 0x00ffffff;
                lastColor = *val->access();
            }
            if (ImGui::IsItemHovered())
            {
                *val = i ? COLORS[i] : 0x00ffffff;
            }
            if (!i)
            {
                ImVec2 p1 = ImGui::GetItemRectMin();
                ImVec2 p2 = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddLine({ p1.x, p2.y }, { p2.x, p1.y }, 0xff0000ff);
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

inline bool InputBindable(bindable<std::string>* val, int flags, UIContext& ctx, std::function<void()> fun = [] {})
{
    std::string id = "##" + std::to_string((uint64_t)val);
    std::string butId = "..." + id;
    bool high = IsHighlighted(id) || IsHighlighted(butId);
    bool hasButton = flags & (InputBindable_MultilineEdit | InputBindable_CustomButton);
    float w = ImGui::CalcItemWidth();
    if (hasButton && high)
        w -= ImGui::GetFrameHeight();

    ImGui::SetNextItemWidth(w);
    ImGui::PushFont(
        !IsAscii(*val->access()) ? ctx.defaultStyleFont :
        !ImRad::IsCurrentItemDisabled() ? ctx.pgbFont :
        ctx.pgFont
    );
    bool changed = ImGui::InputText(id.c_str(), val->access(), ImGuiInputTextFlags_CallbackCharFilter, DefaultCharFilter);
    ImGui::PopFont();
    
    if (hasButton && high)
    {
        ImGui::SameLine(0, 0);
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
        bool isDown = false;
        if (flags & InputBindable_MultilineEdit)
            isDown = val->access()->find('\n') != std::string::npos;
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(
            isDown ? ImGuiCol_ButtonActive : ImGuiCol_Button
            ));
        if (ImGui::Button(butId.c_str(), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
        {
            if (flags & InputBindable_CustomButton)
            {
                fun();
            }
            else if (flags & InputBindable_MultilineEdit)
            {
                comboDlg.title = "Multiline text";
                comboDlg.value = *val->access();
                comboDlg.font = ctx.defaultStyleFont;
                comboDlg.OpenPopup([&ctx, val](ImRad::ModalResult mr) {
                    ctx.setProp = val;
                    ctx.setPropValue = comboDlg.value;
                    });
            }
        }
        ImGui::PopStyleColor();
        ImGui::PopItemFlag();
    }

    return changed;
}

inline bool InputBindable(bindable<std::vector<std::string>>* val, UIContext& ctx)
{
    if (val == ctx.setProp) 
    {
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    std::string label;
    int nl = (int)stx::count(*val->access(), '\0');
    if (!val->has_single_variable())
        label = "[" + std::to_string(nl) + "]";
    else
        label = val->used_variables()[0];
    label += id;

    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && !val->empty() ?
        ctx.pgbFont : ctx.pgFont);
    /*float w = ImGui::CalcItemWidth();
    ImGui::SetNextItemWidth(w - ImGui::GetFrameHeight());

    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImRad::Selectable(label.c_str(), false, 0, { -2 * ImGui::GetFrameHeight(), 0 });
    ImGui::PopItemFlag();
    ImGui::PopFont();

    bool high = IsHighlighted(label) || IsHighlighted(id);
    ImGui::SameLine(0, 0);
    ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
    ImGui::SetNextWindowSizeConstraints({ w, 0 }, { FLT_MAX, FLT_MAX });
    if (high && ImGui::BeginCombo(id.c_str(), nullptr, ImGuiComboFlags_NoPreview))*/
    if (ImGui::BeginCombo(id.c_str(), label.c_str(),
            IsHighlighted(id) ? 0 : ImGuiComboFlags_NoArrowButton))
    {
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetFont());

        ImGui::PushStyleColor(ImGuiCol_Text, RENAME_COLOR);
        if (ImGui::Selectable("Edit Items..."))
        {
            std::string tmp = *val->access(); //preserve embeded nulls
            stx::replace(tmp, '\0', '\n');
            if (tmp.size() && tmp.back() == '\n')
                tmp.pop_back();
            comboDlg.title = "Items";
            comboDlg.value = tmp;
            comboDlg.font = ctx.defaultStyleFont;
            comboDlg.OpenPopup([val, &ctx](ImRad::ModalResult) {
                std::string tmp = comboDlg.value;
                while (tmp.size() && tmp.back() == '\n')
                    tmp.pop_back();
                *val->access() = tmp;
                if (!val->has_single_variable()) {
                    if (tmp.size() && tmp.back() != '\n')
                        tmp.push_back('\n');
                    stx::replace(tmp, '\n', '\0');
                    *val->access() = tmp;
                }
                //force changed in next iteration
                ctx.setProp = val;
                ctx.setPropValue = tmp;
                });
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, NEW_COLOR);
        if (ImGui::Selectable("New Variable..."))
        {
            newFieldPopup.varType = "std::vector<std::string>";
            newFieldPopup.codeGen = ctx.codeGen;
            newFieldPopup.mode = NewFieldPopup::NewField;
            newFieldPopup.OpenPopup([val, &ctx] {
                //force changed in next iteration
                ctx.setProp = val;
                ctx.setPropValue = '{' + newFieldPopup.varName + '}';
                });
        }
        ImGui::PopStyleColor();

        ImGui::Separator();
        const auto& vars = ctx.codeGen->GetVarExprs("std::vector<std::string>", true, ctx.GetCurrentArray());
        for (const auto& v : vars)
        {
            if (ImGui::Selectable(v.first.c_str(), '{' + v.first + '}' == val->c_str()))
            {
                *val->access() = '{' + v.first + '}';
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::PopFont();
    return changed;
}

inline bool InputBindable(bindable<dimension_t>* val, int flags, UIContext& ctx)
{
    if (val == ctx.setProp)
    {
        //commit dialog request
        ctx.setProp = nullptr;
        if (ctx.setPropValue == "")
            *val = 0;
        else
            *val->access() = ctx.setPropValue;
        return true;
    }

    std::string id = "##" + std::to_string((uint64_t)val);
    std::string butId = ICON_FA_LEFT_RIGHT + id;
    if (flags & InputBindable_StretchButton)
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - ImGui::GetFrameHeight());

    bool stretch = val->stretched();
    bool modified = flags & InputBindable_Modified;
    if ((flags & InputBindable_ParentStr) && !val->empty())
        modified = true;
    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() && modified ?
        ctx.pgbFont : ctx.pgFont);
    int fl = ImGuiInputTextFlags_CallbackCharFilter;
    if (val->has_value())
        fl |= ImGuiInputTextFlags_AutoSelectAll;
    std::string hint = "";
    if (flags & InputBindable_ParentStr)
        hint = ImGui::GetActiveID() == ImGui::GetCurrentWindow()->GetID(id.c_str()) ? "" : PARENT_STR;
    bool changed = ImGui::InputTextWithHint(id.c_str(), hint.c_str(), val->access(), fl, DefaultCharFilter);
    /*std::string tmp = *val->access();
    bool active = ImGui::GetFocusID() == ImGui::GetID(id.c_str()) ||
        ImGui::GetActiveID() == ImGui::GetID(id.c_str());
    if (!active)
        tmp = val->to_arg("");
    bool changed = ImGui::InputText(id.c_str(), &tmp, fl, DefaultCharFilter);
    if (changed) {
        if (stretch && tmp.size() && tmp.back() == 'x')
            val->set_from_arg(tmp);
        else
            *val->access() = tmp;
    }*/
    ImGui::PopFont();

    //disallow empty state
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        if (!stretch && val->empty() && !(flags & InputBindable_ParentStr))
            *val = 0;
        else if (stretch && !val->has_value()) {
            *val = 1.0f;
            val->stretch(true);
        }
    }

    if (flags & InputBindable_StretchButton)
    {
        ImGui::SameLine(0, 0);
        if (IsHighlighted(id) || IsHighlighted(butId))
        {
            ImGui::PushFont(ImGui::GetDefaultFont());
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[stretch ? ImGuiCol_ButtonActive : ImGuiCol_Button]);
            ImGui::BeginDisabled((flags & InputBindable_StretchButtonDisabled) == InputBindable_StretchButtonDisabled);
            if (ImGui::Button(butId.c_str(), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
            {
                changed = true;
                stretch = !stretch;
                *val = stretch ? 1.0f : 0.f;
                val->stretch(stretch);
            };
            ImGui::EndDisabled();
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }
        else
        {
            ImGui::InvisibleButton(butId.c_str(), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });
        }
    }

    return changed;
}

template <class T>
inline bool InputFieldRef(field_ref<T>* val, const std::string& type, bool allowEmpty, UIContext& ctx)
{
    if (val == ctx.setProp)
    {
        //commit dialog request
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    ImGui::PushFont(ctx.pgbFont);
    if (ImGui::BeginCombo(id.c_str(), val->c_str(),
        !ImRad::IsCurrentItemDisabled() && IsHighlighted(id) ? 0 : ImGuiComboFlags_NoArrowButton))
    {
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetFont());

        ImGui::PushStyleColor(ImGuiCol_Text, NONE_COLOR);
        if (allowEmpty && ImGui::Selectable("None"))
        {
            *val->access() = "";
            changed = true;
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, RENAME_COLOR);
        if (val->has_single_variable() && ImGui::Selectable("Rename..."))
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
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, NEW_COLOR);
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
        ImGui::PopStyleColor();

        ImGui::Separator();
        auto vars = ctx.codeGen->GetVarExprs(type, true, ctx.GetCurrentArray());
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
    ImGui::PopFont();
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
        //commit dialog request
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool changed = false;
    std::string id = "##" + std::to_string((uint64_t)val);
    ImGui::PushFont(ctx.pgbFont);

    if (ImGui::BeginCombo(id.c_str(), val->c_str(),
            IsHighlighted(id) ? 0 : ImGuiComboFlags_NoArrowButton))
    {
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetFont());

        ImGui::PushStyleColor(ImGuiCol_Text, NONE_COLOR);
        if (allowEmpty && ImGui::Selectable("None"))
        {
            *val->access() = "";
            changed = true;
        }
        ImGui::PopStyleColor();

        /* it's confusing to offer new variable here. User can still do it from BindingButton
        ImGui::PushStyleColor(ImGuiCol_Text, NEW_COLOR);
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
        ImGui::PopStyleColor();
        */

        ImGui::Separator();
        const auto& vars = ctx.codeGen->GetVarExprs("int", false);
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
    ImGui::PopFont();
    return changed;
}

template <class FuncSig>
inline bool InputEvent(const std::string& name, event<FuncSig>* val, int flags, UIContext& ctx)
{
    if (val == ctx.setProp)
    {
        //commit dialog request
        ctx.setProp = nullptr;
        *val->access() = ctx.setPropValue;
        return true;
    }

    bool changed = false;
    std::string type = typeid_name<FuncSig>();
    std::string id = "##" + std::to_string((uint64_t)val);
    std::string inputId = id + "Input";
    bool buttonVisible = !ImRad::IsCurrentItemDisabled() && (IsHighlighted(inputId) || IsHighlighted(id));
    float width = ImGui::CalcItemWidth();
    if (buttonVisible)
        width -= ImGui::GetFrameHeight();
    float realWidth = ImGui::CalcItemSize({ width, 0 }, 0, 0).x;
    ImGui::SetNextItemWidth(width);
    ImGui::PushFont(!ImRad::IsCurrentItemDisabled() ? ctx.pgbFont : ctx.pgFont);
    ImGui::InputText(inputId.c_str(), val->access(), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopFont();
    if (ImRad::IsItemDoubleClicked() && val->empty())
    {
        newFieldPopup.varType = type;
        newFieldPopup.varName = name;
        newFieldPopup.codeGen = ctx.codeGen;
        newFieldPopup.mode = NewFieldPopup::NewEvent;
        newFieldPopup.OpenPopup([&ctx, val] {
            ctx.setProp = val;
            ctx.setPropValue = newFieldPopup.varName;
            });
    }

    ImGui::SameLine(0, 0);
    if (buttonVisible)
    {
        ImGui::SetNextWindowSizeConstraints({ realWidth + ImGui::GetFrameHeight(), 0 }, { FLT_MAX, FLT_MAX });
        if (ImGui::BeginCombo(id.c_str(), val->c_str(), ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoPreview))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, NONE_COLOR);
            if (ImGui::Selectable("None"))
            {
                changed = true;
                *val->access() = "";
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, NEW_COLOR);
            if (ImGui::Selectable("New Method..."))
            {
                newFieldPopup.varType = type;
                newFieldPopup.varName = name;
                newFieldPopup.codeGen = ctx.codeGen;
                newFieldPopup.mode = NewFieldPopup::NewEvent;
                newFieldPopup.OpenPopup([&ctx, val] {
                    ctx.setProp = val;
                    ctx.setPropValue = newFieldPopup.varName;
                    });
            }
            ImGui::PopStyleColor();

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
    else
    {
        ImGui::InvisibleButton(id.c_str(), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });
    }

    return changed;
}
