/*
imrad.h - ImRAD support library for generated sources

This file contains both the interface and implementation.

Define this:
   #define IMRAD_H_IMPLEMENTATION
before you include this file in *one* C++ file to create implementation.
*/

#ifndef IMRAD_H_INTERFACE
#define IMRAD_H_INTERFACE

#include <string>
#include <vector>
#include <functional> //for ModalPopup callback
#include <map>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h> //for ImGui::Input(string)

#ifdef _MSVC_LANG
// in MSVC __cplusplus is not reilable unless /Zc:__cplusplus is given
#define CPLUSPLUS _MSVC_LANG
#else
#define CPLUSPLUS __cplusplus
#endif

#ifdef IMRAD_WITH_FMT
#include <fmt/format.h>
#elif CPLUSPLUS >= 202002L && __has_include(<format>)
#include <format>
#endif

#define IMRAD_INPUTTEXT_EVENT(clazz, member) \
    [](ImGuiInputTextCallbackData* data) { return ((clazz*)data->UserData)->member(*data); }, this


#ifdef ANDROID

// Implement asset loading externally likely by using AAssetManager
extern std::pair<void*, int> GetAndroidAsset(const char* filename);

#endif


namespace ImRad
{

using Int2 = int[2];
using Int3 = int[3];
using Int4 = int[4];
using Float2 = float[2];
using Float3 = float[3];
using Float4 = float[4];
using Color3 = ImVec4;
using Color4 = ImVec4;

enum ModalResult {
    None = 0,
    Ok = 0x1,
    Cancel = 0x2,
    Yes = 0x4,
    No = 0x8,
    Abort = 0x10,
    Retry = 0x20,
    Ignore = 0x40,
    All = 0x80, //deprecated
    YesToAll = 0x80,
    NoToAll = 0x100,
    Apply = 0x200,
    Discard = 0x400,
    Help = 0x800,
    Reset = 0x1000,
};

enum Alignment {
    AlignNone = 0x0,
    AlignLeft = 0x1,
    AlignRight = 0x2,
    AlignHCenter = 0x4,
    AlignTop = 0x8,
    AlignBottom = 0x10,
    AlignVCenter = 0x20,
};

enum ImeType {
    ImeNone = 0,
    ImeText = 1,
    ImeNumber = 2,
    ImeDecimal = 3,
    ImePhone = 4,
    ImeEmail = 5,

    ImeActionDone = 0x100,
    ImeActionGo = 0x200,
    ImeActionNext = 0x300,
    ImeActionPrevious = 0x400,
    ImeActionSearch = 0x500,
    ImeActionSend = 0x600,
};

// from imgui_internal.h
struct Rect
{
    ImVec2      Min;    // Upper-left
    ImVec2      Max;    // Lower-right

    constexpr Rect() : Min(0.0f, 0.0f), Max(0.0f, 0.0f) {}
    constexpr Rect(const ImVec2& min, const ImVec2& max) : Min(min), Max(max) {}
    constexpr Rect(const ImVec4& v) : Min(v.x, v.y), Max(v.z, v.w) {}
    constexpr Rect(float x1, float y1, float x2, float y2) : Min(x1, y1), Max(x2, y2) {}

    ImVec2      GetCenter() const { return ImVec2((Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f); }
    ImVec2      GetSize() const { return ImVec2(Max.x - Min.x, Max.y - Min.y); }
    float       GetWidth() const { return Max.x - Min.x; }
    float       GetHeight() const { return Max.y - Min.y; }
    float       GetArea() const { return (Max.x - Min.x) * (Max.y - Min.y); }
    ImVec2      GetTL() const { return Min; }                   // Top-left
    ImVec2      GetTR() const { return ImVec2(Max.x, Min.y); }  // Top-right
    ImVec2      GetBL() const { return ImVec2(Min.x, Max.y); }  // Bottom-left
    ImVec2      GetBR() const { return Max; }                   // Bottom-right
    bool        Contains(const ImVec2& p) const { return p.x >= Min.x && p.y >= Min.y && p.x < Max.x && p.y < Max.y; }
    bool        Contains(const Rect& r) const { return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x <= Max.x && r.Max.y <= Max.y; }
    bool        ContainsWithPad(const ImVec2& p, const ImVec2& pad) const { return p.x >= Min.x - pad.x && p.y >= Min.y - pad.y && p.x < Max.x + pad.x && p.y < Max.y + pad.y; }
    bool        Overlaps(const Rect& r) const { return r.Min.y <  Max.y && r.Max.y >  Min.y && r.Min.x <  Max.x && r.Max.x >  Min.x; }
    void        Add(const ImVec2& p) { if (Min.x > p.x)     Min.x = p.x;     if (Min.y > p.y)     Min.y = p.y;     if (Max.x < p.x)     Max.x = p.x;     if (Max.y < p.y)     Max.y = p.y; }
    void        Add(const Rect& r) { if (Min.x > r.Min.x) Min.x = r.Min.x; if (Min.y > r.Min.y) Min.y = r.Min.y; if (Max.x < r.Max.x) Max.x = r.Max.x; if (Max.y < r.Max.y) Max.y = r.Max.y; }
    void        Expand(const float amount) { Min.x -= amount;   Min.y -= amount;   Max.x += amount;   Max.y += amount; }
    void        Expand(const ImVec2& amount) { Min.x -= amount.x; Min.y -= amount.y; Max.x += amount.x; Max.y += amount.y; }
    void        Translate(const ImVec2& d) { Min.x += d.x; Min.y += d.y; Max.x += d.x; Max.y += d.y; }
    void        TranslateX(float dx) { Min.x += dx; Max.x += dx; }
    void        TranslateY(float dy) { Min.y += dy; Max.y += dy; }
    //void        ClipWith(const ImRect& r) { Min = ImMax(Min, r.Min); Max = ImMin(Max, r.Max); }                   // Simple version, may lead to an inverted rectangle, which is fine for Contains/Overlaps test but not for display.
    //void        ClipWithFull(const ImRect& r) { Min = ImClamp(Min, r.Min, r.Max); Max = ImClamp(Max, r.Min, r.Max); } // Full version, ensure both points are fully clipped.
    //void        Floor() { Min.x = IM_TRUNC(Min.x); Min.y = IM_TRUNC(Min.y); Max.x = IM_TRUNC(Max.x); Max.y = IM_TRUNC(Max.y); }
    bool        IsInverted() const { return Min.x > Max.x || Min.y > Max.y; }
    ImVec4      ToVec4() const { return ImVec4(Min.x, Min.y, Max.x, Max.y); }
    const ImVec4& AsVec4() const { return *(const ImVec4*)&Min.x; }
};

// UI configuration and UI feedback
struct IOUserData
{
    //to UI
    float dpiScale = 1.f;
    ImVec2 displayOffsetMin;
    ImVec2 displayOffsetMax;
    float dimBgRatio = 1.f;
    bool kbdShown = false;
    std::string activeActivity;
    //from UI
    int imeType = ImeText;
    ImGuiID longPressID = 0;

    void NewFrame();
    Rect WorkRect() const;
};

struct Texture
{
    ImTextureID id = 0;
    int w, h;
    explicit operator bool() const { return id != 0; }
};

struct CustomWidgetArgs
{
    const char* id;
    ImVec2 size;

    CustomWidgetArgs(const char* label, const ImVec2& sz) : size(sz) {}
};

struct Animator
{
    //todo: configure
    static const float DurOpenPopup;
    static const float DurClosePopup;

    void StartPersistent(float* v, float s, float e, float dur);
    void StartOnce(float* v, float s, float e, float dur);
    bool IsDone() const;
    //to be called from withing Begin
    void Tick();

    ImVec2 GetWindowSize() const;

private:
    struct Var
    {
        float time;
        float* var = nullptr;
        float start, end;
        float duration;
        bool oneShot;
    };
    std::vector<Var> vars;
    ImVec2 wsize{ 0, 0 };
};

template <bool HORIZ>
struct BoxLayout
{
    static constexpr float ItemSize = 0;

    struct Stretch {
        float value;
        Stretch(float v) : value(v) {}
    };

    void Reset();
    void BeginLayout();

    //call after a widget call
    void AddSize(float spacing, float size);
    void AddSize(float spacing, Stretch size);

    //call after a widget call for 2nd and subsequent widgets in a row
    void UpdateSize(float spacing, float size);
    void UpdateSize(float spacing, Stretch size);

    //for use in SetNextItemWidth/ctor. Call after possible SameLine
    float GetSize();

private:
    struct Item {
        float spacing;
        float size;
        bool stretch;
    };

    std::vector<Item> items, prevItems;
};

using HBox = BoxLayout<true>;
using VBox = BoxLayout<false>;

//------------------------------------------------------------------------

IOUserData& GetUserData();

void SaveStyle(const std::string& spath, const ImGuiStyle* src = nullptr, const std::map<std::string, std::string>& extra = {});

//This function can be used in your code to load style and fonts from the INI file
void LoadStyle(const std::string& spath, float fontScaling = 1, ImGuiStyle* dst = nullptr, std::map<std::string, ImFont*>* fontMap = nullptr, std::map<std::string, std::string>* extra = nullptr);

//------------------------------------------------------------------------

bool Combo(const char* label, std::string* curr, const std::vector<std::string>& items, int flags = 0);

bool Combo(const char* label, std::string* curr, const char* items, int flags = 0);

// from imgui_internal.h
using SeparatorFlags = int;
enum SeparatorFlags_
{
    SeparatorFlags_None = 0,
    SeparatorFlags_Horizontal = 1 << 0,   // Axis default to current layout type, so generally Horizontal unless e.g. in a menu bar
    SeparatorFlags_Vertical = 1 << 1,
    SeparatorFlags_SpanAllColumns = 1 << 2,   // Make separator cover all columns of a legacy Columns() set.
};
void SeparatorEx(SeparatorFlags flags, float thickness = 1.f);

// Adds negative dimensions support
void Dummy(const ImVec2& size);

// This is an improved version of ImGui::Selectable.
// 1. allows to pass negative dimensions similarly to other widgets
// 2. changes meaning of size.x=0 to use label width. This is more consistent with size.y=0 which doesn't cause
//    height expansion in ImGui. It also fixes layout calculation in box sizer because GetItemRectSize never
//    returns the actual size. User can still supply -1 to request available width expansion
// 3. Interprets ImGuiSelectableFlags_Disabled in terms of PushItemFlag(Disabled) which
//    is imgui_internal only. For full disable state use Begin/EndDisabled
bool Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size);

bool Selectable(const char* label, bool* selected, ImGuiSelectableFlags flags, const ImVec2& size);

bool Splitter(bool split_horiz, float thickness, float* position, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);

//------------------------------------------------------------------------

bool IsItemDoubleClicked();

bool IsItemContextMenuClicked();

bool IsItemLongPressed(double dur = -1);

bool IsItemImeAction();

bool IsCurrentItemDisabled();

Texture LoadTextureFromFile(const std::string& filename, bool linearX = true, bool linearY = true, bool repeatX = false, bool repeatY = false);

//allows to define popups in the window and open it from widgets calling internally
//Push/PopID like TabControl
void OpenWindowPopup(const char* str_id, ImGuiPopupFlags flags = 0);

void Spacing(int n);

bool TableNextColumn(int n);

void NextColumn(int n);

Rect GetParentInnerRect();

Rect GetWindowClipRect();

void SetWindowSkipItems(bool skip);

ImFont* GetFontByName(const char* name);
ImFont* GetFontByName(const std::string& name);

void PushInvisibleScrollbar();

void PopInvisibleScrollbar();

struct CursorData
{
    ImVec2 cursorPos; //affects column border height
    ImVec2 cursorPosPrevLine; //used in SameLine
    ImVec2 prevLineSize;
    float prevLineTextBaseOffset;
    ImVec2 cursorMaxPos; //affects scrollbars
    ImVec2 idealMaxPos; //affects auto resize
    ImVec2 currLineSize;
    float currLineTextBaseOffset;
    bool isSetPos; //causes ErrorCheckUsingSetCursorPosToExtendParentBoundaries
    bool isSameLine;
};

CursorData GetCursorData();

void SetCursorData(const CursorData& data);

struct LastItemData
{
    ImGuiID ID;
    ImGuiItemFlags itemFlags;
    int statusFlags;
    Rect rect;
};

LastItemData GetLastItemData();

//This is to implement items with children such as Selectable
//Begin/EndGroup also sets itemID and rect but it has its own logic not suitable for us
void SetLastItemData(const LastItemData& data);

struct IgnoreWindowPaddingData
{
    ImVec2 maxPos;
    ImVec2 workRectMax;
    bool hasSize;
};

void PushIgnoreWindowPadding(ImVec2* sz, IgnoreWindowPaddingData* data);

void PopIgnoreWindowPadding(const IgnoreWindowPaddingData& data);

//optionally draws scrollbars so they can be kept hidden when no scrolling occurs
//returns:
//0 - nothing happening or scrolling continues
//1 - scrolling started
//2 - scrolling ended
int ScrollWhenDragging(bool drawScrollbars);

//this currently
//* allows to move popups on the screen side further out of the screen just to give it responsive feeling
//* detects modal/popup close event
//returns
// 0 - close popup either by sliding or clicking outside
// 1 - nothing happening
// 2 - todo: maximize up/down popup
int MoveWhenDragging(ImGuiDir dir, ImVec2& pos, float& dimBgRatio);

//todo
//intended for android
//original version doesn't respect ioUserData.displayMinMaxOffset
void RenderDimmedBackground(const Rect& rect, float alpha_mul);

void RenderFilledWindowCorners(ImDrawFlags fl);

//-------------------------------------------------------------------------

inline std::string FormatFallback(const char* fmt, const char* fmte)
{
    return std::string(fmt, fmte);
}

template <class A1, class... A>
std::string FormatFallback(const char* fmt, const char* fmte, A1&& arg, A&&... args)
{
    std::string s;
    for (const char* p = fmt; p != fmte; ++p)
    {
        if (*p == '{') {
            if (p + 1 == fmte)
                break;
            if (p[1] == '{') {
                s += '{';
                ++p;
            }
            else {
                const char* next = std::find(p + 1, fmte, '}');
                if (next == fmte)
                    break;
                if constexpr (std::is_same_v<std::decay_t<A1>, std::string>)
                    s += arg;
                else if constexpr (std::is_same_v<std::decay_t<A1>, const char*> ||
                                    std::is_same_v<std::decay_t<A1>, char*>)
                    s += arg;
                else if constexpr (std::is_same_v<std::decay_t<A1>, char>)
                    s += arg;
                else
                    s += std::to_string(arg);
                return s + FormatFallback(next + 1, fmte, args...);
            }
        }
        else
            s += *p;
    }
    return s;
}

#ifdef IMRAD_WITH_FMT

//only support format_string version for compile time checks
template <class... A>
std::string Format(fmt::format_string<A...> fmt, A&&... args)
{
    return fmt::format(fmt, std::forward<A>(args)...);
}

#elif CPLUSPLUS >= 202002L && __has_include(<format>)

//only support format_string version for compile time checks
template <class... A>
std::string Format(std::format_string<A...> fmt, A&&... args)
{
    return std::format(fmt, std::forward<A>(args)...);
}

#else

template <class... A>
std::string Format(const char* fmt, A&&... args)
{
    return FormatFallback(fmt, fmt + strlen(fmt), std::forward<A>(args)...);
}
#endif

} // namespace

#endif // IMRAD_H_INTERFACE

//-------------------------------------------------------------------------

#ifdef IMRAD_H_IMPLEMENTATION

#include <memory>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <imgui_internal.h> //CurrentItemFlags, GetCurrentWindow, GetCurrentContext, PushOverrideID

#ifdef IMRAD_WITH_LOAD_TEXTURE
#include <stb_image.h>
#endif

#ifdef IMRAD_WITH_MINIZIP
#include <unzip.h>
#endif

namespace ImRad
{

void IOUserData::NewFrame()
{
    if (!ImGui::GetIO().WantTextInput)
        imeType = ImeNone;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        longPressID = 0;
}

Rect IOUserData::WorkRect() const
{
    return {
        displayOffsetMin.x,
        displayOffsetMin.y,
        ImGui::GetMainViewport()->Size.x - displayOffsetMax.x,
        ImGui::GetMainViewport()->Size.y - displayOffsetMax.y };
}

const float Animator::DurOpenPopup = 0.4f;
const float Animator::DurClosePopup = 0.3f;

void Animator::StartPersistent(float* v, float s, float e, float dur)
{
    Var nvar{ 0, v, s, e, dur, false };
    for (auto& var : vars)
        if (var.var == v) {
            var = nvar;
            return;
        }
    vars.push_back(nvar);
}

void Animator::StartOnce(float* v, float s, float e, float dur)
{
    Var nvar{ 0, v, s, e, dur, true };
    for (auto& var : vars)
        if (var.var == v) {
            var = nvar;
            return;
        }
    vars.push_back(nvar);
}

bool Animator::IsDone() const
{
    for (const auto& var : vars)
        if (var.oneShot || std::abs(*var.var - var.end) / std::abs(var.end - var.start) > 0.01)
            return false;
    return true;
}

template <bool HORIZ>
void BoxLayout<HORIZ>::Reset()
{
    prevItems.clear();
}

template <bool HORIZ>
void BoxLayout<HORIZ>::BeginLayout()
{
    std::swap(prevItems, items);
    items.clear();

    float avail = HORIZ ? ImGui::GetContentRegionAvail().x : ImGui::GetContentRegionAvail().y;
    float total = 0;
    float stretchTotal = 0;
    for (Item& it : prevItems) {
        //it.spacing = (int)it.spacing;
        //it.size = (int)it.size;
        total += it.spacing;
        if (it.stretch)
            stretchTotal += it.size;
        else if (it.size < 0) {
            //total = avail + it.size;
            stretchTotal += 1.0;
            total += -it.size;
        }
        else
            total += it.size;
    }
    for (Item& it : prevItems) {
        if (it.stretch)
            it.size = (float)(int)(it.size * (avail - total) / stretchTotal);
        else if (it.size < 0)
            it.size = (float)(int)(1.0 * (avail - total) / stretchTotal);
        //it.size += avail;
    }
}

template <bool HORIZ>
void BoxLayout<HORIZ>::AddSize(float spacing, float size)
{
    if (size == ItemSize)
        size = HORIZ ? ImGui::GetItemRectSize().x : ImGui::GetItemRectSize().y;
    items.push_back({ spacing, size, false });
}

template <bool HORIZ>
void BoxLayout<HORIZ>::AddSize(float spacing, Stretch size)
{
    items.push_back({ spacing, size.value, true });
}

template <bool HORIZ>
void BoxLayout<HORIZ>::UpdateSize(float spacing, float size)
{
    assert(items.size());
    if (size == ItemSize)
        size = HORIZ ? ImGui::GetItemRectSize().x : ImGui::GetItemRectSize().y;
    Item& it = items.back();
    if (spacing > it.spacing)
        it.spacing = spacing;
    if (!it.stretch && (size > it.size || (size < 0 && it.size > 0)))
        it.size = size;
}

template <bool HORIZ>
void BoxLayout<HORIZ>::UpdateSize(float spacing, Stretch size)
{
    assert(items.size());
    Item& it = items.back();
    if (spacing > it.spacing)
        it.spacing = spacing;
    if (!it.stretch || size.value > it.size) {
        it.stretch = true;
        it.size = size.value;
    }
}

template <bool HORIZ>
float BoxLayout<HORIZ>::GetSize()
{
    bool sameLine = !HORIZ && ImGui::GetCurrentWindow()->DC.IsSameLine;
    size_t i = sameLine ? items.size() - 1 : items.size();
    if (i >= prevItems.size()) //widgets changed
        return 0;
    return prevItems[i].size;
}

//explicit instantiation to allow member functions in cpp
template struct BoxLayout<true>;
template struct BoxLayout<false>;

//to be called from withing Begin
void Animator::Tick()
{
    wsize = ImGui::GetWindowSize(); //cache actual windows size
    size_t j = 0;
    for (size_t i = 0; i < vars.size(); ++i) {
        auto& var = vars[i];
        var.time += ImGui::GetIO().DeltaTime;
        float distance = var.end - var.start;
        float x = var.time / var.duration;
        if (x > 1)
            x = 1.f;
        float y = 1 - (1 - x) * (1 - x); //easeOutQuad
        *var.var = var.start + y * distance;
        if (!var.oneShot || std::abs(x - 1.0) >= 0.01) { //keep current var
            if (j < i)
                vars[j] = var;
            ++j;
        }
    }
    vars.resize(j);
}

ImVec2 Animator::GetWindowSize() const
{
    return wsize;
}

IOUserData& GetUserData()
{
    static IOUserData data;
    return data;
}

bool Combo(const char* label, std::string* curr, const std::vector<std::string>& items, int flags)
{
    bool changed = false;
    if (ImGui::BeginCombo(label, curr->c_str(), flags))
    {
        for (const auto& item : items) {
            if (ImGui::Selectable(item.c_str(), item == *curr)) {
                *curr = item;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool Combo(const char* label, std::string* curr, const char* items, int flags)
{
    bool changed = false;
    if (ImGui::BeginCombo(label, curr->c_str(), flags))
    {
        const char* p = items;
        while (*p) {
            if (ImGui::Selectable(p, !curr->compare(p))) {
                *curr = p;
                changed = true;
            }
            p += strlen(p) + 1;
        }
        ImGui::EndCombo();
    }
    return changed;
}

void SeparatorEx(SeparatorFlags flags, float thickness)
{
    ImGuiSeparatorFlags fl = 0;
    if (flags & SeparatorFlags_Horizontal)
        fl |= ImGuiSeparatorFlags_Horizontal;
    if (flags & SeparatorFlags_Vertical)
        fl |= ImGuiSeparatorFlags_Vertical;
    if (flags & SeparatorFlags_SpanAllColumns)
        fl |= ImGuiSeparatorFlags_SpanAllColumns;

    ImGui::SeparatorEx(flags, thickness);
}

void Dummy(const ImVec2& size)
{
    //ImGui Dummy doesn't support negative dimensions like other controls
    ImVec2 sz = ImGui::CalcItemSize(size, 0, 0);
    return ImGui::Dummy(sz);
}

bool Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
    //support negative dimensions
    ImVec2 sz = ImGui::CalcItemSize(size, 0, 0);
    //fit width by default
    if (!sz.x && !(flags & ImGuiSelectableFlags_SpanAllColumns))
        sz.x = ImGui::CalcTextSize(label, nullptr, true).x;
    //redefine ImGuiSelectableFlags_Disabled in terms of PushItemFlags
    //to avoid going through imgui_internal.h
    bool disabled = flags & ImGuiSelectableFlags_Disabled;
    flags &= ~ImGuiSelectableFlags_Disabled;

    if (disabled)
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    bool ret = ImGui::Selectable(label, selected, flags, sz);
    if (disabled)
        ImGui::PopItemFlag();
    return ret;
}

bool Selectable(const char* label, bool* selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
    if (Selectable(label, *selected, flags, size)) {
        *selected = !*selected;
        return true;
    }
    return false;
}

bool Splitter(bool split_horiz, float thickness, float* position, float min_size1, float min_size2, float splitter_long_axis_size)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiID id = window->GetID("##Splitter");
    ImRect bb;
    bb.Min = window->DC.CursorPos;
    bb.Min[!split_horiz] += *position;

    bb.Max = bb.Min;
    ImVec2 sz(thickness, splitter_long_axis_size);
    if (!split_horiz)
        std::swap(sz[0], sz[1]);
    sz = ImGui::CalcItemSize(sz, 0.0f, 0.0f);
    bb.Max[0] += sz[0];
    bb.Max[1] += sz[1];

    float tmp = ImGui::GetContentRegionAvail().x - *position - thickness;
    return ImGui::SplitterBehavior(bb, id, split_horiz ? ImGuiAxis_X : ImGuiAxis_Y, position, &tmp, min_size1, min_size2, 0.0f);
}

bool IsItemDoubleClicked()
{
    return ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered();
}

bool IsItemContextMenuClicked()
{
    return ImGui::IsMouseReleased(ImGuiPopupFlags_MouseButtonDefault_) &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
}

bool IsItemLongPressed(double dur)
{
    if (dur < 0)
        dur = 0.5f;
    double time = ImGui::GetTime() - ImGui::GetIO().MouseClickedTime[ImGuiMouseButton_Left];
    return time > dur &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        ImGui::IsItemHovered();
}

bool IsItemImeAction()
{
    return ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_AppForward);
}

bool IsCurrentItemDisabled()
{
    return ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled;
}

//allows to define popups in the window and open it from widgets calling internally
//Push/PopID like TabControl
void OpenWindowPopup(const char* str_id, ImGuiPopupFlags flags)
{
    //RootWindow skips child window parents
    ImGui::PushOverrideID(ImGui::GetCurrentWindow()->RootWindow->ID);
    //todo: for drop down menu use button's BL corner
    ImGui::OpenPopup(str_id, flags);
    ImGui::PopID();
}

Rect GetParentInnerRect()
{
    ImRect r = ImGui::GetCurrentWindow()->InnerRect;
    if (ImGui::GetCurrentTable())
        r.ClipWith(ImGui::GetCurrentTable()->InnerRect);
    return r.ToVec4();
}

void Spacing(int n)
{
    /*while (n--)
        ImGui::Spacing();*/
        /*ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;*/
    float sp = ImGui::GetStyle().ItemSpacing.y;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + n * sp);
}

bool TableNextColumn(int n)
{
    bool b = true;
    while (n--)
        b = ImGui::TableNextColumn();
    return b;
}

void NextColumn(int n)
{
    while (n--)
        ImGui::NextColumn();
}

Rect GetWindowClipRect()
{
    return ImGui::GetCurrentWindow()->ClipRect.ToVec4();
}

void SetWindowSkipItems(bool skip)
{
    ImGui::GetCurrentWindow()->SkipItems = skip;
}

void PushInvisibleScrollbar()
{
    ImVec4 clr = ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarBg);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, { clr.x, clr.y, clr.z, 0 });
    clr = ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrab);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, { clr.x, clr.y, clr.z, 0 });
}

void PopInvisibleScrollbar()
{
    ImGui::PopStyleColor(2);
}

void SetItemID(ImGuiID id)
{
    GImGui->LastItemData.ID = id;
}

CursorData GetCursorData()
{
    CursorData data;
    ImGuiWindow* wnd = ImGui::GetCurrentWindow();
    data.cursorPos = wnd->DC.CursorPos;
    data.cursorPosPrevLine = wnd->DC.CursorPosPrevLine;
    data.prevLineSize = wnd->DC.PrevLineSize;
    data.prevLineTextBaseOffset = wnd->DC.PrevLineTextBaseOffset;
    data.cursorMaxPos = wnd->DC.CursorMaxPos;
    data.idealMaxPos = wnd->DC.IdealMaxPos;
    data.currLineSize = wnd->DC.CurrLineSize;
    data.currLineTextBaseOffset = wnd->DC.CurrLineTextBaseOffset;
    data.isSetPos = wnd->DC.IsSetPos;
    data.isSameLine = wnd->DC.IsSameLine;
    return data;
}

void SetCursorData(const CursorData& data)
{
    ImGuiWindow* wnd = ImGui::GetCurrentWindow();
    wnd->DC.CursorPos = data.cursorPos;
    wnd->DC.CursorPosPrevLine = data.cursorPosPrevLine;
    wnd->DC.PrevLineSize = data.prevLineSize;
    wnd->DC.PrevLineTextBaseOffset = data.prevLineTextBaseOffset;
    wnd->DC.CursorMaxPos = data.cursorMaxPos;
    wnd->DC.IdealMaxPos = data.idealMaxPos;
    wnd->DC.CurrLineSize = data.currLineSize;
    wnd->DC.CurrLineTextBaseOffset = data.currLineTextBaseOffset;
    wnd->DC.IsSetPos = data.isSetPos;
    wnd->DC.IsSameLine = data.isSameLine;
}

LastItemData GetLastItemData()
{
    const auto& imd = GImGui->LastItemData;
    LastItemData data;
    data.ID = imd.ID;
    data.itemFlags = imd.ItemFlags;
    data.statusFlags = imd.StatusFlags;
    data.rect = imd.Rect.ToVec4();
    return data;
}

void SetLastItemData(const LastItemData& data)
{
    ImGui::SetLastItemData(data.ID, data.itemFlags, data.statusFlags, data.rect.ToVec4());
}

void PushIgnoreWindowPadding(ImVec2* sz, IgnoreWindowPaddingData* data)
{
    ImGuiWindow* wnd = ImGui::GetCurrentWindow();
    data->hasSize = sz != nullptr;
    data->maxPos = wnd->DC.CursorMaxPos;
    data->workRectMax = wnd->WorkRect.Max;
    ImRect r = wnd->InnerRect;
    float fixbs = wnd->WindowBorderSize ? 1.f : 0; //not sure why imgui renders some additional border shadow
    r.Min.x += fixbs;
    r.Max.x -= fixbs;
    ImGui::PushClipRect(r.Min, r.Max, false);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    if (pos.x <= wnd->DC.CursorStartPos.x)
        pos.x = r.Min.x + fixbs;
    if (pos.y <= wnd->DC.CursorStartPos.y)
        pos.y = r.Min.y;
    ImGui::SetCursorScreenPos(pos);
    wnd->WorkRect.Max.x = r.Max.x; //used by Separator
    //+1 here is to exactly map sz=-1 to right/bottom edge
    if (sz && sz->x < 0)
        sz->x = r.Max.x + sz->x + 1 - pos.x;
    if (sz && sz->y < 0)
        sz->y = r.Max.y + sz->y + 1 - pos.y;
}

void PopIgnoreWindowPadding(const IgnoreWindowPaddingData& data)
{
    ImGuiWindow* wnd = ImGui::GetCurrentWindow();
    wnd->WorkRect.Max = data.workRectMax;
    if (data.hasSize)
    {
        ImVec2 pad = wnd->WindowPadding;
        if (wnd->DC.CursorMaxPos.x > data.maxPos.x)
            wnd->DC.CursorMaxPos.x -= pad.x;
        if (wnd->DC.CursorMaxPos.y > data.maxPos.y)
            wnd->DC.CursorMaxPos.y -= pad.y;
    }
    ImGui::PopClipRect();
}

int ScrollWhenDragging(bool drawScrollbars)
{
    static int dragState = 0;

    if (!ImGui::IsWindowFocused())
        return 0;

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        int ret = !dragState ? 1 : 0;
        dragState = 1;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImGui::GetCurrentContext()->NavHighlightItemUnderNav = true;
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        if (delta.x)
            ImGui::SetScrollX(window, window->Scroll.x - delta.x);
        if (delta.y)
            ImGui::SetScrollY(window, window->Scroll.y - delta.y);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);

        //scrollbars were made invisible, draw them again
        if (drawScrollbars)
        {
            bool tmp = window->SkipItems;
            window->SkipItems = false;
            ImGui::PushClipRect(window->Rect().Min, window->Rect().Max, false);
            ImVec4 clr = ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrab);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, { clr.x, clr.y, clr.z, 1 });
            if (window->ScrollbarX)
                ImGui::Scrollbar(ImGuiAxis_X);
            if (window->ScrollbarY)
                ImGui::Scrollbar(ImGuiAxis_Y);
            ImGui::PopStyleColor();
            ImGui::PopClipRect();
            window->SkipItems = tmp;
        }
        return ret;
    }
    else if (dragState == 1)
    {
        dragState = 0;
        ImGui::GetCurrentContext()->NavHighlightItemUnderNav = false;
        ImGui::GetIO().MousePos = { -FLT_MAX, -FLT_MAX }; //ignore mouse release event, buttons won't get pushed
        return 2;
    }

    return 0;
}

int MoveWhenDragging(ImGuiDir dir, ImVec2& pos, float& dimBgRatio)
{
    static int dragState = 0;
    static ImVec2 mousePos[3];
    static ImVec2 startPos, lastPos;
    static float lastDim;

    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            if (!dragState)
            {
                startPos = pos;
                mousePos[1] = mousePos[2] = ImGui::GetMousePos();
            }
            dragState = 1;
            mousePos[0] = mousePos[1];
            mousePos[1] = mousePos[2];
            mousePos[2] = ImGui::GetMousePos();
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            ImGui::GetCurrentContext()->NavHighlightItemUnderNav = true;

            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            //don't reset DragDelta - we need to apply full delta if pos
            //was externally modified with Animator
            //ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
            if (dir == ImGuiDir_Left) {
                pos.x = startPos.x + delta.x;
                dimBgRatio = (window->Size.x + pos.x) / window->Size.x;
            }
            else if (dir == ImGuiDir_Right) {
                pos.x = startPos.x - delta.x;
                dimBgRatio = (window->Size.x + pos.x) / window->Size.x;
            }
            else if (dir == ImGuiDir_Up) {
                pos.y = startPos.y + delta.y;
                dimBgRatio = (window->Size.y + pos.y) / window->Size.y;
            }
            else if (dir == ImGuiDir_Down) {
                pos.y = startPos.y - delta.y;
                dimBgRatio = (window->Size.y + pos.y) / window->Size.y;
            }
            if (pos.x > 0) {
                pos.x = 0;
                dimBgRatio = 1;
            }
            if (pos.y > 0) {
                pos.y = 0;
                dimBgRatio = 1;
            }
            lastPos = pos;
            lastDim = dimBgRatio;
        }
        else if (dragState == 1)
        {
            //apply lastPos because position could be rewritten by Animator but the real value
            //needs to be taken for next closing animation
            pos = lastPos;
            dimBgRatio = lastDim;
            dragState = 0;
            ImGui::GetCurrentContext()->NavHighlightItemUnderNav = false;
            ImGui::GetIO().MousePos = { -FLT_MAX, -FLT_MAX }; //ignore mouse release event, buttons won't get pushed

            float spx = (mousePos[2].x - mousePos[0].x) / 2;
            float spy = (mousePos[2].y - mousePos[0].y) / 2;
            if (dir == ImGuiDir_Left && spx < -5)
                return 0;
            if (dir == ImGuiDir_Right && spx > 5)
                return 0;
            if (dir == ImGuiDir_Up && spy < -5)
                return 0;
            if (dir == ImGuiDir_Down && spy > 5)
                return 0;
        }
    }

    if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
    {
        ImGui::GetIO().MouseClicked[0] = false; //eat event
        return 0;
    }

    return 1;
}

//todo
//intended for android
//original version doesn't respect ioUserData.displayMinMaxOffset
void RenderDimmedBackground(const Rect& rect, float alpha_mul)
{
    static ImVec4 origDimColor = { 0, 0, 0, 0.5f };
    ImVec4& styleDimColor = ImGui::GetStyle().Colors[ImGuiCol_ModalWindowDimBg];
    if (ImGui::ColorConvertFloat4ToU32(styleDimColor)) {
        origDimColor = styleDimColor;
        styleDimColor = { 0, 0, 0, 0 }; //disable ImGui dimming
    }
    ImU32 color = ImGui::ColorConvertFloat4ToU32({ origDimColor.x, origDimColor.y, origDimColor.z, origDimColor.w * alpha_mul });

    // Draw list have been trimmed already, hence the explicit recreation of a draw command if missing.
    // FIXME: This is creating complication, might be simpler if we could inject a drawlist in drawdata at a given position and not attempt to manipulate ImDrawCmd order.
    /*ImDrawList* dl = ImGui::GetCurrentWindow()->RootWindowDockTree->DrawList;
    dl->ChannelsMerge();
    //if (dl->CmdBuffer.Size == 0)
        dl->AddDrawCmd();
    dl->PushClipRectFullScreen();
    //dl->PushClipRect(rect.Min - ImVec2(1, 1), rect.Max + ImVec2(1, 1), false); // FIXME: Need to stricty ensure ImDrawCmd are not merged (ElemCount==6 checks below will verify that)
    dl->AddRectFilled(rect.Min, rect.Max, color);
    ImDrawCmd cmd = dl->CmdBuffer.back();
    IM_ASSERT(cmd.ElemCount == 6);
    dl->CmdBuffer.pop_back();
    dl->CmdBuffer.push_front(cmd);
    dl->AddDrawCmd(); // We need to create a command as CmdBuffer.back().IdxOffset won't be correct if we append to same command.
    dl->PopClipRect();*/

    /*ImDrawList* dl = ImGui::GetCurrentWindow()->RootWindowDockTree->DrawList;
    dl->PushClipRectFullScreen();
    dl->AddRectFilled(rect.Min, rect.Max, color);
    dl->PopClipRect();*/

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRectFullScreen();
    const ImRect& wr = ImGui::GetCurrentWindow()->Rect();
    dl->AddRectFilled(rect.Min, { rect.Max.x, wr.Min.y }, color);
    dl->AddRectFilled({ rect.Min.x, wr.Min.y }, { wr.Min.x, wr.Max.y }, color);
    dl->AddRectFilled({ wr.Max.x, wr.Min.y }, { rect.Max.x, wr.Max.y }, color);
    dl->AddRectFilled({ rect.Min.x, wr.Max.y }, rect.Max, color);
    float r = ImGui::GetCurrentWindow()->WindowRounding;
    if (r)
    {
        dl->AddRectFilled(wr.Min, { wr.Min.x + r, wr.Min.y + r }, color);
        dl->AddRectFilled({ wr.Min.x, wr.Max.y - r }, { wr.Min.x + r, wr.Max.y }, color);
        dl->AddRectFilled({ wr.Max.x - r, wr.Min.y }, { wr.Max.x, wr.Min.y + r }, color);
        dl->AddRectFilled({ wr.Max.x - r, wr.Max.y - r }, wr.Max, color);

        ImU32 bg = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_PopupBg));
        dl->PathArcToFast({ wr.Min.x + r, wr.Min.y + r }, r, 6, 9);
        dl->PathLineTo({ wr.Min.x + r, wr.Min.y + r });
        dl->PathFillConvex(bg);
        dl->PathArcToFast({ wr.Min.x + r, wr.Max.y - r }, r, 3, 6);
        dl->PathLineTo({ wr.Min.x + r, wr.Max.y - r });
        dl->PathFillConvex(bg);
        dl->PathArcToFast({ wr.Max.x - r, wr.Min.y + r }, r, 9, 12);
        dl->PathLineTo({ wr.Max.x - r, wr.Min.y + r });
        dl->PathFillConvex(bg);
        dl->PathArcToFast({ wr.Max.x - r, wr.Max.y - r }, r, 0, 3);
        dl->PathLineTo({ wr.Max.x - r, wr.Max.y - r });
        dl->PathFillConvex(bg);
    }
    dl->PopClipRect();
}

void RenderFilledWindowCorners(ImDrawFlags fl)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    float r = (win->Flags & ImGuiWindowFlags_Popup) && !(win->Flags & ImGuiWindowFlags_Modal) ?
        ImGui::GetStyle().PopupRounding : ImGui::GetStyle().WindowRounding;
    ImVec2 pad = ImGui::GetStyle().WindowPadding;
    //GetWindowBgColorIdx is not accessible
    ImU32 col = ImGui::GetColorU32((win->Flags & ImGuiWindowFlags_Popup) ? ImGuiCol_PopupBg : ImGuiCol_WindowBg);

    ImGui::PushClipRect(pos, { pos.x + size.x, pos.y + size.y }, false);

    if (fl & ImDrawFlags_RoundCornersBottomLeft)
        dl->AddRectFilled({ pos.x, pos.y + size.y - r }, { pos.x + r, pos.y + size.y }, col);
    if (fl & ImDrawFlags_RoundCornersBottomRight)
        dl->AddRectFilled({ pos.x + size.x - r, pos.y + size.y - r }, { pos.x + size.x, pos.y + size.y }, col);
    if (fl & ImDrawFlags_RoundCornersTopLeft)
        dl->AddRectFilled(pos, { pos.x + r, pos.y + r }, col);
    if (fl & ImDrawFlags_RoundCornersTopRight)
        dl->AddRectFilled({ pos.x + size.x - r, pos.y }, { pos.x + size.x, pos.y + r }, col);

    ImGui::PopClipRect();
}

#ifdef IMRAD_WITH_MINIZIP
std::vector<uint8_t> UnzipAssetData(const std::string& url)
{
    std::vector<uint8_t> buffer;

    //decompose resource url
    if (url.compare(0, 4, "zip:"))
        return buffer;
    size_t i = url.find('.');
    if (i == std::string::npos)
        return buffer;
    i = url.find('/', i + 1);
    if (i == std::string::npos)
        return buffer;
    std::string rname(url.begin() + i + 1, url.end());
    std::string fname(url.begin() + 4, url.begin() + i);

    unzFile zipfh = unzOpen(fname.c_str());
    if (!zipfh)
        return buffer;

    unz_global_info global_info;
    if (unzGetGlobalInfo(zipfh, &global_info) != UNZ_OK)
    {
        unzClose(zipfh);
        return buffer;
    }

    for (size_t i = 0; i < global_info.number_entry; ++i)
    {
        unz_file_info file_info;
        char filename[256];
        if (unzGetCurrentFileInfo(zipfh, &file_info, filename, 256, NULL, 0, NULL, 0) != UNZ_OK) {
            unzGoToNextFile(zipfh);
            continue;
        }
        if (filename != rname) {
            unzGoToNextFile(zipfh);
            continue;
        }
        if (unzOpenCurrentFile(zipfh) != UNZ_OK) {
            break;
        }
        buffer.resize(file_info.uncompressed_size);
        for (size_t pos = 0; pos < buffer.size(); )
        {
            int nbytes = unzReadCurrentFile(zipfh, buffer.data() + pos, 8192);
            if (nbytes < 0)
            {
                buffer.clear();
                unzCloseCurrentFile(zipfh);
                break;
            }
            pos += nbytes;
        }

        unzCloseCurrentFile(zipfh);
        break;
    }

    unzClose(zipfh);
    return buffer;
}
#endif

#ifdef IMRAD_WITH_LOAD_TEXTURE

#ifndef GL_TEXTURE_2D
#error IMRAD_WITH_LOAD_TEXTURE requires OpenGL/ES header to be included *before* IMRAD_H_IMPLEMENTATION
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Simple helper function to load an image into a OpenGL texture with common settings
// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
Texture LoadTextureFromFile(const std::string& filename, bool linearX, bool linearY, bool repeatX, bool repeatY)
{
    // Load from file
    Texture tex;
    unsigned char* image_data = nullptr;
#ifdef ANDROID
    auto image = GetAndroidAsset(filename.c_str());
    image_data = stbi_load_from_memory(image.first, image.second, &tex.w, &tex.h, NULL, 4);
#else
#ifdef IMRAD_WITH_MINIZIP
    if (!filename.compare(0, 4, "zip:"))
    {
        auto buffer = UnzipAssetData(filename);
        image_data = stbi_load_from_memory(buffer.data(), (int)buffer.size(), &tex.w, &tex.h, NULL, 4);
    }
    else
#endif
    {
        image_data = stbi_load(filename.c_str(), &tex.w, &tex.h, NULL, 4);
    }
#endif
    if (image_data == NULL)
        return {};

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    tex.id = (ImTextureID)(intptr_t)image_texture;
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linearX ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linearY ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeatX ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeatY ? GL_REPEAT : GL_CLAMP_TO_EDGE);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.w, tex.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    return tex;
}
#endif

std::filesystem::path u8path(const std::string& s)
{
#if CPLUSPLUS >= 202002L
    return std::filesystem::path((const char8_t*)s.data(), (const char8_t*)s.data() + s.size());
#else
    return std::filesystem::u8path(s);
#endif
}

std::string u8string(const std::filesystem::path& p)
{
#if CPLUSPLUS >= 202002L
    return std::string((const char*)p.u8string().data());
#else
    return p.u8string();
#endif
}

void SaveStyle(const std::string& spath, const ImGuiStyle* src, const std::map<std::string, std::string>& extra)
{
    const ImGuiStyle* style = src ? src : &ImGui::GetStyle();
    auto stylePath = u8path(spath);
    std::ofstream fout(stylePath);
    if (!fout)
        throw std::runtime_error("can't write '" + stylePath.string() + "'");

    fout << "[colors]\n";
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        fout << ImGui::GetStyleColorName(i) << " = ";
        const auto& clr = style->Colors[i];
        fout << int(clr.x * 255) << " " << int(clr.y * 255) << " "
            << int(clr.z * 255) << " " << int(clr.w * 255) << "\n";
    }

    fout << "\n[variables]\n";
#define WRITE_FLT(a) fout << #a " = " << style->a << "\n"
#define WRITE_VEC(a) fout << #a " = " << style->a.x << " " << style->a.y << "\n"

    WRITE_FLT(Alpha);
    WRITE_FLT(DisabledAlpha);
    WRITE_VEC(WindowPadding);
    WRITE_FLT(WindowRounding);
    WRITE_FLT(WindowBorderSize);
    WRITE_VEC(WindowMinSize);
    WRITE_VEC(WindowTitleAlign);
    WRITE_FLT(ChildRounding);
    WRITE_FLT(ChildBorderSize);
    WRITE_FLT(PopupRounding);
    WRITE_FLT(PopupBorderSize);
    WRITE_VEC(FramePadding);
    WRITE_FLT(FrameRounding);
    WRITE_FLT(FrameBorderSize);
    WRITE_VEC(ItemSpacing);
    WRITE_VEC(ItemInnerSpacing);
    WRITE_VEC(CellPadding);
    WRITE_VEC(TouchExtraPadding);
    WRITE_FLT(IndentSpacing);
    WRITE_FLT(ColumnsMinSpacing);
    WRITE_FLT(ScrollbarSize);
    WRITE_FLT(ScrollbarRounding);
    WRITE_FLT(ScrollbarPadding);
    WRITE_FLT(GrabMinSize);
    WRITE_FLT(GrabRounding);
    WRITE_FLT(ImageBorderSize);
    WRITE_FLT(TabRounding);
    WRITE_FLT(TabBorderSize);
    WRITE_FLT(TabMinWidthBase);
    WRITE_FLT(TabMinWidthShrink);
    WRITE_FLT(TabCloseButtonMinWidthSelected);
    WRITE_FLT(TabCloseButtonMinWidthUnselected);
    WRITE_FLT(TabBarBorderSize);
    WRITE_FLT(TabBarOverlineSize);
    WRITE_FLT(TreeLinesSize);
    WRITE_FLT(TreeLinesRounding);
    WRITE_VEC(ButtonTextAlign);
    WRITE_VEC(SelectableTextAlign);
    WRITE_FLT(SeparatorTextBorderSize);
    WRITE_VEC(SeparatorTextAlign);
    WRITE_VEC(SeparatorTextPadding);
#undef WRITE_FLT
#undef WRITE_VEC

    fout << "\n[fonts]\n";
    fout << "Default = \"Roboto-Regular.ttf\" size 15\n";

    std::string lastSection;
    for (const auto& kv : extra)
    {
        size_t i = kv.first.find_last_of('.');
        if (i == std::string::npos)
            continue;
        if (kv.first.substr(0, i) != lastSection) {
            lastSection = kv.first.substr(0, i);
            fout << "\n[" << lastSection << "]\n";
        }
        fout << kv.first.substr(i + 1) << " = " << kv.second << "\n";
    }
}

void LoadStyle(const std::string& spath, float fontScaling, ImGuiStyle* dst, std::map<std::string, ImFont*>* fontMap, std::map<std::string, std::string>* extra)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    *style = ImGuiStyle();
    auto& io = ImGui::GetIO();

    auto stylePath = u8path(spath);
    std::ifstream fin(stylePath);
    if (!fin)
        throw std::runtime_error("Can't read " + stylePath.string());
    std::string line;
    std::string cat;
    int lastClr = -1;
    std::string lastFont;
    while (std::getline(fin, line))
    {
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;
        else if (line[0] == '[' && line.back() == ']')
            cat = line.substr(1, line.size() - 2);
        else
        {
            size_t i1 = line.find_first_not_of("=\t ", 0);
            size_t i2 = line.find_first_of("=\t ", i1);
            if (i1 == std::string::npos || i2 == std::string::npos)
                continue;
            std::string key = line.substr(i1, i2 - i1);
            i1 = line.find_first_not_of("=\t ", i2);
            std::istringstream is(line.substr(i1));

            if (cat == "colors")
            {
                for (int i = lastClr + 1; i != lastClr; i = (i + 1) % ImGuiCol_COUNT)
                    if (key == ImGui::GetStyleColorName(i)) {
                        lastClr = i;
                        int r, g, b, a;
                        is >> r >> g >> b >> a;
                        style->Colors[i].x = r / 255.f;
                        style->Colors[i].y = g / 255.f;
                        style->Colors[i].z = b / 255.f;
                        style->Colors[i].w = a / 255.f;
                        break;
                    }
            }
            else if (cat == "variables")
            {
#define READ_FLT(a) if (key == #a) is >> style->a;
#define READ_VEC(a) if (key == #a) is >> style->a.x >> style->a.y;

                READ_FLT(Alpha);
                READ_FLT(DisabledAlpha);
                READ_VEC(WindowPadding);
                READ_FLT(WindowRounding);
                READ_FLT(WindowBorderSize);
                READ_VEC(WindowMinSize);
                READ_VEC(WindowTitleAlign);
                READ_FLT(ChildRounding);
                READ_FLT(ChildBorderSize);
                READ_FLT(PopupRounding);
                READ_FLT(PopupBorderSize);
                READ_VEC(FramePadding);
                READ_FLT(FrameRounding);
                READ_FLT(FrameBorderSize);
                READ_VEC(ItemSpacing);
                READ_VEC(ItemInnerSpacing);
                READ_VEC(CellPadding);
                READ_VEC(TouchExtraPadding);
                READ_FLT(IndentSpacing);
                READ_FLT(ColumnsMinSpacing);
                READ_FLT(ScrollbarSize);
                READ_FLT(ScrollbarRounding);
                READ_FLT(ScrollbarPadding);
                READ_FLT(GrabMinSize);
                READ_FLT(GrabRounding);
                READ_FLT(ImageBorderSize);
                READ_FLT(TabRounding);
                READ_FLT(TabBorderSize);
                READ_FLT(TabMinWidthBase);
                READ_FLT(TabMinWidthShrink);
                READ_FLT(TabCloseButtonMinWidthSelected);
                READ_FLT(TabCloseButtonMinWidthUnselected);
                READ_FLT(TabBarBorderSize);
                READ_FLT(TabBarOverlineSize);
                READ_FLT(TreeLinesSize);
                READ_FLT(TreeLinesRounding);
                READ_VEC(ButtonTextAlign);
                READ_VEC(SelectableTextAlign);
                READ_FLT(SeparatorTextBorderSize);
                READ_VEC(SeparatorTextAlign);
                READ_VEC(SeparatorTextPadding);
#undef READ_FLT
#undef READ_VEC
            }
            else if (cat == "fonts")
            {
                std::string fname;
                float size = 0.f;
                ImVec2 goffset;
                bool hasRange = false;
                static std::vector<std::unique_ptr<ImWchar[]>> rngs;

                is >> std::quoted(fname);
                std::string tmp;
                while (is >> tmp)
                {
                    if (tmp == "size")
                        is >> size;
                    else if (tmp == "range") {
                        hasRange = true;
                        //needs to outlive this function
                        rngs.push_back(std::unique_ptr<ImWchar[]>(new ImWchar[3]));
                        is >> rngs.back()[0] >> rngs.back()[1];
                        rngs.back()[2] = 0;
                    }
                    else if (tmp == "goffset") {
                        is >> goffset.x >> goffset.y;
                    }
                }

                ImFontConfig cfg;
                snprintf(cfg.Name, sizeof(cfg.Name), "%s", key.c_str());
                cfg.MergeMode = key == lastFont;
                cfg.GlyphRanges = hasRange ? rngs.back().get() : nullptr;
                cfg.GlyphOffset = { goffset.x * fontScaling, goffset.y * fontScaling };
                ImFont* font;
#ifdef ANDROID
                auto font_data = GetAndroidAsset(fname.c_str());
                font = io.Fonts->AddFontFromMemoryTTF(font_data.first, font_data.second, size * fontScaling, &cfg);
#else
#ifdef IMRAD_WITH_MINIZIP
                if (!fname.compare(0, 4, "zip:"))
                {
                    auto fpath = u8path(fname.substr(4));
                    if (fpath.is_relative())
                        fpath = stylePath.parent_path() / fpath;
                    auto buffer = UnzipAssetData("zip:" + u8string(fpath));
                    if (buffer.empty())
                        throw std::runtime_error("Can't read '" + u8string(fname) + "'");
                    font = io.Fonts->AddFontFromMemoryTTF(buffer.data(), (int)buffer.size(), size * fontScaling, &cfg);
                }
                else
#endif
                {
                    auto fpath = u8path(fname);
                    if (fpath.is_relative())
                        fpath = stylePath.parent_path() / fpath;
                    if (!std::ifstream(fpath))
                        throw std::runtime_error("Can't read '" + u8string(fpath) + "'");
                    font = io.Fonts->AddFontFromFileTTF(u8string(fpath).c_str(), size * fontScaling, &cfg);
                }
#endif
                if (!font)
                    throw std::runtime_error("Can't load " + fname);
                if (!cfg.MergeMode && fontMap)
                    (*fontMap)[lastFont == "" ? "" : key] = font;

                lastFont = key;
            }
            else if (extra)
            {
                (*extra)[cat + "." + key] = is.str();
            }
        }
    }
    if (fontMap && !(*fontMap).count(""))
        (*fontMap)[""] = io.Fonts->AddFontDefault();
}

ImFont* GetFontByName(const char* name)
{
    if (!*name)
        return ImGui::GetDefaultFont();

    const auto& io = ImGui::GetIO();
    for (const auto& cfg : io.Fonts->Sources) {
        if (cfg.MergeMode)
            continue;
        if (!strcmp(name, cfg.Name))
            return cfg.DstFont;
    }
    return nullptr;
}

ImFont* GetFontByName(const std::string& name)
{
    return GetFontByName(name.c_str());
}

} // namespace

#endif